/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/test_ledger_client.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=UpholdUtilTest.*

using ::testing::_;

namespace brave_rewards::internal::uphold {

class UpholdUtilTest : public testing::Test {};

TEST_F(UpholdUtilTest, GetClientId) {
  _environment = mojom::Environment::PRODUCTION;
  ASSERT_EQ(GetClientId(), BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_ID));

  _environment = mojom::Environment::STAGING;
  ASSERT_EQ(GetClientId(), BUILDFLAG(UPHOLD_SANDBOX_CLIENT_ID));

  _environment = mojom::Environment::DEVELOPMENT;
  ASSERT_EQ(GetClientId(), BUILDFLAG(UPHOLD_SANDBOX_CLIENT_ID));
}

TEST_F(UpholdUtilTest, GetClientSecret) {
  _environment = mojom::Environment::PRODUCTION;
  ASSERT_EQ(GetClientSecret(), BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_SECRET));

  _environment = mojom::Environment::STAGING;
  ASSERT_EQ(GetClientSecret(), BUILDFLAG(UPHOLD_SANDBOX_CLIENT_SECRET));

  _environment = mojom::Environment::DEVELOPMENT;
  ASSERT_EQ(GetClientSecret(), BUILDFLAG(UPHOLD_SANDBOX_CLIENT_SECRET));
}

TEST_F(UpholdUtilTest, GetFeeAddress) {
  _environment = mojom::Environment::PRODUCTION;
  ASSERT_EQ(GetFeeAddress(), BUILDFLAG(UPHOLD_PRODUCTION_FEE_ADDRESS));

  _environment = mojom::Environment::STAGING;
  ASSERT_EQ(GetFeeAddress(), BUILDFLAG(UPHOLD_SANDBOX_FEE_ADDRESS));

  _environment = mojom::Environment::DEVELOPMENT;
  ASSERT_EQ(GetFeeAddress(), BUILDFLAG(UPHOLD_SANDBOX_FEE_ADDRESS));
}

TEST(UpholdUtilTest, GetServerUrl) {
  _environment = mojom::Environment::PRODUCTION;
  ASSERT_EQ(endpoint::uphold::GetServerUrl("/test"),
            base::StrCat({BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL), "/test"}));

  _environment = mojom::Environment::STAGING;
  ASSERT_EQ(endpoint::uphold::GetServerUrl("/test"),
            base::StrCat({BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL), "/test"}));

  _environment = mojom::Environment::DEVELOPMENT;
  ASSERT_EQ(endpoint::uphold::GetServerUrl("/test"),
            base::StrCat({BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL), "/test"}));
}

TEST_F(UpholdUtilTest, GetWallet) {
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;

  // no wallet
  ON_CALL(*mock_ledger_impl_.mock_client(),
          GetStringState(state::kWalletUphold, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run("");
      });
  auto result = mock_ledger_impl_.uphold()->GetWallet();
  ASSERT_TRUE(!result);

  ON_CALL(*mock_ledger_impl_.mock_client(),
          GetStringState(state::kWalletUphold, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::string wallet = FakeEncryption::Base64EncryptString(R"({
          "account_url":"https://wallet-sandbox.uphold.com/dashboard",
          "address":"2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af",
          "fees":{},
          "login_url":"https://wallet-sandbox.uphold.com/authorize/4c2b665ca060d",
          "one_time_string":"1F747AE0A708E47ED7E650BF1856B5A4EF7E36833BDB115",
          "status":2,
          "token":"4c80232r219c30cdf112208890a32c7e00",
          "user_name":"test"
        })");
        std::move(callback).Run(std::move(wallet));
      });

  // uphold wallet
  result = mock_ledger_impl_.uphold()->GetWallet();
  ASSERT_TRUE(result);
  ASSERT_EQ(result->address, "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af");
  ASSERT_EQ(result->user_name, "test");
  ASSERT_EQ(result->token, "4c80232r219c30cdf112208890a32c7e00");
  ASSERT_EQ(result->status, mojom::WalletStatus::kConnected);

  task_environment_.RunUntilIdle();
}

TEST_F(UpholdUtilTest, GenerateRandomHexString) {
  is_testing = true;
  auto result = util::GenerateRandomHexString();
  ASSERT_EQ(result, "123456789");

  is_testing = false;
  result = util::GenerateRandomHexString();
  ASSERT_EQ(result.length(), 64u);
}

TEST_F(UpholdUtilTest, GenerateLinks) {
  EXPECT_FALSE(uphold::GenerateLinks(nullptr));

  auto wallet = mojom::ExternalWallet::New();
  wallet->address = "123123123124234234234";
  wallet->one_time_string = "one_time_string";

  const auto account_url =
      base::StrCat({_environment == mojom::Environment::PRODUCTION
                        ? BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL)
                        : BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL),
                    "/ex/Home?login=1"});
  const auto activity_url =
      base::StrCat({_environment == mojom::Environment::PRODUCTION
                        ? BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL)
                        : BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL),
                    "/ja-jp/ex/tradehistory"});
  const auto login_url = base::StringPrintf(
      "%s/ex/OAuth/authorize"
      "?client_id=%s"
      "&scope="
      "assets "
      "create_deposit_id "
      "withdraw_to_deposit_id"
      "&redirect_uri=rewards://uphold/authorization"
      "&state="
      "&response_type=code"
      "&code_challenge_method=S256"
      "&code_challenge=47DEQpj8HBSa-_TImW-5JCeuQeRkm5NMpJWZG3hSuFU",
      _environment == mojom::Environment::PRODUCTION
          ? BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL)
          : BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL),
      _environment == mojom::Environment::PRODUCTION
          ? BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_ID)
          : BUILDFLAG(UPHOLD_SANDBOX_CLIENT_ID));

  _environment = mojom::Environment::PRODUCTION;
  wallet->status = mojom::WalletStatus::kNotConnected;
  auto result = GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->account_url, account_url);
  ASSERT_EQ(result->activity_url, activity_url);
  ASSERT_EQ(result->login_url, login_url);

  wallet->status = mojom::WalletStatus::kConnected;
  result = GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->account_url, account_url);
  ASSERT_EQ(result->activity_url, activity_url);
  ASSERT_EQ(result->login_url, login_url);

  wallet->status = mojom::WalletStatus::kLoggedOut;
  result = GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->account_url, account_url);
  ASSERT_EQ(result->activity_url, activity_url);
  ASSERT_EQ(result->login_url, login_url);
}

}  // namespace brave_rewards::internal::uphold
