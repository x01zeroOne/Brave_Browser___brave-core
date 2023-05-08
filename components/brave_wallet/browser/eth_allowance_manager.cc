/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_allowance_manager.h"

#include <algorithm>
#include <utility>

#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
constexpr char kLastBlockNumber[] = "last_block_number";
constexpr char kAllowanceFound[] = "allowances_found";
constexpr char kApproverAddress[] = "approver_address";
constexpr char kContractAddress[] = "contract_address";
constexpr char kSpenderAddress[] = "spender_address";
constexpr char kAmount[] = "amount";

constexpr char kAddress[] = "address";
constexpr char kTopics[] = "topics";
constexpr char kFromBlock[] = "fromBlock";
constexpr char kToBlock[] = "toBlock";
constexpr char kEarliestBlock[] = "earliest";
constexpr char kLatestBlock[] = "latest";

constexpr char kApprovalTopicHash[] = "Approval(address,address,uint256)";

// Returns vector of chain_ids supported by allowance discovering.
const std::vector<std::string>& GetChainIdsForAlowanceDiscovering() {
  static base::NoDestructor<std::vector<std::string>> supported_chain_ids(
      {brave_wallet::mojom::kMainnetChainId,
       brave_wallet::mojom::kPolygonMainnetChainId,
       brave_wallet::mojom::kAvalancheMainnetChainId,
       brave_wallet::mojom::kCeloMainnetChainId,
       brave_wallet::mojom::kArbitrumMainnetChainId,
       brave_wallet::mojom::kOptimismMainnetChainId});
  return *supported_chain_ids;
}
static std::string GetAllowanceMapKey(const std::string& contract_address,
                                      const std::string& approver_addr,
                                      const std::string& spender_address) {
  return base::JoinString({contract_address, approver_addr, spender_address},
                          "_");
}
}  // namespace

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

/**
 * @brief Represents start of the allowance discovering operation
 *
 * @param callback callback to front end, to pass allowances list, error code
 * and description
 */
void EthAllowanceManager::DiscoverEthAllowancesOnAllSupportedChains(
    BraveWalletService::DiscoverEthAllowancesCallback callback) {
  if (is_eth_allowance_discovering_running_) {
    std::move(callback).Run(
        {}, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
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
    std::move(callback).Run({}, mojom::ProviderError::kInternalError, "");
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
    std::move(callback).Run({}, mojom::ProviderError::kInternalError, "");
    return;
  }

  const auto& allowance_cache_dict =
      prefs_->GetDict(kBraveWalletEthAllowancesCache);

  // Use a barrier callback to wait for all EthGetLogs calls to
  // complete (one for each chain_id).
  const auto barrier_callback = base::BarrierCallback<base::flat_map<
      std::string,
      std::tuple<uint256_t, std::vector<mojom::AllowanceInfoPtr>>>>(
      account_addresses.size() * chain_id_to_contract_addresses.size(),
      base::BindOnce(&EthAllowanceManager::MergeEthAllowances,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  const auto approval_topic_hash = KeccakHash(kApprovalTopicHash);
  for (const auto& account_address : account_addresses) {
    for (const auto& [chain_id, contract_addresses] :
         chain_id_to_contract_addresses) {
      const auto* chain_cached_data = allowance_cache_dict.FindDict(chain_id);

      const auto* last_block_number_ptr =
          chain_cached_data ? chain_cached_data->FindString(kLastBlockNumber)
                            : nullptr;
      const std::string last_block_number =
          last_block_number_ptr != nullptr ? *last_block_number_ptr : "";

      std::string hex_account_address;
      if (!PadHexEncodedParameter(account_address, &hex_account_address)) {
        base::flat_map<
            std::string,
            std::tuple<uint256_t, std::vector<mojom::AllowanceInfoPtr>>>
            empty_allowance_map;
        // Call the barrier_callback, to have correct value for barrier calls
        // counter.
        barrier_callback.Run(std::move(empty_allowance_map));
        continue;
      }
      auto internal_callback = base::BindOnce(
          &EthAllowanceManager::OnGetAllowances, weak_ptr_factory_.GetWeakPtr(),
          barrier_callback, chain_id, hex_account_address);

      base::Value::List topics;
      topics.Append(approval_topic_hash);
      topics.Append(std::move(hex_account_address));

      base::Value::Dict filter_options;
      filter_options.Set(kAddress, contract_addresses.Clone());
      filter_options.Set(kTopics, std::move(topics));
      uint256_t block_number{0};
      if (!last_block_number.empty() &&
          HexValueToUint256(last_block_number, &block_number)) {
        filter_options.Set(kFromBlock, Uint256ValueToHex(++block_number));
      } else {
        filter_options.Set(kFromBlock, kEarliestBlock);
      }
      filter_options.Set(kToBlock, kLatestBlock);
      json_rpc_service_->EthGetLogs(chain_id, std::move(filter_options),
                                    std::move(internal_callback));
    }
  }
}

/**
 * @brief Represents loading cached data from the preferences.
 * Cached data, always must be loaded first as we loading retrieved from the
 * logs "fresh" data
 *
 * Path of the new preferences key is: "brave.wallet.eth_allowances_cache"
 * "eth_allowances_cache": {
 *    "<chain_id>": {
 *      "allowances_found": [{
 *          "amount": "<hex encoded (with padding) value allowed to be spent>",
 *          "approver_address": "<approver address (with padding)>",
 *          "contract_address": "<address of the contract>",
 *          "spender_address": "<approver address (with padding)>"
 *      }],
 *      "last_block_number": "<number of the last processed block (hex
 * encoded)>"
 *    }
 *  }
 *
 * @param chain_id - Id of the chain you want to load.
 * @param hex_account_address  - account address (with padding) you want to
 * filter by.
 * @param [out] allowance_map - Loaded from the preferences allowances cached
 * data represented as map. Key of the map is the unique combination of the next
 * values: <contract_address>_<approver_addr>_<spender_address>. The value of
 * the map is a std::tuple which contains <uint256_t> - block number and
 * <mojom::AllowanceInfoPtr> allowance info data.
 */
void EthAllowanceManager::LoadCachedAllowances(
    const std::string& chain_id,
    const std::string& hex_account_address,
    base::flat_map<std::string, std::tuple<uint256_t, mojom::AllowanceInfoPtr>>&
        allowance_map) {
  const auto& allowance_cache_dict =
      prefs_->GetDict(kBraveWalletEthAllowancesCache);

  const auto* chain_cached_data = allowance_cache_dict.FindDict(chain_id);
  if (!chain_cached_data) {
    return;
  }

  const auto* cached_allowances_ptr =
      chain_cached_data->FindList(kAllowanceFound);
  if (!cached_allowances_ptr) {
    return;
  }

  const auto* block_number_ptr =
      chain_cached_data->FindString(kLastBlockNumber);

  uint256_t block_number{0};
  if (!block_number_ptr ||
      !HexValueToUint256(*block_number_ptr, &block_number)) {
    return;
  }

  for (const auto& ca_item : *cached_allowances_ptr) {
    const auto* ca_dict = ca_item.GetIfDict();
    if (!ca_dict) {
      continue;
    }
    const auto* approver_address = ca_dict->FindString(kApproverAddress);
    const auto* contract_address = ca_dict->FindString(kContractAddress);
    const auto* spender_address = ca_dict->FindString(kSpenderAddress);
    const auto* amount = ca_dict->FindString(kAmount);

    if (!approver_address || !contract_address || !spender_address || !amount ||
        base::ToUpperASCII(*approver_address) !=
            base::ToUpperASCII(hex_account_address)) {
      continue;
    }

    allowance_map.insert_or_assign(
        GetAllowanceMapKey(*contract_address, *approver_address,
                           *spender_address),
        std::make_tuple(block_number,
                        mojom::AllowanceInfo::New(chain_id, *contract_address,
                                                  *approver_address,
                                                  *spender_address, *amount)));
  }
}

/**
 * @brief Represents intermediate callback which processes data portions
 * retrieved from logs
 *
 * @param barrier_callback - Callback, which must be called, when all data
 * portions are processed. Incoming parameter of the callback is a map, which
 * has a chain_id as a std::string key and std::tuple as a value. Each tuple
 * contains pair of values, <uint256_t> - block number and
 * std::vector<mojom::AllowanceInfoPtr> - allowance info data.
 * @param chain_id - Id of the Chain, which the current data portion belongs to
 * @param hex_account_address - Account address (whith padding), which the
 * current data portion belongs to
 * @param logs - chain_id and account address related Log data portion
 * @param rawlogs - chain_id and account address related Log data portion
 * @param error - error code
 * @param error_message - error message
 */
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

  // Collection of the latest allowances per contract & spender & approver.
  base::flat_map<std::string, std::tuple<uint256_t, mojom::AllowanceInfoPtr>>
      allowance_map;

  // Put cached data into the map first if available.
  LoadCachedAllowances(chain_id, hex_account_address, allowance_map);
  for (const auto& log_item : logs_tmp) {
    // Skip pending logs.
    if (log_item.block_number == 0) {
      continue;
    }
    if (log_item.topics.size() < 3) {
      continue;
    }
    uint256_t parsed_amount{0};
    if (!HexValueToUint256(log_item.data, &parsed_amount)) {
      continue;
    }

    const auto current_map_key = GetAllowanceMapKey(
        log_item.address, log_item.topics[1], log_item.topics[2]);
    if (parsed_amount > 0) {
      // Replace if same key exists by the fresh allowance data.
      allowance_map.insert_or_assign(
          current_map_key,
          std::make_tuple(log_item.block_number,
                          mojom::AllowanceInfo::New(
                              chain_id, log_item.address, log_item.topics[1],
                              log_item.topics[2], log_item.data)));
    } else if (allowance_map.find(current_map_key) != allowance_map.end()) {
      allowance_map.erase(current_map_key);
    }
  }
  std::vector<mojom::AllowanceInfoPtr> allowances_found;
  uint256_t max_block_number{0};
  for (const auto& [allowance_map_key, allowance_tuple] : allowance_map) {
    allowances_found.push_back(std::get<1>(allowance_tuple).Clone());
    if (std::get<0>(allowance_tuple) > max_block_number) {
      max_block_number = std::get<0>(allowance_tuple);
    }
  }
  allowances.insert_or_assign(
      chain_id, std::make_tuple(max_block_number, std::move(allowances_found)));
  std::move(barrier_callback).Run(std::move(allowances));
}

/**
 * @brief Represents barrier callback, which processes vector of data portions,
 * collected with intermediate callbacks.
 *
 * @param callback callback to front end, to pass allowances list, error code
 * and description
 * @param discovered_allowance_results vector of data portions, It is vector of
 * maps. Each map has a chain_id as a std::string key and std::tuple as a value.
 * Each tuple contains pair of values, <uint256_t> - block number and
 * std::vector<mojom::AllowanceInfoPtr> - allowance info data.
 */
void EthAllowanceManager::MergeEthAllowances(
    BraveWalletService::DiscoverEthAllowancesCallback callback,
    const std::vector<base::flat_map<
        std::string,
        std::tuple<uint256_t, std::vector<mojom::AllowanceInfoPtr>>>>&
        discovered_allowance_results) {
  ScopedDictPrefUpdate allowance_cache_update(prefs_,
                                              kBraveWalletEthAllowancesCache);
  auto& allowance_cache = allowance_cache_update.Get();
  std::vector<mojom::AllowanceInfoPtr> result;
  for (const auto& allowance_map : discovered_allowance_results) {
    for (const auto& [chain_id, allowances] : allowance_map) {
      base::Value::List allw_list;
      for (const auto& allowance : std::get<1>(allowances)) {
        base::Value::Dict allw_dict_item;
        allw_dict_item.Set(kContractAddress, allowance->contract_address);
        allw_dict_item.Set(kApproverAddress, allowance->approver_address);
        allw_dict_item.Set(kSpenderAddress, allowance->spender_address);
        allw_dict_item.Set(kAmount, allowance->amount);
        allw_list.Append(std::move(allw_dict_item));
        result.push_back(allowance.Clone());
      }

      if (allw_list.empty()) {
        continue;
      }
      auto* chain_section = allowance_cache.EnsureDict(chain_id);
      chain_section->Set(kLastBlockNumber,
                         Uint256ValueToHex(std::get<0>(allowances)));
      chain_section->Set(kAllowanceFound, std::move(allw_list));
    }
  }

  is_eth_allowance_discovering_running_ = false;
  std::move(callback).Run(std::move(result), mojom::ProviderError::kSuccess,
                          "");
}
}  // namespace brave_wallet
