// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/task_environment.h"
#include "brave/services/device/geolocation/geoclue_location_provider.h"

#include "content/public/test/browser_task_environment.h"
#include "services/device/public/mojom/geoposition.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "gtest/gtest.h"
#include <memory>

namespace device {

class TestGeoclueLocationProvider : public GeoClueProvider {
public:
  TestGeoclueLocationProvider() = default;
  ~TestGeoclueLocationProvider() override = default;

  bool HasPermission() { return permission_granted_; }

  bool Started() { return started_; }

  void SetPositionForTesting(const mojom::Geoposition &position) {
    SetLocation(position);
  }

private:
  mojom::Geoposition position_;
};

class GeoclueLocationProviderTest : public testing::Test {
public:
  GeoclueLocationProviderTest() = default;
  ~GeoclueLocationProviderTest() override = default;

  void InitializeProvider() {
    provider_ = std::make_unique<TestGeoclueLocationProvider>();
    provider_->SetUpdateCallback(base::BindRepeating(
        [](GeoclueLocationProviderTest *test, const LocationProvider *provider,
           const mojom::Geoposition &position) { test->loop_.Quit(); },
        base::Unretained(this)));
  }

protected:
  content::BrowserTaskEnvironment task_environment_;
  base::RunLoop loop_;

  std::unique_ptr<TestGeoclueLocationProvider> provider_;
};

TEST_F(GeoclueLocationProviderTest,
       CreateDestroy) { // should not crash
  InitializeProvider();
  EXPECT_TRUE(provider_);
  provider_.reset();
}

TEST_F(GeoclueLocationProviderTest, OnPermissionGranted) {
  InitializeProvider();
  EXPECT_FALSE(provider_->HasPermission());
  provider_->OnPermissionGranted();
  EXPECT_TRUE(provider_->HasPermission());
}

TEST_F(GeoclueLocationProviderTest, CanStart) {
  InitializeProvider();
  EXPECT_FALSE(provider_->Started());
  provider_->StartProvider(false);
  EXPECT_TRUE(provider_->Started());
}

TEST_F(GeoclueLocationProviderTest, CanStop) {
  InitializeProvider();
  EXPECT_FALSE(provider_->Started());

  // Shouldn't crash, even though we haven't started.
  provider_->StopProvider();

  EXPECT_FALSE(provider_->Started());

  provider_->StartProvider(true);
  EXPECT_TRUE(provider_->Started());

  provider_->StopProvider();
  EXPECT_FALSE(provider_->Started());

  // Shouldn't crash calling stop a second time, after having started.
  provider_->StopProvider();
  EXPECT_FALSE(provider_->Started());
}

TEST_F(GeoclueLocationProviderTest, NoLocationUntilPermissionGranted) {}

} // namespace device
