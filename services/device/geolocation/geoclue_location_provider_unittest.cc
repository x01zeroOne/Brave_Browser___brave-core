// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/device/geolocation/geoclue_location_provider.h"

#include <cmath>
#include <memory>

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "content/public/test/browser_task_environment.h"
#include "services/device/public/mojom/geoposition.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "gtest/gtest.h"

namespace device {

class TestGeoclueLocationProvider : public GeoClueProvider {
public:
  TestGeoclueLocationProvider() = default;
  ~TestGeoclueLocationProvider() override = default;

  bool HasPermission() { return permission_granted_; }

  bool Started() { return client_state_ != kStopped; }

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
           const mojom::Geoposition &position) {
          test->loop_.Quit();
          test->update_count_++;
        },
        base::Unretained(this)));
  }

protected:
  content::BrowserTaskEnvironment task_environment_;
  base::RunLoop loop_;
  int update_count_ = 0;

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

TEST_F(GeoclueLocationProviderTest, CanStopPermissionGranted) {
  InitializeProvider();
  EXPECT_FALSE(provider_->Started());

  provider_->OnPermissionGranted();
  provider_->StopProvider();

  EXPECT_FALSE(provider_->Started());
  EXPECT_TRUE(provider_->HasPermission());
}

TEST_F(GeoclueLocationProviderTest, CanStopStartedAndPermissionGranted) {
  InitializeProvider();

  provider_->OnPermissionGranted();
  provider_->StartProvider(false);

  // Let everything initialize until we get a location
  loop_.Run();

  EXPECT_EQ(1, update_count_);
  EXPECT_TRUE(provider_->Started());
  EXPECT_TRUE(provider_->HasPermission());

  // After stopping, further updates should no propagate.
  provider_->StopProvider();

  mojom::Geoposition fake_position;
  fake_position.latitude = 0;
  fake_position.longitude = 0;
  fake_position.accuracy = 1;
  fake_position.timestamp = base::Time::Now();
  fake_position.error_code = mojom::Geoposition_ErrorCode::NONE;
  provider_->SetPositionForTesting(fake_position);

  EXPECT_EQ(1, update_count_);
}

TEST_F(GeoclueLocationProviderTest, NoLocationUntilPermissionGranted) {
  InitializeProvider();
  EXPECT_FALSE(provider_->Started());
  EXPECT_FALSE(provider_->HasPermission());
  EXPECT_EQ(0, update_count_);

  provider_->StartProvider(false);
  EXPECT_TRUE(provider_->Started());
  EXPECT_FALSE(provider_->HasPermission());
  EXPECT_EQ(0, update_count_);

  mojom::Geoposition fake_position;
  fake_position.latitude = 0;
  fake_position.longitude = 0;
  fake_position.accuracy = 1;
  fake_position.timestamp = base::Time::Now();
  fake_position.error_code = mojom::Geoposition_ErrorCode::NONE;

  provider_->SetPositionForTesting(fake_position);
  EXPECT_EQ(0, update_count_);

  provider_->OnPermissionGranted();

  // Wait for the client to initialize.
  loop_.Run();
  EXPECT_EQ(1, update_count_);

  fake_position.latitude = 1;
  provider_->SetPositionForTesting(fake_position);
  EXPECT_EQ(2, update_count_);
}

TEST_F(GeoclueLocationProviderTest, GetsLocation) {
  InitializeProvider();
  provider_->StartProvider(false);
  provider_->OnPermissionGranted();

  loop_.Run();
  EXPECT_EQ(1, update_count_);

  EXPECT_LE(provider_->GetPosition().latitude, 90);
  EXPECT_GE(provider_->GetPosition().latitude, -90);
  EXPECT_LE(provider_->GetPosition().longitude, 180);
  EXPECT_GE(provider_->GetPosition().longitude, -180);
  EXPECT_GE(provider_->GetPosition().accuracy, 0);
  EXPECT_FALSE(provider_->GetPosition().timestamp.is_null());
}

} // namespace device
