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
// constexpr char kGeoClueInterfaceName[] = "org.freedesktop.GeoClue2.Manager";
// constexpr char kGeoClueObjectPath[] = "/org/freedesktop/GeoClue2/Manager";
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
    const PropertyChangedCallback& callback)
    : dbus::PropertySet(proxy, interface_name, callback) {
  RegisterProperty("Latitude", &latitude);
  RegisterProperty("Longitude", &longitude);
  RegisterProperty("Accuracy", &accuracy);
  RegisterProperty("Altitude", &altitude);
  RegisterProperty("Speed", &speed);
  RegisterProperty("Heading", &heading);
}

GeoClueLocationProperties::~GeoClueLocationProperties() = default;

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

  started_ = true;
  dbus::ObjectProxy* proxy = bus_->GetObjectProxy(
      "org.freedesktop.GeoClue2",
      dbus::ObjectPath("/org/freedesktop/GeoClue2/Manager"));

  dbus::MethodCall call("org.freedesktop.GeoClue2.Manager", "GetClient");
  proxy->CallMethod(&call, 1000,
                    base::BindOnce(&GeoClueProvider::OnGetClientCompleted,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueProvider::StopProvider() {
  if (!started_) {
    LOG(ERROR) << "Stop: Not started!";
    return;
  }

  started_ = false;
  dbus::MethodCall stop("org.freedesktop.GeoClue2.Client", "Stop");
  gclue_client_->CallMethod(&stop, 1000, base::BindOnce([](dbus::Response* r) {
    LOG(ERROR) << "Stopped!";
  }));
}

const mojom::Geoposition& GeoClueProvider::GetPosition() {
  return last_position_;
}

void GeoClueProvider::OnPermissionGranted() {}

void GeoClueProvider::OnLocationChanged(const std::string& property_name) {
  LOG(ERROR) << "LocationChanged: " << property_name;
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

  // We should get this from the service too, as soon as I know what to do with
  // a variant...
  last_position_.timestamp = base::Time::Now();
  if (!device::ValidateGeoposition(last_position_)
    // SuperHacky: Don't read until we have a few values. Really, we should wait for the complete struct before we fire this event.
    || last_position_.longitude == 0) {
    // We might not have received everything yet..
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
  gclue_client_ = bus_->GetObjectProxy("org.freedesktop.GeoClue2", path);
  gclue_client_properties_ = std::make_unique<GeoClueProperties>(
      gclue_client_.get(), "org.freedesktop.GeoClue2.Client",
      base::NullCallback());

  gclue_client_properties_->desktop_id.Set(
      "com.brave.Browser", base::BindOnce(&GeoClueProvider::OnSetDesktopId,
                                          weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueProvider::OnSetDesktopId(bool success) {
  LOG(ERROR) << "Set DesktopId " << success << ". Starting";

  dbus::MethodCall start("org.freedesktop.GeoClue2.Client", "Start");
  gclue_client_->CallMethod(&start, 1000,
                            base::BindOnce(&GeoClueProvider::OnStarted,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueProvider::OnStarted(dbus::Response* response) {
  LOG(ERROR) << "Started";
  gclue_client_properties_->location.Get(
      base::BindOnce(&GeoClueProvider::OnGetLocationObjectPath,
                     weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueProvider::OnGetLocationObjectPath(bool success) {
  dbus::ObjectPath location_path = gclue_client_properties_->location.value();
  dbus::ObjectProxy* location_proxy =
      bus_->GetObjectProxy("org.freedesktop.GeoClue2", location_path);
  LOG(ERROR) << "Location Path: " << location_path.value();
  gclue_location_properties_ = std::make_unique<GeoClueLocationProperties>(
      location_proxy, "org.freedesktop.GeoClue2.Location",
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
