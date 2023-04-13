/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_BRAVE_VPN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_BRAVE_VPN_SERVICE_H_

#include <string>

#include <windows.h>

#include <wrl/implements.h>
#include <wrl/module.h>

#include "base/gtest_prod_util.h"
#include "base/win/windows_types.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/brave_vpn_service_idl.h"

namespace brave_vpn {

constexpr IID kTestBraveVpnServiceClsid = {
    0x416C51AC,
    0x4DEF,
    0x43CA,
    {0xE7, 0x35, 0xE7, 0x35, 0x21, 0x0A, 0xB2,
     0x57}};  // Elevator Test CLSID. {416C51AC-4DEF-43CA-96E8-E735210AB257}

namespace switches {
constexpr char kBraveVpnServiceClsIdForTestingSwitch[] =
    "elevator-clsid-for-testing";
}  // namespace switches

class BraveVpnService
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
          IBraveVpnService> {
 public:
  static constexpr HRESULT kErrorCouldNotObtainCallingProcess =
      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA001);
  static constexpr HRESULT kErrorCouldNotGenerateValidationData =
      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA002);
  static constexpr HRESULT kErrorCouldNotDecryptWithUserContext =
      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA003);
  static constexpr HRESULT kErrorCouldNotDecryptWithSystemContext =
      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA004);
  static constexpr HRESULT kErrorCouldNotEncryptWithUserContext =
      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA005);
  static constexpr HRESULT kErrorCouldNotEncryptWithSystemContext =
      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA006);
  static constexpr HRESULT kValidationDidNotPass =
      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xA007);

  BraveVpnService() = default;

  BraveVpnService(const BraveVpnService&) = delete;
  BraveVpnService& operator=(const BraveVpnService&) = delete;

  IFACEMETHODIMP EnableVpn(const wchar_t* config, DWORD* last_error) override;
  IFACEMETHODIMP DisableVpn(DWORD* last_error) override;
  IFACEMETHODIMP GenerateKeypair(BSTR* public_key,
                                 BSTR* private_key,
                                 DWORD* last_error) override;

 private:
  ~BraveVpnService() override = default;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_CONNECTION_MANAGER_H_
