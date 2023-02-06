/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/tip_panel_handler.h"

#include <utility>

#include "chrome/browser/profiles/profile.h"

namespace brave_rewards {

TipPanelHandler::TipPanelHandler(
    mojo::PendingRemote<mojom::TipPanel> banner,
    mojo::PendingReceiver<mojom::TipPanelHandler> receiver,
    base::WeakPtr<ui::MojoBubbleWebUIController::Embedder> embedder)
    : receiver_(this, std::move(receiver)),
      banner_(std::move(banner)),
      embedder_(embedder) {
  DCHECK(embedder_);
}

TipPanelHandler::~TipPanelHandler() = default;

void TipPanelHandler::ShowUI() {
  if (embedder_) {
    embedder_->ShowUI();
  }
}

void TipPanelHandler::CloseUI() {
  if (embedder_) {
    embedder_->CloseUI();
  }
}

}  // namespace brave_rewards
