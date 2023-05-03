/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/nonce_tracker.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

NonceTracker::NonceTracker(TxStateManager* tx_state_manager,
                           JsonRpcService* json_rpc_service)
    : json_rpc_service_(json_rpc_service),
      tx_state_manager_(tx_state_manager),
      weak_factory_(this) {}

NonceTracker::~NonceTracker() = default;

void NonceTracker::GetFinalNonce(const std::string& chain_id,
                                 const std::string& from,
                                 uint256_t network_nonce,
                                 GetFinalNonceCallback callback) {
  tx_state_manager_->GetTransactionsByStatus(
      chain_id, mojom::TransactionStatus::Confirmed, from,
      base::BindOnce(&NonceTracker::ContineGetFinalNonceConfirmed,
                     weak_factory_.GetWeakPtr(), chain_id, from, network_nonce,
                     std::move(callback)));
}

void NonceTracker::ContineGetFinalNonceConfirmed(
    const std::string& chain_id,
    const std::string& from,
    uint256_t network_nonce,
    GetFinalNonceCallback callback,
    std::vector<std::unique_ptr<TxMeta>> confirmed_txs) {
  uint256_t local_highest = GetHighestLocallyConfirmed(confirmed_txs);

  uint256_t highest_confirmed = std::max(network_nonce, local_highest);

  tx_state_manager_->GetTransactionsByStatus(
      chain_id, mojom::TransactionStatus::Submitted, from,
      base::BindOnce(&NonceTracker::ContineGetFinalNoncePending,
                     weak_factory_.GetWeakPtr(), highest_confirmed,
                     network_nonce, std::move(callback)));
}

void NonceTracker::ContineGetFinalNoncePending(
    uint256_t highest_confirmed,
    uint256_t network_nonce,
    GetFinalNonceCallback callback,
    std::vector<std::unique_ptr<TxMeta>> pending_txs) {
  uint256_t highest_continuous_from =
      GetHighestContinuousFrom(pending_txs, highest_confirmed);

  uint256_t nonce = std::max(network_nonce, highest_continuous_from);

  std::move(callback).Run(nonce);
}

}  // namespace brave_wallet
