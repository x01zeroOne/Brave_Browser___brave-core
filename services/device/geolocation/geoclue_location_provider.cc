// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/services/device/geolocation/geoclue_location_provider.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "dbus/property.h"
#include "services/device/public/cpp/geolocation/geoposition.h"
#include "services/device/public/mojom/geoposition.mojom.h"

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
  RegisterProperty("Location", &location);
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

GeoClueProvider::GeoClueProvider() {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  options.connection_type = dbus::Bus::PRIVATE;
  options.dbus_task_runner = dbus_thread_linux::GetTaskRunner();

  bus_ = base::MakeRefCounted<dbus::Bus>(options);
}
GeoClueProvider::~GeoClueProvider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  dbus_thread_linux::GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&dbus::Bus::ShutdownAndBlock, std::move(bus_)));
}

void GeoClueProvider::SetUpdateCallback(
    const LocationProviderUpdateCallback &callback) {
  location_update_callback_ = callback;
}

void GeoClueProvider::StartProvider(bool high_accuracy) {
  if (client_state_ != kStopped) {
    return;
  }
  client_state_ = kInitializing;

  dbus::ObjectProxy *proxy =
      bus_->GetObjectProxy(kServiceName, dbus::ObjectPath(kManagerObjectPath));

  dbus::MethodCall call(kManagerInterfaceName, "GetClient");
  proxy->CallMethod(&call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                    base::BindOnce(&GeoClueProvider::OnGetClientCompleted,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueProvider::StopProvider() {
  if (client_state_ == kStopped) {
    return;
  }

  client_state_ = kStopped;

  // Stop can be called before the gclue_client_ has resolved.
  if (!gclue_client_) {
    return;
  }

  dbus::MethodCall stop(kClientInterfaceName, "Stop");
  gclue_client_->CallMethod(&stop, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                            base::DoNothing());
}

const mojom::Geoposition &GeoClueProvider::GetPosition() {
  return last_position_;
}

void GeoClueProvider::OnPermissionGranted() {
  permission_granted_ = true;
  StartClient();
}

void GeoClueProvider::OnLocationChanged() {
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

void GeoClueProvider::SetLocation(const mojom::Geoposition &position) {
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

void GeoClueProvider::OnGetClientCompleted(dbus::Response *response) {
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

  gclue_client_properties_->desktop_id.Set(
      kBraveDesktopId, base::BindOnce(&GeoClueProvider::OnSetDesktopId,
                                      weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueProvider::OnSetDesktopId(bool success) {
  if (!success) {
    LOG(ERROR) << "Failed to set desktop id. GeoClue2 location provider will "
                  "not work properly";
    SetLocation(GetErrorPosition());
    return;
  }

  client_state_ = kInitialized;
  StartClient();
}

void GeoClueProvider::StartClient() {
  if (!gclue_client_ || !permission_granted_ || client_state_ != kInitialized) {
    return;
  }

  client_state_ = kStarting;

  dbus::MethodCall start(kClientInterfaceName, "Start");
  gclue_client_->CallMethod(&start, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                            base::BindOnce(&GeoClueProvider::OnStarted,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueProvider::OnStarted(dbus::Response *response) {
  client_state_ = kStarted;

  gclue_client_->ConnectToSignal(
      kClientInterfaceName, "LocationUpdated",
      base::BindRepeating(
          [](base::WeakPtr<GeoClueProvider> provider, dbus::Signal *signal) {
            dbus::MessageReader reader(signal);
            dbus::ObjectPath old_location;
            dbus::ObjectPath new_location;
            if (!reader.PopObjectPath(&old_location) ||
                !reader.PopObjectPath(&new_location)) {
              if (provider)
                provider->SetLocation(GetErrorPosition());
              return;
            }

            if (provider) {
              provider->SetLocationPath(new_location);
            }
          },
          weak_ptr_factory_.GetWeakPtr()),
      base::DoNothing());
  gclue_client_properties_->location.Get(
      base::BindOnce(&GeoClueProvider::OnGetLocationObjectPath,
                     weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueProvider::OnGetLocationObjectPath(bool success) {
  dbus::ObjectPath location_path = gclue_client_properties_->location.value();
  SetLocationPath(location_path);
}

void GeoClueProvider::SetLocationPath(const dbus::ObjectPath &location_path) {
  dbus::ObjectProxy *location_proxy =
      bus_->GetObjectProxy(kServiceName, location_path);
  gclue_location_properties_ = std::make_unique<GeoClueLocationProperties>(
      location_proxy, kLocationInterfaceName,
      base::BindRepeating(&GeoClueProvider::OnLocationChanged,
                          weak_ptr_factory_.GetWeakPtr()));
  gclue_location_properties_->GetAll();
}

std::unique_ptr<LocationProvider> NewSystemLocationProvider(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    GeolocationManager *geolocation_manager) {
  return std::make_unique<GeoClueProvider>();
}

} // namespace device
