/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_discovery_manager.h"

#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_observer_base.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_allowance_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

const char eth_allowance_detected_response[] = R"({
    "jsonrpc": "2.0",
    "id": 1,
    "result": [
        {
            "address": "0x3333333333333333333333333333333333333333",
            "blockHash": "0xaff41c269d9f87f9d71e826ccc612bec9eff33fe5f01a0c9b6f54bfaa8178686",
            "blockNumber": "0x101a7f1",
            "data": "0x0000000000000000000000000000000000000000000000000000000000000001",
            "logIndex": "0x92",
            "removed": false,
            "topics": [
                "0x8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925",
                "0x000000000000000000000000BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
                "0x000000000000000000000000dac308312e195710467ce36effe51ac7a4ecbf01"
            ],
            "transactionHash": "0x32132b285e95d82a9b81e3f25ea8290756f36c9fedd92af8290d4ee8cd1d7f98",
            "transactionIndex": "0x38"
        }
    ]
})";

const char eth_all_allowances_revoked_response[] = R"({
    "jsonrpc": "2.0",
    "id": 1,
    "result": [
    {
        "address": "0x3333333333333333333333333333333333333333",
        "blockHash": "0xaff41c269d9f87f9d71e826ccc612bec9eff33fe5f01a0c9b6f54bfaa8178686",
        "blockNumber": "0x101a7f1",
        "data": "0x0000000000000000000000000000000000000000000000000000000000000001",
        "logIndex": "0x92",
        "removed": false,
        "topics": [
            "0x8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925",
            "0x0000000000000000000000004444444444444444444444444444444444444444",
            "0x000000000000000000000000dac308312e195710467ce36effe51ac7a4ecbf01"
        ],
        "transactionHash": "0x32132b285e95d82a9b81e3f25ea8290756f36c9fedd92af8290d4ee8cd1d7f98",
        "transactionIndex": "0x38"
    },
    {
        "address": "0x3333333333333333333333333333333333333333",
        "blockHash": "0x12a48c50f513be08dd7b2b1514f79ee01e4c59c83852d6e9c4966d5dc283181e",
        "blockNumber": "0x10277f5",
        "data": "0x0000000000000000000000000000000000000000000000000000000000000000",
        "logIndex": "0x144",
        "removed": false,
        "topics": [
            "0x8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925",
            "0x0000000000000000000000004444444444444444444444444444444444444444",
            "0x000000000000000000000000dac308312e195710467ce36effe51ac7a4ecbf01"
        ],
        "transactionHash": "0xf40f833ced52120baeb3c505e444f73a130ed054e7b7ea453d3c207a773212c9",
        "transactionIndex": "0xb0"
    }]
})";

const std::vector<std::string>& GetAssetDiscoverySupportedEthChainsForTest() {
static base::NoDestructor<std::vector<std::string>>
    asset_discovery_supported_chains({mojom::kMainnetChainId,
                                      mojom::kPolygonMainnetChainId,
                                      mojom::kOptimismMainnetChainId});
return *asset_discovery_supported_chains;
}

const char kMnemonic1[] =
    "divide cruise upon flag harsh carbon filter merit once advice bright "
    "drive";
const char kPasswordBrave[] = "brave";

}  // namespace

class TestBraveWalletServiceObserverForAssetDiscoveryManager
    : public brave_wallet::BraveWalletServiceObserverBase {
 public:
  TestBraveWalletServiceObserverForAssetDiscoveryManager() = default;

  void OnDiscoverAssetsStarted() override {
    on_discover_assets_started_fired_ = true;
  }

  void OnDiscoverAssetsCompleted(
      std::vector<mojom::BlockchainTokenPtr> discovered_assets) override {
    ASSERT_EQ(expected_contract_addresses_.size(), discovered_assets.size());
    for (size_t i = 0; i < discovered_assets.size(); i++) {
      EXPECT_EQ(expected_contract_addresses_[i],
                discovered_assets[i]->contract_address);
    }
    on_discover_assets_completed_fired_ = true;
    run_loop_asset_discovery_->Quit();
  }
  void OnDiscoverEthAllowancesCompleted(
      std::vector<mojom::AllowanceInfoPtr> allowances) override {
    for (auto& allowance : allowances) {
      ASSERT_TRUE(std::find(expected_contract_addresses_.begin(),
                            expected_contract_addresses_.end(),
                            allowance->contract_address) !=
                  expected_contract_addresses_.end());
    }

    ASSERT_EQ(expected_allowances_count, allowances.size());

    on_discover_eth_allowance_completed_count_++;

    if (run_loop_asset_discovery_) {
      run_loop_asset_discovery_->Quit();
    }
  }

  bool OnDiscoverEthAllowancesCompletedFired() {
    return on_discover_eth_allowance_completed_count_ > 0;
  }

  void WaitForOnDiscoverAssetsCompleted(
      const std::vector<std::string>& addresses) {
    expected_contract_addresses_ = addresses;
    run_loop_asset_discovery_ = std::make_unique<base::RunLoop>();
    run_loop_asset_discovery_->Run();
  }

  bool OnDiscoverAssetsStartedFired() {
    return on_discover_assets_started_fired_;
  }

  bool OnDiscoverAssetsCompletedFired() {
    return on_discover_assets_completed_fired_;
  }

  void WaitForOnDiscoverEthAllowancesCompleted(
      const std::vector<std::string>& addresses,
      const std::size_t& expected_allowances_count_val) {
    expected_allowances_count = expected_allowances_count_val;
    expected_contract_addresses_ = addresses;
    run_loop_asset_discovery_ = std::make_unique<base::RunLoop>();
    run_loop_asset_discovery_->Run();

    // make sure that we ignore all DiscoverEthAllowancesOnAllSupportedChains
    // calls
    //  while we are waiting for the callback:OnDiscoverEthAllowancesCompleted
    ASSERT_EQ(1, on_discover_eth_allowance_completed_count_);
  }

  mojo::PendingRemote<brave_wallet::mojom::BraveWalletServiceObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }
  void Reset() {
    expected_contract_addresses_.clear();
    on_discover_assets_started_fired_ = false;
    on_discover_assets_completed_fired_ = false;
    on_discover_eth_allowance_completed_count_ = 0;
  }

 private:
  std::unique_ptr<base::RunLoop> run_loop_asset_discovery_;
  std::vector<std::string> expected_contract_addresses_;
  bool on_discover_assets_started_fired_ = false;
  std::size_t expected_allowances_count{0};
  bool on_discover_assets_completed_fired_ = false;

  int on_discover_eth_allowance_completed_count_{0};
  mojo::Receiver<brave_wallet::mojom::BraveWalletServiceObserver>
      observer_receiver_{this};
};

class AssetDiscoveryManagerUnitTest : public testing::Test {
 public:
  AssetDiscoveryManagerUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)),
        task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~AssetDiscoveryManagerUnitTest() override = default;

 protected:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(
        features::kNativeBraveWalletFeature);

    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());
    keyring_service_ =
        KeyringServiceFactory::GetServiceForContext(profile_.get());
    json_rpc_service_ =
        JsonRpcServiceFactory::GetServiceForContext(profile_.get());
    json_rpc_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
    tx_service = TxServiceFactory::GetServiceForContext(profile_.get());
    wallet_service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_,
        BraveWalletServiceDelegate::Create(profile_.get()), keyring_service_,
        json_rpc_service_, tx_service, GetPrefs(), GetLocalState());
    asset_discovery_manager_ = std::make_unique<AssetDiscoveryManager>(
        shared_url_loader_factory_, wallet_service_.get(), json_rpc_service_,
        keyring_service_, GetPrefs());
    wallet_service_observer_ =
        std::make_unique<TestBraveWalletServiceObserverForAssetDiscoveryManager>();
    wallet_service_->AddObserver(wallet_service_observer_->GetReceiver());
  }

  void TestDiscoverAssetsOnAllSupportedChains(
      const std::map<mojom::CoinType, std::vector<std::string>>&
          account_addresses,
      bool triggered_by_accounts_added,
      bool expect_events_fired,
      const std::vector<std::string>& expected_token_contract_addresses,
      size_t expected_ending_queue_size = 0u) {
    asset_discovery_manager_->DiscoverAssetsOnAllSupportedChains(
        account_addresses, triggered_by_accounts_added);
    if (expect_events_fired) {
      wallet_service_observer_->WaitForOnDiscoverAssetsCompleted(
          expected_token_contract_addresses);
      EXPECT_TRUE(wallet_service_observer_->OnDiscoverAssetsStartedFired());
      EXPECT_TRUE(wallet_service_observer_->OnDiscoverAssetsCompletedFired());
    } else {
      base::RunLoop().RunUntilIdle();
      EXPECT_FALSE(wallet_service_observer_->OnDiscoverAssetsStartedFired());
      EXPECT_FALSE(wallet_service_observer_->OnDiscoverAssetsCompletedFired());
    }
    EXPECT_EQ(asset_discovery_manager_->GetQueueSizeForTesting(),
              expected_ending_queue_size);
    wallet_service_observer_->Reset();
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }
  TestingPrefServiceSimple* GetLocalState() { return local_state_->Get(); }
  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return brave_wallet::GetNetworkURL(GetPrefs(), chain_id, coin);
  }
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<TestBraveWalletServiceObserverForAssetDiscoveryManager>
      wallet_service_observer_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<BraveWalletService> wallet_service_;
  std::unique_ptr<AssetDiscoveryManager> asset_discovery_manager_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  JsonRpcService* json_rpc_service_;
  TxService* tx_service;
  base::test::ScopedFeatureList scoped_feature_list_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

class EthAllowanceManagerUnitTest : public testing::Test {
 public:
  EthAllowanceManagerUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)),
        task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~EthAllowanceManagerUnitTest() override = default;

 protected:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(
        features::kNativeBraveWalletFeature);

    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());
    keyring_service_ =
        KeyringServiceFactory::GetServiceForContext(profile_.get());
    json_rpc_service_ =
        JsonRpcServiceFactory::GetServiceForContext(profile_.get());
    json_rpc_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
    tx_service = TxServiceFactory::GetServiceForContext(profile_.get());
    wallet_service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_,
        BraveWalletServiceDelegate::Create(profile_.get()), keyring_service_,
        json_rpc_service_, tx_service, GetPrefs(), GetLocalState());
    eth_allowance_manager_ = std::make_unique<EthAllowanceManager>(
        wallet_service_.get(), json_rpc_service_, keyring_service_, GetPrefs());
    eth_allowance_manager_->SetSupportedChainsForTesting(
        GetAssetDiscoverySupportedEthChainsForTest());
    wallet_service_observer_ =
        std::make_unique<TestBraveWalletServiceObserverForAssetDiscoveryManager>();
    wallet_service_->AddObserver(wallet_service_observer_->GetReceiver());
  }

  void TestAllowancesLoading(
      const std::string& token_list_json,
      const std::map<GURL, std::map<std::string, std::string>>& requests,
      const int& expected_allowances_count,
      const std::size_t& eth_allowance_completed_call_count) {
    auto* blockchain_registry = BlockchainRegistry::GetInstance();
    TokenListMap token_list_map;

    eth_allowance_manager_->SetSupportedChainsForTesting(
        {mojom::kMainnetChainId, mojom::kPolygonMainnetChainId});

    ASSERT_TRUE(
        ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));

    std::vector<std::string> contract_addresses;
    for (auto const& [contract_addr, token_info] : token_list_map) {
      for (auto const& tkn : token_info) {
        contract_addresses.push_back(tkn->contract_address);
      }
    }
    blockchain_registry->UpdateTokenList(std::move(token_list_map));
    keyring_service_->RestoreWallet(kMnemonic1, kPasswordBrave, false,
                                    base::DoNothing());

    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, requests](const network::ResourceRequest& request) {
          for (auto const& [url, address_response_map] : requests) {
            if (request.url.spec().find("nfts") != std::string::npos) {
              continue;
            }
            std::string header_value;
            EXPECT_TRUE(
                request.headers.GetHeader("X-Eth-Method", &header_value));

            if (request.url.spec() == url.spec() &&
                header_value == "eth_getLogs") {
              base::StringPiece request_string(
                  request.request_body->elements()
                      ->at(0)
                      .As<network::DataElementBytes>()
                      .AsStringPiece());
              std::string response;
              for (auto const& [address, potential_response] :
                   address_response_map) {
                if (request_string.find(address.substr(2)) !=
                    std::string::npos) {
                  response = potential_response;
                  break;
                }
              }
              ASSERT_FALSE(response.empty());
              url_loader_factory_.ClearResponses();
              url_loader_factory_.AddResponse(request.url.spec(), response);
            }
          }
        }));

    std::size_t call_count{0};
    while (eth_allowance_completed_call_count > call_count++) {
      eth_allowance_manager_->DiscoverEthAllowancesOnAllSupportedChains();
    }

    wallet_service_observer_->WaitForOnDiscoverEthAllowancesCompleted(
        contract_addresses, expected_allowances_count);
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }
  TestingPrefServiceSimple* GetLocalState() { return local_state_->Get(); }
  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return brave_wallet::GetNetworkURL(GetPrefs(), chain_id, coin);
  }

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<TestBraveWalletServiceObserverForAssetDiscoveryManager>
      wallet_service_observer_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<BraveWalletService> wallet_service_;
  std::unique_ptr<EthAllowanceManager> eth_allowance_manager_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  JsonRpcService* json_rpc_service_;
  TxService* tx_service;
  base::test::ScopedFeatureList scoped_feature_list_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(EthAllowanceManagerUnitTest, AllowancesLoading) {
  const std::string token_list_json = R"({
        "0x3333333333333333333333333333333333333333": {
          "name": "3333",
          "logo": "333.svg",
          "erc20": true,
          "symbol": "333",
          "decimals": 18,
          "chainId": "0x1"
        }
      })";

  std::map<GURL, std::map<std::string, std::string>> requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {{"0x3333333333333333333333333333333333333333",
         eth_allowance_detected_response}}},
  };

  TestAllowancesLoading(token_list_json, requests, 1, 3);

  EXPECT_TRUE(
      wallet_service_observer_->OnDiscoverEthAllowancesCompletedFired());

  auto& allowance_cashe_dict =
      GetPrefs()->GetDict(kBraveWalletEthAllowancesCache);
  const auto* chain_id_dict = allowance_cashe_dict.FindDict("0x1");
  ASSERT_TRUE(nullptr != chain_id_dict);
  const auto* last_block_number_ptr =
      chain_id_dict->FindString("last_block_number");
  ASSERT_TRUE(nullptr != last_block_number_ptr);
  EXPECT_EQ(*last_block_number_ptr, "0x101a7f1");
  const auto* allowances_found_ptr =
      chain_id_dict->FindList("allowances_found");
  ASSERT_TRUE(nullptr != allowances_found_ptr);
  EXPECT_EQ(allowances_found_ptr->size(), (size_t)1);
}

TEST_F(EthAllowanceManagerUnitTest, NoAllowancesLoaded) {
  const std::string token_list_json = R"({
        "0x3333333333333333333333333333333333333333": {
          "name": "3333",
          "logo": "333.svg",
          "erc20": true,
          "symbol": "333",
          "decimals": 18,
          "chainId": "0x1"
        }
      })";

  std::map<GURL, std::map<std::string, std::string>> requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {{"0x3333333333333333333333333333333333333333",
         eth_all_allowances_revoked_response}}},
  };

  TestAllowancesLoading(token_list_json, requests, 0, 4);

  EXPECT_TRUE(
      wallet_service_observer_->OnDiscoverEthAllowancesCompletedFired());

  ASSERT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletEthAllowancesCache));
  EXPECT_EQ(GetPrefs()->GetDict(kBraveWalletEthAllowancesCache).size(),
            (size_t)0);
}

TEST_F(EthAllowanceManagerUnitTest, NoAllowancesLoadedForSkippedNetwork) {
  const std::string token_list_json = R"({
        "0x3333333333333333333333333333333333333333": {
          "name": "3333",
          "logo": "333.svg",
          "erc20": true,
          "symbol": "333",
          "decimals": 18,
          "chainId": "0x1"
        }
      })";

  std::map<GURL, std::map<std::string, std::string>> requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {{"0x3333333333333333333333333333333333333333",
         R"({
   "error": {
      "code": -32000,
      "message": "requested too many blocks from 0 to 27842567,
      maximum is set to 2048"
   },
   "id": 1,
   "jsonrpc": "2.0"
})"}}},
  };

  TestAllowancesLoading(token_list_json, requests, 0, 5);

  EXPECT_TRUE(
      wallet_service_observer_->OnDiscoverEthAllowancesCompletedFired());

  ASSERT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletEthAllowancesCache));
  EXPECT_EQ(GetPrefs()->GetDict(kBraveWalletEthAllowancesCache).size(),
            (size_t)0);
}

TEST_F(AssetDiscoveryManagerUnitTest, GetAssetDiscoverySupportedChains) {
  // GetAssetDiscoverySupportedChains should return a map of the same size
  // vectors every time
  const std::map<mojom::CoinType, std::vector<std::string>> chains1 =
      asset_discovery_manager_->GetAssetDiscoverySupportedChains();
  const std::map<mojom::CoinType, std::vector<std::string>> chains2 =
      asset_discovery_manager_->GetAssetDiscoverySupportedChains();
  const std::map<mojom::CoinType, std::vector<std::string>> chains3 =
      asset_discovery_manager_->GetAssetDiscoverySupportedChains();
  EXPECT_GT(chains1.at(mojom::CoinType::ETH).size(), 0u);
  EXPECT_GT(chains1.at(mojom::CoinType::SOL).size(), 0u);

  EXPECT_EQ(chains2.at(mojom::CoinType::ETH).size(),
            chains1.at(mojom::CoinType::ETH).size());
  EXPECT_EQ(chains2.at(mojom::CoinType::SOL).size(),
            chains1.at(mojom::CoinType::SOL).size());
}

TEST_F(AssetDiscoveryManagerUnitTest, DiscoverAssetsOnAllSupportedChains) {
  // Verify OnDiscoverAssetsStarted and OnDiscoverAssetsCompleted both fire and
  // kBraveWalletLastDiscoveredAssetsAt does not update if accounts_added set
  base::Time current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  ASSERT_EQ(current_assets_last_discovered_at, base::Time());
  TestDiscoverAssetsOnAllSupportedChains({}, true, true, {});
  base::Time previous_assets_last_discovered_at =
      current_assets_last_discovered_at;
  current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  EXPECT_EQ(current_assets_last_discovered_at,
            previous_assets_last_discovered_at);

  // Verify OnDiscoverAssetsStarted and OnDiscoverAssetsCompleted both fire and
  // kBraveWalletLastDiscoveredAssetsAt updates if accounts_added not set
  TestDiscoverAssetsOnAllSupportedChains({}, false, true, {});
  previous_assets_last_discovered_at = current_assets_last_discovered_at;
  current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  EXPECT_GT(current_assets_last_discovered_at,
            previous_assets_last_discovered_at);

  // Verify subsequent requests are throttled if within the rate limiting window
  // and accounts_added not set
  TestDiscoverAssetsOnAllSupportedChains({}, false, false, {});
  previous_assets_last_discovered_at = current_assets_last_discovered_at;
  current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  EXPECT_EQ(current_assets_last_discovered_at,
            previous_assets_last_discovered_at);

  // Verify subsequent requests are not throttled if within the rate limiting
  // window and accounts added set
  TestDiscoverAssetsOnAllSupportedChains({}, true, true, {});

  // Verify once the rate limiting window has passed, subsequent requests are
  // not rate limited if accounts added not set
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));
  TestDiscoverAssetsOnAllSupportedChains({}, false, true, {});
  previous_assets_last_discovered_at = current_assets_last_discovered_at;
  current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  EXPECT_GT(current_assets_last_discovered_at,
            previous_assets_last_discovered_at);

  // If there is already a task in-flight, OnDiscoverAssetsStarted and
  // OnDiscoverAssetsCompleted both fire if accounts_added is set.
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));
  std::queue<std::unique_ptr<AssetDiscoveryTask>> tasks;
  tasks.push(
      std::make_unique<AssetDiscoveryTask>(nullptr, nullptr, nullptr, nullptr));
  asset_discovery_manager_->SetQueueForTesting(std::move(tasks));
  TestDiscoverAssetsOnAllSupportedChains({}, true, true, {}, 1);

  // If there is already a task in-flight, nothing is run
  // if accounts_added not set.
  TestDiscoverAssetsOnAllSupportedChains({}, false, false, {}, 1);
}

// KeyringServiceObserver test
TEST_F(AssetDiscoveryManagerUnitTest, AccountsAdded) {
  // Verifies that the AssetDiscoveryManager is added as an observer to the
  // KeyringService, and that discovery is run when new accounts are added
  base::Time current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  ASSERT_EQ(current_assets_last_discovered_at, base::Time());
  keyring_service_->RestoreWallet(kMnemonic1, kPasswordBrave, false,
                                  base::DoNothing());
  wallet_service_observer_->WaitForOnDiscoverAssetsCompleted({});
  base::Time previous_assets_last_discovered_at =
      current_assets_last_discovered_at;
  current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  EXPECT_TRUE(wallet_service_observer_->OnDiscoverAssetsStartedFired());
  EXPECT_TRUE(wallet_service_observer_->OnDiscoverAssetsCompletedFired());
  EXPECT_EQ(current_assets_last_discovered_at,
            previous_assets_last_discovered_at);
  wallet_service_observer_->Reset();
}

}  // namespace brave_wallet
