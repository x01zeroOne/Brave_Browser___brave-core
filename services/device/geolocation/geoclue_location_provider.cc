// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <gio/gio.h>
#include <memory>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
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
}  // namespace

GeoClueProperties::GeoClueProperties(dbus::ObjectProxy* proxy,
                                     const std::string& interface_name,
                                     const PropertyChangedCallback& callback)
    : dbus::PropertySet(proxy, interface_name, callback) {
  RegisterProperty("DesktopId", &desktop_id);
  RegisterProperty("Location", &location);
}
GeoClueProperties::~GeoClueProperties() = default;

GeoClueLocationProperties::GeoClueLocationProperties(
    dbus::ObjectProxy* proxy,
    const std::string& interface_name,
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

void GeoClueLocationProperties::OnGetAll(dbus::Response* response) {
  dbus::PropertySet::OnGetAll(response);

  if (on_got_initial_values_) {
    std::move(on_got_initial_values_).Run();
  }
}

GeoClueProvider::GeoClueProvider() {
  DETACH_FROM_SEQUENCE(sequence_checker_);

  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  options.connection_type = dbus::Bus::SHARED;
  options.dbus_task_runner = dbus_thread_linux::GetTaskRunner();

  bus_ = base::MakeRefCounted<dbus::Bus>(options);
}
GeoClueProvider::~GeoClueProvider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // bus_->ShutdownAndBlock();
  // bus_.reset();
}

void GeoClueProvider::SetUpdateCallback(
    const LocationProviderUpdateCallback& callback) {
  location_update_callback_ = callback;
}

void GeoClueProvider::StartProvider(bool high_accuracy) {
  if (started_) {
    return;
  }
  LOG(ERROR) << "Starting: " << high_accuracy;
  started_ = true;
  dbus::ObjectProxy* proxy =
      bus_->GetObjectProxy(kServiceName, dbus::ObjectPath(kManagerObjectPath));

  dbus::MethodCall call(kManagerInterfaceName, "GetClient");
  proxy->CallMethod(&call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                    base::BindOnce(&GeoClueProvider::OnGetClientCompleted,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueProvider::StopProvider() {
  if (!started_) {
    LOG(ERROR) << "Stop: Not started!";
    return;
  }

  started_ = false;
  dbus::MethodCall stop(kClientInterfaceName, "Stop");
  gclue_client_->CallMethod(
      &stop, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::BindOnce([](dbus::Response* r) { LOG(ERROR) << "Stopped!"; }));
}

const mojom::Geoposition& GeoClueProvider::GetPosition() {
  return last_position_;
}

void GeoClueProvider::OnPermissionGranted() {}

void GeoClueProvider::OnLocationChanged() {
  last_position_ = mojom::Geoposition();
  last_position_.latitude = gclue_location_properties_->latitude.value();
  last_position_.longitude = gclue_location_properties_->longitude.value();
  last_position_.accuracy = gclue_location_properties_->accuracy.value();
  last_position_.altitude = gclue_location_properties_->altitude.value();
  last_position_.heading = gclue_location_properties_->heading.value();
  last_position_.speed = gclue_location_properties_->speed.value();
  last_position_.error_code = mojom::Geoposition::ErrorCode::NONE;

  LOG(ERROR) << "Lat: " << last_position_.latitude
             << ", Lng: " << last_position_.longitude
             << ", Accuracy: " << last_position_.accuracy
             << ", TS: " << last_position_.timestamp.ToJsTime();

  last_position_.timestamp = base::Time::Now();
  if (!device::ValidateGeoposition(last_position_)) {
    return;
  }
  location_update_callback_.Run(this, last_position_);
}

void GeoClueProvider::OnGetClientCompleted(dbus::Response* response) {
  if (!response) {
    LOG(ERROR) << "Failed to get GeoClue2 Client";
    return;
  }

  dbus::MessageReader reader(response);
  dbus::ObjectPath path;
  if (!reader.PopObjectPath(&path)) {
    LOG(ERROR) << "Failed to parse client path from response";
    return;
  }

  LOG(ERROR) << "Got Client: " << path.value();
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
    return;
  }
  dbus::MethodCall start(kClientInterfaceName, "Start");
  gclue_client_->CallMethod(&start, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                            base::BindOnce(&GeoClueProvider::OnStarted,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueProvider::OnStarted(dbus::Response* response) {
  LOG(ERROR) << "Started";
  gclue_client_->ConnectToSignal(
      kClientInterfaceName, "LocationUpdated",
      base::BindRepeating(
          [](base::WeakPtr<GeoClueProvider> provider, dbus::Signal* signal) {
            LOG(ERROR) << "Location Update :O";
            dbus::MessageReader reader(signal);
            dbus::ObjectPath old_location;
            reader.PopObjectPath(&old_location);
            dbus::ObjectPath new_location;
            reader.PopObjectPath(&new_location);

            if (provider) {
              provider->SetLocationPath(new_location);
            }

            LOG(ERROR) << "Old: " << old_location.value()
                       << ", New: " << new_location.value();
          },
          weak_ptr_factory_.GetWeakPtr()),
      base::DoNothing());
  gclue_client_->ConnectToSignal(
      kClientInterfaceName, "LocationUpdated",
      base::BindRepeating(
          [](base::WeakPtr<GeoClueProvider> provider, dbus::Signal* signal) {
            LOG(ERROR) << "Location Update :O";
            dbus::MessageReader reader(signal);
            dbus::ObjectPath old_location;
            reader.PopObjectPath(&old_location);
            dbus::ObjectPath new_location;
            reader.PopObjectPath(&new_location);

            if (provider) {
              provider->SetLocationPath(new_location);
            }

            LOG(ERROR) << "Old: " << old_location.value()
                       << ", New: " << new_location.value();
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

void GeoClueProvider::SetLocationPath(const dbus::ObjectPath& location_path) {
  dbus::ObjectProxy* location_proxy =
      bus_->GetObjectProxy(kServiceName, location_path);
  LOG(ERROR) << "Location Path: " << location_path.value();
  gclue_location_properties_ = std::make_unique<GeoClueLocationProperties>(
      location_proxy, kLocationInterfaceName,
      base::BindRepeating(&GeoClueProvider::OnLocationChanged,
                          weak_ptr_factory_.GetWeakPtr()));
  gclue_location_properties_->GetAll();
}

std::unique_ptr<LocationProvider> NewSystemLocationProvider(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    GeolocationManager* geolocation_manager) {
  return std::make_unique<GeoClueProvider>();
}

}  // namespace device
