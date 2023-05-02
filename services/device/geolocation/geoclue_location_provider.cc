// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/device/geolocation/geoclue_location_provider.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/time/time.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "services/device/public/cpp/geolocation/geoposition.h"

namespace device {

namespace {

constexpr char kServiceName[] = "org.freedesktop.GeoClue2";
constexpr char kLocationInterfaceName[] = "org.freedesktop.GeoClue2.Location";
constexpr char kClientInterfaceName[] = "org.freedesktop.GeoClue2.Client";
constexpr char kManagerInterfaceName[] = "org.freedesktop.GeoClue2.Manager";
constexpr char kManagerObjectPath[] = "/org/freedesktop/GeoClue2/Manager";
constexpr char kBraveDesktopId[] = "com.brave.Browser";

mojom::Geoposition GetErrorPosition() {
  mojom::Geoposition response;
  response.error_code = mojom::Geoposition_ErrorCode::POSITION_UNAVAILABLE;
  response.error_message = "Unable to create instance of Geolocation API";
  return response;
}

} // namespace

GeoClueProperties::GeoClueProperties(dbus::ObjectProxy *proxy,
                                     const std::string &interface_name,
                                     const PropertyChangedCallback &callback)
    : dbus::PropertySet(proxy, interface_name, callback) {
  RegisterProperty("DesktopId", &desktop_id);
}
GeoClueProperties::~GeoClueProperties() = default;

GeoClueLocationProperties::GeoClueLocationProperties(
    dbus::ObjectProxy *proxy, const std::string &interface_name,
    base::OnceCallback<void()> on_got_initial_values)
    : dbus::PropertySet(proxy, interface_name, base::NullCallback()),
      on_got_initial_values_(std::move(on_got_initial_values)) {
  RegisterProperty("Latitude", &latitude);
  RegisterProperty("Longitude", &longitude);
  RegisterProperty("Accuracy", &accuracy);
  RegisterProperty("Altitude", &altitude);
  RegisterProperty("Speed", &speed);
  RegisterProperty("Heading", &heading);
}

GeoClueLocationProperties::~GeoClueLocationProperties() = default;

void GeoClueLocationProperties::OnGetAll(dbus::Response *response) {
  dbus::PropertySet::OnGetAll(response);

  if (on_got_initial_values_) {
    std::move(on_got_initial_values_).Run();
  }
}

GeoClueLocationProvider::GeoClueLocationProvider() {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  options.connection_type = dbus::Bus::PRIVATE;
  options.dbus_task_runner = dbus_thread_linux::GetTaskRunner();

  bus_ = base::MakeRefCounted<dbus::Bus>(options);
}
GeoClueLocationProvider::~GeoClueLocationProvider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  dbus_thread_linux::GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&dbus::Bus::ShutdownAndBlock, std::move(bus_)));
}

void GeoClueLocationProvider::SetUpdateCallback(
    const LocationProviderUpdateCallback &callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  location_update_callback_ = callback;
}

void GeoClueLocationProvider::StartProvider(bool high_accuracy) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (client_state_ != kStopped) {
    return;
  }
  client_state_ = kInitializing;

  dbus::ObjectProxy *proxy =
      bus_->GetObjectProxy(kServiceName, dbus::ObjectPath(kManagerObjectPath));

  dbus::MethodCall call(kManagerInterfaceName, "GetClient");
  proxy->CallMethod(
      &call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::BindOnce(&GeoClueLocationProvider::OnGetClientCompleted,
                     weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueLocationProvider::StopProvider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (client_state_ == kStopped) {
    return;
  }

  client_state_ = kStopped;

  // Invalidate weak pointers, so we don't continue any async operations.
  weak_ptr_factory_.InvalidateWeakPtrs();

  // Stop can be called before the gclue_client_ has resolved.
  if (!gclue_client_) {
    return;
  }

  dbus::MethodCall stop(kClientInterfaceName, "Stop");
  gclue_client_->CallMethod(&stop, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                            base::DoNothing());

  // Reset pointers to dbus objects. They will be destroyed when all references
  // are gone.
  gclue_client_.reset();
  gclue_client_properties_.reset();
  gclue_location_properties_.reset();
}

const mojom::Geoposition &GeoClueLocationProvider::GetPosition() {
  return last_position_;
}

void GeoClueLocationProvider::OnPermissionGranted() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  permission_granted_ = true;
  StartClient();
}

void GeoClueLocationProvider::SetLocation(const mojom::Geoposition &position) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  last_position_ = position;

  if (client_state_ != kStarted) {
    return;
  }

  if (last_position_.error_code == mojom::Geoposition_ErrorCode::NONE &&
      !device::ValidateGeoposition(last_position_)) {
    return;
  }
  location_update_callback_.Run(this, last_position_);
}

void GeoClueLocationProvider::OnGetClientCompleted(dbus::Response *response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!response) {
    SetLocation(GetErrorPosition());
    return;
  }

  dbus::MessageReader reader(response);
  dbus::ObjectPath path;
  if (!reader.PopObjectPath(&path)) {
    SetLocation(GetErrorPosition());
    return;
  }

  gclue_client_ = bus_->GetObjectProxy(kServiceName, path);
  gclue_client_properties_ = std::make_unique<GeoClueProperties>(
      gclue_client_.get(), kClientInterfaceName, base::NullCallback());

  SetDesktopId();
}

void GeoClueLocationProvider::SetDesktopId() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  gclue_client_properties_->desktop_id.Set(
      kBraveDesktopId, base::BindOnce(&GeoClueLocationProvider::OnSetDesktopId,
                                      weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueLocationProvider::OnSetDesktopId(bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!success) {
    LOG(ERROR) << "Failed to set desktop id. GeoClue2 location provider will "
                  "not work properly";
    SetLocation(GetErrorPosition());
    return;
  }

  ConnectSignal();
}

void GeoClueLocationProvider::ConnectSignal() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  gclue_client_->ConnectToSignal(
      kClientInterfaceName, "LocationUpdated",
      base::BindRepeating(
          [](base::WeakPtr<GeoClueLocationProvider> provider,
             dbus::Signal *signal) {
            dbus::MessageReader reader(signal);
            dbus::ObjectPath old_location;
            dbus::ObjectPath new_location;
            if (!reader.PopObjectPath(&old_location) ||
                !reader.PopObjectPath(&new_location)) {
              if (provider) {
                provider->SetLocation(GetErrorPosition());
              }
              return;
            }

            if (provider) {
              provider->ReadGeoClueLocation(new_location);
            }
          },
          weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&GeoClueLocationProvider::OnSignalConnected,
                     weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueLocationProvider::OnSignalConnected(
    const std::string &interface_name, const std::string &signal_name,
    bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!success) {
    LOG(ERROR) << "Failed to connect to LocationUpdated Signal. GeoClue2 "
                  "location provider will "
                  "not work properly";
    SetLocation(GetErrorPosition());
    return;
  }

  client_state_ = kInitialized;
  StartClient();
}

void GeoClueLocationProvider::StartClient() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!gclue_client_ || !permission_granted_ || client_state_ != kInitialized) {
    return;
  }

  client_state_ = kStarting;

  dbus::MethodCall start(kClientInterfaceName, "Start");
  gclue_client_->CallMethod(
      &start, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::BindOnce(&GeoClueLocationProvider::OnClientStarted,
                     weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueLocationProvider::OnClientStarted(dbus::Response *response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  client_state_ = kStarted;
}

void GeoClueLocationProvider::ReadGeoClueLocation(
    const dbus::ObjectPath &location_path) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  dbus::ObjectProxy *location_proxy =
      bus_->GetObjectProxy(kServiceName, location_path);
  gclue_location_properties_ = std::make_unique<GeoClueLocationProperties>(
      location_proxy, kLocationInterfaceName,
      base::BindRepeating(&GeoClueLocationProvider::OnReadGeoClueLocation,
                          weak_ptr_factory_.GetWeakPtr()));
  gclue_location_properties_->GetAll();
}

void GeoClueLocationProvider::OnReadGeoClueLocation() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  mojom::Geoposition position;
  position.latitude = gclue_location_properties_->latitude.value();
  position.longitude = gclue_location_properties_->longitude.value();
  position.accuracy = gclue_location_properties_->accuracy.value();
  position.altitude = gclue_location_properties_->altitude.value();
  position.heading = gclue_location_properties_->heading.value();
  position.speed = gclue_location_properties_->speed.value();
  position.error_code = mojom::Geoposition::ErrorCode::NONE;
  position.timestamp = base::Time::Now();
  SetLocation(position);
}

std::unique_ptr<LocationProvider> NewSystemLocationProvider(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    GeolocationManager *geolocation_manager) {
  return std::make_unique<GeoClueLocationProvider>();
}

} // namespace device
