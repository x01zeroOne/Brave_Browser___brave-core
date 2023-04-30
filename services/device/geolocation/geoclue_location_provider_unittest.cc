// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/device/geolocation/geoclue_location_provider.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace device {

TEST(GeoclueLocationProviderTest,
     CanInitializeNewLocationProvider) { // should not crash
  auto provider = GeoClueProvider();
}

TEST(GeoclueLocationProviderTest, DBusErrorIsGeolocationError) {}

TEST(GeoclueLocationProviderTest, CanStart) {}
TEST(GeoclueLocationProviderTest, CanStop) {}

TEST(GeoclueLocationProviderTest, NoLocationUntilPermissionGranted) {}

} // namespace device
