/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/brave_vpn_service.h"

#include <windows.h>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_constants.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_main.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/wireguard_tunnel_service.h"

namespace brave_vpn {

HRESULT BraveVpnService::EnableVpn(const wchar_t* config, DWORD* last_error) {
  VLOG(1) << __func__ << ":" << config;
  if (!config || !last_error) {
    VLOG(1) << "Invalid parameters";
    return E_FAIL;
  }
  *last_error = brave_vpn::wireguard::LaunchService(config);
  return S_OK;
}

HRESULT BraveVpnService::DisableVpn(DWORD* last_error) {
  *last_error = !brave_vpn::wireguard::RemoveExistingWireguradService();
  return S_OK;
}

HRESULT BraveVpnService::GenerateKeypair(BSTR* public_key,
                                         BSTR* private_key,
                                         DWORD* last_error) {
  VLOG(1) << __func__;
  std::string public_key_raw;
  std::string private_key_raw;
  if (!brave_vpn::wireguard::WireGuardGenerateKeypair(&public_key_raw,
                                                      &private_key_raw)) {
    VLOG(1) << __func__ << ": unable to generate keys";
    *last_error = 1;
    return S_OK;
  }

  *public_key = ::SysAllocString(base::UTF8ToWide(public_key_raw).c_str());
  *private_key = ::SysAllocString(base::UTF8ToWide(private_key_raw).c_str());
  *last_error = 0;
  return S_OK;
}

}  // namespace brave_vpn
