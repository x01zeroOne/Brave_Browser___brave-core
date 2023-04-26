/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_allowance_manager.h"

#include <algorithm>
#include <string>
#include <tuple>
#include <utility>

#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

EthAllowanceManager::EthAllowanceManager(BraveWalletService* wallet_service,
                                         JsonRpcService* json_rpc_service,
                                         KeyringService* keyring_service,
                                         PrefService* prefs)
    : wallet_service_(wallet_service),
      json_rpc_service_(json_rpc_service),
      keyring_service_(keyring_service),
      prefs_(prefs),
      weak_ptr_factory_(this) {}

EthAllowanceManager::~EthAllowanceManager() = default;

void EthAllowanceManager::DiscoverEthAllowancesOnAllSupportedChains() {
  if (is_eth_allowance_discovering_running_) {
    return;
  }
  is_eth_allowance_discovering_running_ = true;

  const auto keyring_info = keyring_service_->GetKeyringInfoSync(
      brave_wallet::mojom::kDefaultKeyringId);
  std::vector<std::string> account_addresses;
  for (const auto& account_info : keyring_info->account_infos) {
    account_addresses.push_back(account_info->address);
  }

  if (account_addresses.empty()) {
    return;
  }

  const auto token_list_map =
      BlockchainRegistry::GetInstance()->GetEthTokenListMap(
          GetChainIdsForAlowanceDiscovering());

  base::flat_map<std::string, base::Value::List> chain_id_to_contract_addresses;

  for (const auto& [chain_id, token_list] : token_list_map) {
    for (const auto& token : token_list) {
      chain_id_to_contract_addresses[chain_id].Append(token->contract_address);
    }
  }

  if (chain_id_to_contract_addresses.empty()) {
    return;
  }
  const auto& allowance_cashe_dict =
      prefs_->GetDict(kBraveWalletEthAllowancesCache);

  // Use a barrier callback to wait for all EthGetLogs calls to
  // complete (one for each chain_id).
  const auto barrier_callback = base::BarrierCallback<base::flat_map<
      std::string,
      std::tuple<uint256_t, std::vector<mojom::AllowanceInfoPtr>>>>(
      account_addresses.size() * chain_id_to_contract_addresses.size(),
      base::BindOnce(&EthAllowanceManager::MergeEthAllowances,
                     weak_ptr_factory_.GetWeakPtr()));

  const auto approval_topic_hash =
      KeccakHash("Approval(address,address,uint256)");
  for (const auto& account_address : account_addresses) {
    for (auto& [chain_id, contract_addresses] :
         chain_id_to_contract_addresses) {
      const auto* chain_cashed_data = allowance_cashe_dict.FindDict(chain_id);

      const auto* last_block_number_ptr =
          chain_cashed_data != nullptr
              ? chain_cashed_data->FindString("last_block_number")
              : nullptr;
      const std::string last_block_number{
          last_block_number_ptr != nullptr ? *last_block_number_ptr : ""};

      std::string hex_account_address;
      if (!PadHexEncodedParameter(account_address, &hex_account_address)) {
        continue;
      }
      auto internal_callback = base::BindOnce(
          &EthAllowanceManager::OnGetAllowances, weak_ptr_factory_.GetWeakPtr(),
          barrier_callback, chain_id, hex_account_address);

      base::Value::List topics;
      topics.Append(approval_topic_hash);
      topics.Append(std::move(hex_account_address));

      base::Value::Dict filter_options;
      filter_options.Set("address", std::move(contract_addresses));
      filter_options.Set("topics", std::move(topics));
      uint256_t block_number{0};
      if (!last_block_number.empty() &&
          HexValueToUint256(last_block_number, &block_number)) {
        filter_options.Set("fromBlock", Uint256ValueToHex(++block_number));
      } else {
        filter_options.Set("fromBlock", "earliest");
      }
      filter_options.Set("toBlock", "latest");
      json_rpc_service_->EthGetLogs(chain_id, std::move(filter_options),
                                    std::move(internal_callback));
    }
  }
}

auto EthAllowanceManager::GetAllowanceMapKey(
    const std::string& contract_address,
    const std::string& approver_addr,
    const std::string& spender_address) {
  return base::JoinString({contract_address, approver_addr, spender_address},
                          "_");
}

void EthAllowanceManager::LoadCachedAllowances(
    const std::string& chain_id,
    const std::string& hex_account_address,
    base::flat_map<std::string,
                   std::tuple<brave_wallet::uint256_t,
                              mojom::AllowanceInfoPtr>>& allowances_map) {
  const auto& allowance_cashe_dict =
      prefs_->GetDict(kBraveWalletEthAllowancesCache);

  const auto* chain_cashed_data = allowance_cashe_dict.FindDict(chain_id);
  const auto* cashed_allowances_ptr =
      chain_cashed_data != nullptr
          ? chain_cashed_data->FindList("allowances_found")
          : nullptr;
  const auto* block_number_ptr =
      chain_cashed_data != nullptr
          ? chain_cashed_data->FindString("last_block_number")
          : nullptr;

  uint256_t block_number{0};
  if (!block_number_ptr ||
      !HexValueToUint256(*block_number_ptr, &block_number)) {
    block_number = 0;
  }

  auto ca_is_not_loaded = [&hex_account_address](
                              const std::string* approver_address,
                              const std::string* contract_address,
                              const std::string* spender_address,
                              const std::string* amount) {
    return !approver_address || !contract_address || !spender_address ||
           !amount ||
           base::ToUpperASCII(*approver_address) !=
               base::ToUpperASCII(hex_account_address);
  };

  if (!cashed_allowances_ptr) {
    return;
  }

  for (const auto& ca_item : *cashed_allowances_ptr) {
    const auto* ca_dict = ca_item.GetIfDict();
    if (!ca_dict) {
      continue;
    }
    const auto* approver_address = ca_dict->FindString("approver_address");
    const auto* contract_address = ca_dict->FindString("contract_address");
    const auto* spender_address = ca_dict->FindString("spender_address");
    const auto* amount = ca_dict->FindString("amount");

    if (ca_is_not_loaded(approver_address, contract_address, spender_address,
                         amount)) {
      continue;
    }

    allowances_map.insert_or_assign(
        GetAllowanceMapKey(*contract_address, *approver_address,
                           *spender_address),
        std::make_tuple(block_number, mojom::AllowanceInfo::New(
                                          *contract_address, *approver_address,
                                          *spender_address, *amount)));
  }
}

void EthAllowanceManager::OnGetAllowances(
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
    const std::string& error_message) {
  base::flat_map<std::string,
                 std::tuple<uint256_t, std::vector<mojom::AllowanceInfoPtr>>>
      allowances;

  if (error != mojom::ProviderError::kSuccess) {
    std::move(barrier_callback).Run(std::move(allowances));
    return;
  }

  std::vector<Log> logs_tmp{logs};
  std::sort(logs_tmp.begin(), logs_tmp.end(),
            [](const Log& first, const Log& second) -> bool {
              if (first.block_number < second.block_number) {
                return true;
              }
              if (first.block_number == second.block_number) {
                return first.log_index < second.log_index;
              }
              return false;
            });

  // Collection of the latest allowances per contract & spender.
  base::flat_map<std::string,
                 std::tuple<brave_wallet::uint256_t, mojom::AllowanceInfoPtr>>
      allowances_map;

  auto allowance_map_key = [](const std::string& contract_address,
                              const std::string& approver_addr,
                              const std::string& spender_address) {
    return base::JoinString({contract_address, approver_addr, spender_address},
                            "_");
  };
  // Put cashed data into the map first.
  LoadCachedAllowances(chain_id, hex_account_address, allowances_map);
  for (const auto& log_item : logs_tmp) {
    // Skip pending logs.
    if (log_item.block_number == 0) {
      continue;
    }
    if (log_item.topics.size() < 3) {
      continue;
    }

    allowances_map.insert_or_assign(
        allowance_map_key(log_item.address, log_item.topics[1],
                          log_item.topics[2]),
        std::make_tuple(
            log_item.block_number,
            mojom::AllowanceInfo::New(log_item.address, log_item.topics[1],
                                      log_item.topics[2], log_item.data)));
  }
  std::vector<mojom::AllowanceInfoPtr> allowances_found;
  uint256_t max_block_number{0};
  for (auto& [contract_address, allowance_tuple] : allowances_map) {
    uint256_t parsed_amount{0};
    if (HexValueToUint256(std::get<1>(allowance_tuple)->amount,
                          &parsed_amount) &&
        parsed_amount > 0) {
      allowances_found.push_back(std::move(std::get<1>(allowance_tuple)));
    }
    if (std::get<0>(allowance_tuple) > max_block_number) {
      max_block_number = std::get<0>(allowance_tuple);
    }
  }
  allowances.insert_or_assign(
      chain_id, std::make_tuple(max_block_number, std::move(allowances_found)));
  std::move(barrier_callback).Run(std::move(allowances));
}

void EthAllowanceManager::MergeEthAllowances(
    const std::vector<base::flat_map<
        std::string,
        std::tuple<uint256_t, std::vector<mojom::AllowanceInfoPtr>>>>&
        discovered_allowance_results) {
  ScopedDictPrefUpdate allowance_cashe_update(prefs_,
                                              kBraveWalletEthAllowancesCache);
  auto& allowance_cashe = allowance_cashe_update.Get();
  std::vector<mojom::AllowanceInfoPtr> result;
  for (const auto& allowances_map : discovered_allowance_results) {
    for (const auto& [chain_id, allowances] : allowances_map) {
      auto* chain_section = allowance_cashe.FindDict(chain_id);

      base::Value::List allw_list;
      for (const auto& allowance : std::get<1>(allowances)) {
        base::Value::Dict allw_dict_item;
        allw_dict_item.Set("contract_address", allowance->contract_address);
        allw_dict_item.Set("approver_address", allowance->approver_address);
        allw_dict_item.Set("spender_address", allowance->spender_address);
        allw_dict_item.Set("amount", allowance->amount);
        allw_list.Append(std::move(allw_dict_item));
        result.push_back(allowance.Clone());
      }

      if (allw_list.empty()) {
        continue;
      }

      if (!chain_section) {
        base::Value::Dict chain_dict_item;
        chain_dict_item.Set("last_block_number",
                            Uint256ValueToHex(std::get<0>(allowances)));
        chain_dict_item.Set("allowances_found", std::move(allw_list));
        allowance_cashe.Set(chain_id, std::move(chain_dict_item));
      } else {
        chain_section->Set("last_block_number",
                           Uint256ValueToHex(std::get<0>(allowances)));
        chain_section->Set("allowances_found", std::move(allw_list));
      }
    }
  }
  CompleteDiscovedEthAllowance(result);
}

void EthAllowanceManager::CompleteDiscovedEthAllowance(
    const std::vector<mojom::AllowanceInfoPtr>& allowances) {
  is_eth_allowance_discovering_running_ = false;

  wallet_service_->OnDiscoverEthAllowancesCompleted(allowances);
}

}  // namespace brave_wallet
