/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/brave_vpn_service.h"

#include "base/logging.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_main.h"

namespace brave_vpn {

HRESULT BraveVpnService::EnableVpn(const wchar_t* config) {
  LOG(ERROR) << __func__ << ":" << config;
  brave_vpn::ServiceMain* service = brave_vpn::ServiceMain::GetInstance();
  service->CreateWGConnection();
  return S_OK;
}

}  // namespace brave_vpn
