// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include </usr/include/libgeoclue-2.0/gclue-simple.h>

#include <gio/gcancellable.h>

#include "base/functional/bind.h"
#include "brave/services/device/geolocation/geoclue_location_provider.h"
#include "services/device/public/mojom/geoposition.mojom.h"
namespace device {

GeoClueProvider::GeoClueProvider() = default;
GeoClueProvider::~GeoClueProvider() = default;

void GeoClueProvider::SetUpdateCallback(
    const LocationProviderUpdateCallback& callback) {}

void GeoClueProvider::StartProvider(bool high_accuracy) {
  auto accuracy = high_accuracy ? GCLUE_ACCURACY_LEVEL_EXACT
                                : GCLUE_ACCURACY_LEVEL_NEIGHBORHOOD;

  GError* error = nullptr;                                
  gclue_simple_ = gclue_simple_new_sync("brave.desktop",
    accuracy,
    nullptr, 
    &error);

  CHECK(!error);

  auto* location = gclue_simple_get_location(gclue_simple_);
  last_position_.accuracy = gclue_location_get_accuracy(location);
  last_position_.altitude = gclue_location_get_altitude(location);
  last_position_.heading = gclue_location_get_heading(location);
  last_position_.latitude = gclue_location_get_latitude(location);
  last_position_.longitude = gclue_location_get_longitude(location);
  last_position_.speed = gclue_location_get_speed(location);
  
  // TODO: Work out how to get this from a GVariant.
  last_position_.timestamp = base::Time::Now();
}
void GeoClueProvider::StopProvider() {}

const mojom::Geoposition& GeoClueProvider::GetPosition() {
  return last_position_;
}

void GeoClueProvider::OnPermissionGranted() {}

}  // namespace device
