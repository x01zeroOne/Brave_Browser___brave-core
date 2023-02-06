/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/common/brave_rewards_tip_panel.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

namespace brave_rewards {

class TipPanelHandler : public mojom::TipPanelHandler {
 public:
  TipPanelHandler(
      mojo::PendingRemote<mojom::TipPanel> banner,
      mojo::PendingReceiver<mojom::TipPanelHandler> receiver,
      base::WeakPtr<ui::MojoBubbleWebUIController::Embedder> embedder);

  TipPanelHandler(const TipPanelHandler&) = delete;
  TipPanelHandler& operator=(const TipPanelHandler&) = delete;

  ~TipPanelHandler() override;

  // mojom::TipPanelHandler:
  void ShowUI() override;
  void CloseUI() override;

 private:
  mojo::Receiver<mojom::TipPanelHandler> receiver_;
  mojo::Remote<mojom::TipPanel> banner_;
  base::WeakPtr<ui::MojoBubbleWebUIController::Embedder> embedder_;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_HANDLER_H_
