/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_WIREGUARD_TUNNEL_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_WIREGUARD_TUNNEL_SERVICE_H_

#include <string>
#include "base/files/file_path.h"

namespace brave_vpn {

namespace wireguard {
int LaunchService(const std::wstring& config);
bool RemoveExistingWireguradService();
int CreateAndRunBraveWGService(const std::wstring& config);

int RunWireGuardTunnelService(const std::wstring& config);
bool WireGuardGenerateKeypair(std::string* public_key,
                              std::string* private_key);
}  // namespace wireguard

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_WIREGUARD_TUNNEL_SERVICE_H_
