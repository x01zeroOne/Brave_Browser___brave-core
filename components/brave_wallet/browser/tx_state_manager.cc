/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_state_manager.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/json/values_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/solana_message.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/value_store/value_store_factory_impl.h"
#include "components/value_store/value_store_frontend.h"
#include "components/value_store/value_store_task_runner.h"
#include "url/origin.h"

#if BUILDFLAG(IS_IOS)
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#else
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#endif  // BUILDFLAG(IS_IOS)

namespace brave_wallet {

// static
bool TxStateManager::ValueToTxMeta(const base::Value::Dict& value,
                                   TxMeta* meta) {
  const std::string* id = value.FindString("id");
  if (!id) {
    return false;
  }
  meta->set_id(*id);

  absl::optional<int> status = value.FindInt("status");
  if (!status) {
    return false;
  }
  meta->set_status(static_cast<mojom::TransactionStatus>(*status));
  const std::string* from = value.FindString("from");
  if (!from) {
    return false;
  }
  meta->set_from(*from);

  const base::Value* created_time = value.Find("created_time");
  if (!created_time) {
    return false;
  }
  absl::optional<base::Time> created_time_from_value =
      base::ValueToTime(created_time);
  if (!created_time_from_value) {
    return false;
  }
  meta->set_created_time(*created_time_from_value);

  const base::Value* submitted_time = value.Find("submitted_time");
  if (!submitted_time) {
    return false;
  }
  absl::optional<base::Time> submitted_time_from_value =
      base::ValueToTime(submitted_time);
  if (!submitted_time_from_value) {
    return false;
  }
  meta->set_submitted_time(*submitted_time_from_value);

  const base::Value* confirmed_time = value.Find("confirmed_time");
  if (!confirmed_time) {
    return false;
  }
  absl::optional<base::Time> confirmed_time_from_value =
      base::ValueToTime(confirmed_time);
  if (!confirmed_time_from_value) {
    return false;
  }
  meta->set_confirmed_time(*confirmed_time_from_value);

  const std::string* tx_hash = value.FindString("tx_hash");
  if (!tx_hash) {
    return false;
  }
  meta->set_tx_hash(*tx_hash);

  const std::string* origin_spec = value.FindString("origin");
  // That's ok not to have origin.
  if (origin_spec) {
    meta->set_origin(url::Origin::Create(GURL(*origin_spec)));
    DCHECK(!meta->origin()->opaque());
  }

  const std::string* group_id = value.FindString("group_id");
  if (group_id) {
    meta->set_group_id(*group_id);
  }

  const auto* chain_id_string = value.FindString("chain_id");
  if (!chain_id_string) {
    return false;
  }
  meta->set_chain_id(*chain_id_string);

  return true;
}

TxStateManager::TxStateManager(PrefService* prefs,
                               const base::FilePath& context_path)
    : prefs_(prefs), weak_factory_(this) {
  store_factory_ = base::MakeRefCounted<value_store::ValueStoreFactoryImpl>(
      context_path.AppendASCII(kWalletBaseDirectory));
  store_ = std::make_unique<value_store::ValueStoreFrontend>(
      store_factory_, base::FilePath(FILE_PATH_LITERAL(kWalletStorage)),
      kWalletStorage,
#if BUILDFLAG(IS_IOS)
      web::GetUIThreadTaskRunner({}),
#else
      content::GetUIThreadTaskRunner({}),
#endif
      value_store::GetValueStoreTaskRunner());
}

TxStateManager::~TxStateManager() = default;

void TxStateManager::AddOrUpdateTx(const TxMeta& meta) {
  store_->Get(kBraveWalletTransactions,
              base::BindOnce(&TxStateManager::ContinueAddOrUpdateTx,
                             weak_factory_.GetWeakPtr(), meta.ToValue()));
}

void TxStateManager::ContinueAddOrUpdateTx(base::Value::Dict meta_dict,
                                           absl::optional<base::Value> txs) {
  base::Value::Dict dict;
  if (txs) {
    dict = std::move(txs->GetDict());
  }
  auto meta = ValueToTxMeta(std::move(meta_dict));
  CHECK(meta);
  const std::string path = base::JoinString(
      {GetTxPrefPathPrefix(meta->chain_id()), meta->id()}, ".");

  bool is_add = dict.FindByDottedPath(path) == nullptr;
  dict.SetByDottedPath(path, meta->ToValue());
  store_->Set(kBraveWalletTransactions, base::Value(std::move(dict)));
  if (!is_add) {
    for (auto& observer : observers_) {
      observer.OnTransactionStatusChanged(meta->ToTransactionInfo());
    }
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNewUnapprovedTx(meta->ToTransactionInfo());
  }
}

void TxStateManager::GetTx(const std::string& chain_id,
                           const std::string& id,
                           GetTxCallback callback) {
  store_->Get(
      kBraveWalletTransactions,
      base::BindOnce(&TxStateManager::ContinueGetTx, weak_factory_.GetWeakPtr(),
                     chain_id, id, std::move(callback)));
}

void TxStateManager::ContinueGetTx(const std::string& chain_id,
                                   const std::string& id,
                                   GetTxCallback callback,
                                   absl::optional<base::Value> txs) {
  if (!txs) {
    std::move(callback).Run(nullptr);
    return;
  }
  const auto& dict = txs->GetDict();
  const base::Value::Dict* value = dict.FindDictByDottedPath(
      base::JoinString({GetTxPrefPathPrefix(chain_id), id}, "."));
  if (!value) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(ValueToTxMeta(*value));
}

void TxStateManager::DeleteTx(const std::string& chain_id,
                              const std::string& id) {
  store_->Get(kBraveWalletTransactions,
              base::BindOnce(&TxStateManager::ContinueDeleteTx,
                             weak_factory_.GetWeakPtr(), chain_id, id));
}

void TxStateManager::ContinueDeleteTx(const std::string& chain_id,
                                      const std::string& id,
                                      absl::optional<base::Value> txs) {
  if (!txs) {
    return;
  }
  auto& dict = txs->GetDict();
  dict.RemoveByDottedPath(
      base::JoinString({GetTxPrefPathPrefix(chain_id), id}, "."));
  store_->Set(kBraveWalletTransactions, base::Value(std::move(dict)));
}

void TxStateManager::WipeTxs() {
  store_->Remove(kBraveWalletTransactions);
}

void TxStateManager::GetTransactionsByStatus(
    const absl::optional<std::string>& chain_id,
    const absl::optional<mojom::TransactionStatus>& status,
    const absl::optional<std::string>& from,
    GetTxsByStatusCallback callback) {
  store_->Get(kBraveWalletTransactions,
              base::BindOnce(&TxStateManager::ContinueGetTransactionsByStatus,
                             weak_factory_.GetWeakPtr(), chain_id, status, from,
                             std::move(callback)));
}

void TxStateManager::ContinueGetTransactionsByStatus(
    const absl::optional<std::string>& chain_id,
    const absl::optional<mojom::TransactionStatus>& status,
    const absl::optional<std::string>& from,
    GetTxsByStatusCallback callback,
    absl::optional<base::Value> txs) {
  std::vector<std::unique_ptr<TxMeta>> result;
  if (!txs) {
    std::move(callback).Run(std::move(result));
    return;
  }
  const auto& dict = txs->GetDict();
  GetTransactionsByStatusInternal(chain_id, status, from, dict, result);
  std::move(callback).Run(std::move(result));
}

void TxStateManager::GetTransactionsByStatusInternal(
    const absl::optional<std::string>& chain_id,
    const absl::optional<mojom::TransactionStatus>& status,
    const absl::optional<std::string>& from,
    const base::Value::Dict& dict,
    std::vector<std::unique_ptr<TxMeta>>& result) {
  const base::Value::Dict* network_dict =
      dict.FindDictByDottedPath(GetTxPrefPathPrefix(chain_id));
  if (!network_dict) {
    return;
  }

  for (const auto it : *network_dict) {
    if (chain_id.has_value()) {
      std::unique_ptr<TxMeta> meta = ValueToTxMeta(it.second.GetDict());
      if (!meta) {
        continue;
      }
      if (!status.has_value() || meta->status() == *status) {
        if (from.has_value() && meta->from() != *from) {
          continue;
        }
        result.push_back(std::move(meta));
      }
    } else {
      auto chain_id_from_pref = GetChainId(prefs_, GetCoinType(), it.first);
      if (!chain_id_from_pref) {
        continue;
      }
      GetTransactionsByStatusInternal(chain_id_from_pref, status, from, dict,
                                      result);
    }
  }
}

void TxStateManager::AddObserver(TxStateManager::Observer* observer) {
  observers_.AddObserver(observer);
}

void TxStateManager::RemoveObserver(TxStateManager::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void TxStateManager::MigrateAddChainIdToTransactionInfo(PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletTransactionsChainIdMigrated)) {
    return;
  }
  if (!prefs->HasPrefPath(kBraveWalletTransactions)) {
    prefs->SetBoolean(kBraveWalletTransactionsChainIdMigrated, true);
    return;
  }

  ScopedDictPrefUpdate txs_update(prefs, kBraveWalletTransactions);
  auto& all_txs = txs_update.Get();

  auto set_chain_id = [&](base::Value::Dict* tx_by_network_ids,
                          const mojom::CoinType& coin) {
    for (auto tnid : *tx_by_network_ids) {
      auto chain_id = GetChainIdByNetworkId(prefs, coin, tnid.first);
      if (!chain_id.has_value()) {
        continue;
      }

      auto* txs = tnid.second.GetIfDict();

      if (!txs || txs->empty()) {
        return;
      }
      for (auto tx : *txs) {
        auto* ptx = tx.second.GetIfDict();

        if (!ptx) {
          continue;
        }

        ptx->Set("chain_id", chain_id.value());
      }
    }
  };

  for (auto txs_coin_type : all_txs) {
    auto coin = GetCoinTypeFromPrefKey(txs_coin_type.first);

    if (!coin.has_value()) {
      continue;
    }

    if (!txs_coin_type.second.is_dict()) {
      continue;
    }

    set_chain_id(&txs_coin_type.second.GetDict(), coin.value());
  }
  prefs->SetBoolean(kBraveWalletTransactionsChainIdMigrated, true);
}

void TxStateManager::MigrateSolanaTransactionsForV0TransactionsSupport(
    PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletSolanaTransactionsV0SupportMigrated)) {
    return;
  }

  if (!prefs->HasPrefPath(kBraveWalletTransactions)) {
    prefs->SetBoolean(kBraveWalletSolanaTransactionsV0SupportMigrated, true);
    return;
  }

  // Get message dict via solana.network_name.tx_id.tx.message. (example path:
  // solana.devnet.tx_id1.tx.message)
  // Then update message using SolanaMessage::FromDeprecatedLegacyValue and
  // SolanaMessage::ToValue.
  ScopedDictPrefUpdate update(prefs, kBraveWalletTransactions);
  base::Value::Dict* sol_txs = update.Get().FindDict(kSolanaPrefKey);
  if (!sol_txs) {
    prefs->SetBoolean(kBraveWalletSolanaTransactionsV0SupportMigrated, true);
    return;
  }

  for (auto txs_by_networks : *sol_txs) {
    if (!txs_by_networks.second.is_dict()) {
      continue;
    }

    for (auto txs_by_ids : txs_by_networks.second.GetDict()) {
      if (!txs_by_ids.second.is_dict()) {
        continue;
      }

      auto* tx_message =
          txs_by_ids.second.GetDict().FindDictByDottedPath("tx.message");
      if (!tx_message) {
        continue;
      }

      auto message = SolanaMessage::FromDeprecatedLegacyValue(*tx_message);
      if (!message) {
        continue;
      }

      *tx_message = message->ToValue();
    }
  }

  prefs->SetBoolean(kBraveWalletSolanaTransactionsV0SupportMigrated, true);
}

}  // namespace brave_wallet
