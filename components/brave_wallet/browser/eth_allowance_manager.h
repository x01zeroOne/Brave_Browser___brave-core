/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_ALLOWANCE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_ALLOWANCE_MANAGER_H_

#include <string>
#include <tuple>
#include <vector>

#include "base/barrier_callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

class PrefService;

namespace brave_wallet {

class BraveWalletService;
class JsonRpcService;
class KeyringService;

class EthAllowanceManager {
 public:
  EthAllowanceManager(BraveWalletService* wallet_service,
                      JsonRpcService* json_rpc_service,
                      KeyringService* keyring_service,
                      PrefService* prefs);
  EthAllowanceManager(const EthAllowanceManager&) = delete;
  auto& operator=(const EthAllowanceManager&) = delete;
  EthAllowanceManager(const EthAllowanceManager&&) = delete;
  auto& operator=(const EthAllowanceManager&&) = delete;

  ~EthAllowanceManager();

  void DiscoverEthAllowancesOnAllSupportedChains();

  void SetSupportedChainsForTesting(
      const std::vector<std::string>& supported_chains_for_testing) {
    supported_chains_for_testing_ = supported_chains_for_testing;
  }

 private:
  friend class EthAllowanceManagerUnitTest;
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest, AllowancesLoading);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest, NoAllowancesLoaded);
  FRIEND_TEST_ALL_PREFIXES(EthAllowanceManagerUnitTest,
                           NoAllowancesLoadedForSkippedNetwork);

  void OnGetAllowances(
      base::OnceCallback<
          void(base::flat_map<
               std::string,
               std::tuple<uint256_t, std::vector<mojom::AllowanceInfoPtr>>>)>
          barrier_callback,
      const std::string& chain_id,
      const std::string& hex_account_address,
      [[maybe_unused]] const std::vector<Log>& logs,
      base::Value rawlogs,
      mojom::ProviderError error,
      const std::string& error_message);
  void MergeEthAllowances(
      const std::vector<base::flat_map<
          std::string,
          std::tuple<uint256_t, std::vector<mojom::AllowanceInfoPtr>>>>&
          discovered_allowance_results);
  void CompleteDiscovedEthAllowance(
      const std::vector<mojom::AllowanceInfoPtr>& allowances);
  std::vector<std::string> supported_chains_for_testing_;

  void LoadCachedAllowances(
      const std::string& chain_id,
      const std::string& hex_account_address,
      base::flat_map<std::string,
                     std::tuple<brave_wallet::uint256_t,
                                mojom::AllowanceInfoPtr>>& allowances_map);

  static auto GetAllowanceMapKey(const std::string& contract_address,
                                 const std::string& approver_addr,
                                 const std::string& spender_address);

  bool is_eth_allowance_discovering_running_{false};
  raw_ptr<BraveWalletService> wallet_service_;
  raw_ptr<JsonRpcService> json_rpc_service_;
  raw_ptr<KeyringService> keyring_service_;
  raw_ptr<PrefService> prefs_;

  base::WeakPtrFactory<EthAllowanceManager> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_ALLOWANCE_MANAGER_H_
