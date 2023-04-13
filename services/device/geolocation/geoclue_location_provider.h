// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_
#define BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_

#include <gio/gio.h>

#include "base/memory/weak_ptr.h"
#include "services/device/public/cpp/geolocation/geolocation_manager.h"
#include "services/device/public/cpp/geolocation/location_provider.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace device {

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

 protected:
  void OnGeoClueInstanceReady(GObject* source,
                                GAsyncResult* result,
                                gpointer user_data);

 private:
  GClueSimple* gclue_simple_;

  mojom::Geoposition last_position_;
  LocationProviderUpdateCallback location_update_callback_;

  base::WeakPtrFactory<GeoClueProvider> weak_ptr_factory_{this};
};

}  // namespace device

#endif  // BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_
