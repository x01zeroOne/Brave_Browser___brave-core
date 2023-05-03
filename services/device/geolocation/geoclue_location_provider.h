// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_
#define BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_

#include <gio/gio.h>
#include <cstddef>
#include <memory>
#include <string>

#include "base/allocator/partition_allocator/pointers/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "dbus/property.h"
#include "services/device/public/cpp/geolocation/geolocation_manager.h"
#include "services/device/public/cpp/geolocation/location_provider.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace device {

struct GeoClueProperties : public dbus::PropertySet {
  dbus::Property<std::string> desktop_id;
  dbus::Property<dbus::ObjectPath> location;

  GeoClueProperties(dbus::ObjectProxy* proxy,
                    const std::string& interface_name,
                    const PropertyChangedCallback& callback);
  ~GeoClueProperties() override;
};

struct GeoClueLocationProperties : public dbus::PropertySet {
  dbus::Property<double> latitude;
  dbus::Property<double> longitude;
  dbus::Property<double> accuracy;
  dbus::Property<double> altitude;
  dbus::Property<double> speed;
  dbus::Property<double> heading;

  GeoClueLocationProperties(dbus::ObjectProxy* proxy,
                            const std::string& interface_name,
                            const PropertyChangedCallback& callback);
  ~GeoClueLocationProperties() override;
};

class GeoClueProvider : public LocationProvider {
 public:
  GeoClueProvider();

  GeoClueProvider(const GeoClueProvider&) = delete;
  GeoClueProvider& operator=(const GeoClueProvider&) = delete;

  ~GeoClueProvider() override;

  // LocationProvider:
  void SetUpdateCallback(
      const LocationProviderUpdateCallback& callback) override;
  void StartProvider(bool high_accuracy) override;
  void StopProvider() override;
  const mojom::Geoposition& GetPosition() override;
  void OnPermissionGranted() override;

 private:
  void OnGetClientCompleted(dbus::Response* response);
  void OnSetDesktopId(bool success);
  void OnStarted(dbus::Response* response);
  void OnGetLocationObjectPath(bool success);

  void OnLocationChanged(const std::string& property_name);

  SEQUENCE_CHECKER(sequence_checker_);

  bool started_ = false;

  scoped_refptr<dbus::Bus> bus_;
  scoped_refptr<dbus::ObjectProxy> gclue_client_;
  std::unique_ptr<GeoClueProperties> gclue_client_properties_;

  std::unique_ptr<GeoClueLocationProperties> gclue_location_properties_;

  mojom::Geoposition last_position_;
  LocationProviderUpdateCallback location_update_callback_;

  base::WeakPtrFactory<GeoClueProvider> weak_ptr_factory_{this};
};

}  // namespace device

#endif  // BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_
