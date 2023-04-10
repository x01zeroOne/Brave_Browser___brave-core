/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_SERVICE_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_SERVICE_CONSTANTS_H_

#include <guiddef.h>
#include <string>

#include "brave/components/brave_vpn/common/buildflags/buildflags.h"

namespace brave_vpn {

constexpr wchar_t kBraveVPNServiceExecutable[] = L"brave_vpn_service.exe";
constexpr char kBraveWgServiceInstall[] = "install";
// Returns the Brave Vpn Service CLSID, IID, Name, and Display Name
// respectively.
const CLSID& GetBraveVpnServiceClsid();
const IID& GetBraveVpnServiceIid();
std::wstring GetBraveVpnServiceName();
std::wstring GetBraveVpnServiceDisplayName();

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_SERVICE_CONSTANTS_H_
