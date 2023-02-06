/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/tip_panel_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/webui/brave_rewards/tip_panel_handler.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/tip_panel_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"

namespace brave_rewards {

namespace {

// static constexpr webui::LocalizedString kStrings[] = {};

}  // namespace

TipPanelUI::TipPanelUI(content::WebUI* web_ui)
    : MojoBubbleWebUIController(web_ui, true) {
  auto* profile = Profile::FromWebUI(web_ui);

  auto* source = content::WebUIDataSource::Create(kBraveTipPanelHost);

  // source->AddLocalizedStrings(kStrings);

  webui::SetupWebUIDataSource(
      source, base::make_span(kTipPanelGenerated, kTipPanelGeneratedSize),
      IDR_TIP_PANEL_HTML);

  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                source);

  // TODO: Needed?
  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));
}

TipPanelUI::~TipPanelUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(TipPanelUI)

void TipPanelUI::BindInterface(
    mojo::PendingReceiver<TipPanelHandlerFactory> receiver) {
  factory_receiver_.reset();
  factory_receiver_.Bind(std::move(receiver));
}

void TipPanelUI::CreateHandler(
    mojo::PendingRemote<mojom::TipPanel> panel,
    mojo::PendingReceiver<mojom::TipPanelHandler> handler) {
  DCHECK(panel);

  handler_ = std::make_unique<TipPanelHandler>(std::move(panel),
                                               std::move(handler), embedder());
}

}  // namespace brave_rewards
