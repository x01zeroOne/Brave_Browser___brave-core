/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_UTILS_H_

#include <string>

#include "base/win/windows_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

class BraveVPNConnectionInfo;

namespace internal {

enum class CheckConnectionResult {
  CONNECTED,
  CONNECTING,
  CONNECT_FAILED,
  DISCONNECTING,
  DISCONNECTED,
};

struct RasOperationResult {
  bool success;
  // If not success, store user friendly error description.
  std::string error_description;
};

// Returns human readable error description.
std::string GetRasErrorMessage(DWORD error);
std::wstring GetPhonebookPath(const std::wstring& entry_name);

RasOperationResult CreateEntry(const BraveVPNConnectionInfo& info);
RasOperationResult RemoveEntry(const std::wstring& entry_name);
RasOperationResult DisconnectEntry(const std::wstring& entry_name);
RasOperationResult ConnectEntry(const std::wstring& entry_name);
CheckConnectionResult CheckConnection(const std::wstring& entry_name);
bool WireGuardGenerateKeypair(std::string* public_key,
                              std::string* private_key);
absl::optional<std::string> CreateWireguardConfig(
    const std::string& client_private_key,
    const std::string& server_public_key,
    const std::string& vpn_server_hostname,
    const std::string& mapped_ipv4_address,
    const std::string& dns_servers);
}  // namespace internal

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_UTILS_H_
