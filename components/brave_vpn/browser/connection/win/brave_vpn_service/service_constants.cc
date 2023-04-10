/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_constants.h"

#include <guiddef.h>

#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/install_static/install_util.h"

namespace brave_vpn {

namespace {

constexpr CLSID kBraveVpnServiceCLSID = {0x576b31af,
          0x6369,
          0x4b6b,
          {0x85, 0x60, 0xe4, 0xb2, 0x3, 0xa9, 0x7a,
          0x8b}};

constexpr IID kBraveVpnServiceIID = {0xb7965c30,
                         0x7d58,
                         0x4d86,
                         {0x9e, 0x18, 0x47, 0x94, 0x25, 0x64, 0x9, 0xee}};

} // namespace

// Returns the Brave Vpn Service CLSID, IID, Name, and Display Name
// respectively.
const CLSID& GetBraveVpnServiceClsid() {
return  kBraveVpnServiceCLSID;
}

const IID& GetBraveVpnServiceIid() {
  return kBraveVpnServiceIID;
}

std::wstring GetVpnServiceDisplayName() {
  static constexpr wchar_t kBraveVpnServiceDisplayName[] = L" Vpn WG Service";
  return install_static::GetBaseAppName() + kBraveVpnServiceDisplayName;
}

std::wstring GetBraveVpnServiceName() {
  std::wstring name = GetVpnServiceDisplayName();
  name.erase(std::remove_if(name.begin(), name.end(), isspace), name.end());
  return name;
}

}  // namespace brave_vpn
