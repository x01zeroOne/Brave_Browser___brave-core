/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <windows.h>
#include <string>

namespace brave_vpn {

bool ConfigureServiceAutoRestart(const std::wstring& service_name);

bool SetServiceFailActions(SC_HANDLE service);
std::wstring GetVpnServiceName();
}  // namespace brave_vpn
