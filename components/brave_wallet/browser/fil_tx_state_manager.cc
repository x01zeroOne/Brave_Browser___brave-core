/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"

#include <utility>

#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

FilTxStateManager::FilTxStateManager(PrefService* prefs,
                                     const base::FilePath& context_path)
    : TxStateManager(prefs, context_path) {}

FilTxStateManager::~FilTxStateManager() = default;

void FilTxStateManager::GetFilTx(const std::string& chain_id,
                                 const std::string& id,
                                 GetFilTxCallback callback) {
  TxStateManager::GetTx(
      chain_id, id,
      base::BindOnce(
          [](GetFilTxCallback callback, std::unique_ptr<TxMeta> meta) {
            std::move(callback).Run(std::unique_ptr<FilTxMeta>{
                static_cast<FilTxMeta*>(meta.release())});
          },
          std::move(callback)));
}

std::string FilTxStateManager::GetTxPrefPathPrefix(
    const absl::optional<std::string>& chain_id) {
  if (chain_id.has_value()) {
    return base::StrCat(
        {kFilecoinPrefKey, ".",
         GetNetworkId(prefs_, mojom::CoinType::FIL, *chain_id)});
  }
  return kFilecoinPrefKey;
}

mojom::CoinType FilTxStateManager::GetCoinType() const {
  return mojom::CoinType::FIL;
}

std::unique_ptr<FilTxMeta> FilTxStateManager::ValueToFilTxMeta(
    const base::Value::Dict& value) {
  return std::unique_ptr<FilTxMeta>{
      static_cast<FilTxMeta*>(ValueToTxMeta(value).release())};
}

std::unique_ptr<TxMeta> FilTxStateManager::ValueToTxMeta(
    const base::Value::Dict& value) {
  std::unique_ptr<FilTxMeta> meta = std::make_unique<FilTxMeta>();

  if (!TxStateManager::ValueToTxMeta(value, meta.get())) {
    return nullptr;
  }
  const base::Value::Dict* tx = value.FindDict("tx");
  if (!tx) {
    return nullptr;
  }
  absl::optional<FilTransaction> tx_from_value = FilTransaction::FromValue(*tx);
  if (!tx_from_value) {
    return nullptr;
  }
  meta->set_tx(std::make_unique<FilTransaction>(*tx_from_value));
  return meta;
}

}  // namespace brave_wallet
