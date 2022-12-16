/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_

#include "build/buildflag.h"

namespace brave_vpn {

constexpr char kBraveVPNHelperProcessType[] = "brave-vpn-helper";
const char kBraveVpnHelperConfigureTriggers[] = "--configure-triggers";
constexpr wchar_t kBraveVpnHelperRegistryStoragePath[] =
    L"Software\\BraveSoftware\\Brave\\Vpn";
const wchar_t kBraveVPNHelperExecutable[] = L"brave_vpn_helper.exe";
const wchar_t kBraveVpnServiceName[] = L"BraveVPNService";
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_HELPER_BRAVE_VPN_HELPER_CONSTANTS_H_
