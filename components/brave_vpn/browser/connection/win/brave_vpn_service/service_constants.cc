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
// 053057AB-CF06-4E6C-BBAD-F8DA6436D933
constexpr CLSID kBraveVpnServiceCLSID = {
    0x053057ab,
    0xcf06,
    0x4e6c,
    {0xbb, 0xad, 0xf8, 0xda, 0x64, 0x36, 0xd9, 0x33}};
// 088C5F6E-B213-4A8E-98AD-9D64D8913968
constexpr IID kBraveVpnServiceIID = {
    0x088c5f6e,
    0xb213,
    0x4a8e,
    {0x98, 0xad, 0x9d, 0x64, 0xd8, 0x91, 0x39, 0x68}};

}  // namespace

// Returns the Brave Vpn Service CLSID, IID, Name, and Display Name
// respectively.
const CLSID& GetBraveVpnServiceClsid() {
  return kBraveVpnServiceCLSID;
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
