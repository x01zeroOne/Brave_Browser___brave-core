/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_os_connection_api_wg.h"

#include "base/json/json_reader.h"
#include "brave/components/brave_vpn/browser/connection/win/utils.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"

namespace brave_vpn {

using ConnectionState = mojom::ConnectionState;

BraveVPNOSConnectionAPIWireguard::BraveVPNOSConnectionAPIWireguard(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel)
    : BraveVPNOSConnectionAPIBase(url_loader_factory, local_prefs, channel),
      local_prefs_(local_prefs) {}

BraveVPNOSConnectionAPIWireguard::~BraveVPNOSConnectionAPIWireguard() {}

void BraveVPNOSConnectionAPIWireguard::CreateVPNConnectionImpl(
    const BraveVPNConnectionInfo& info) {
  BraveVPNOSConnectionAPIBase::OnCreated();
}

void BraveVPNOSConnectionAPIWireguard::RemoveVPNConnectionImpl(
    const std::string& name) {
  // TODO(spylogsster)
}

void BraveVPNOSConnectionAPIWireguard::ConnectImpl(const std::string& name) {
  // TODO(spylogsster)
  LOG(ERROR) << __func__;
}

void BraveVPNOSConnectionAPIWireguard::DisconnectImpl(const std::string& name) {
  // TODO(spylogsster)
  LOG(ERROR) << __func__;
}

void BraveVPNOSConnectionAPIWireguard::CheckConnectionImpl(
    const std::string& name) {
  // TODO(spylogsster)
  LOG(ERROR) << __func__;
}

void BraveVPNOSConnectionAPIWireguard::FetchProfileCredentials() {
  if (!GetAPIRequest()) {
    return;
  }

  // Get profile credentials it to create OS VPN entry.
  VLOG(2) << __func__ << " : request profile credential:"
          << GetBraveVPNPaymentsEnv(GetCurrentEnvironment());

  brave_vpn::internal::WireGuardGenerateKeypair(base::BindOnce(
      &BraveVPNOSConnectionAPIWireguard::OnWireguardKeypairGenerated,
      weak_factory_.GetWeakPtr()));
}

void BraveVPNOSConnectionAPIWireguard::OnWireguardKeypairGenerated(
    bool success,
    const std::string& public_key,
    const std::string& private_key) {
  LOG(ERROR) << "public_key:" << public_key << " private_key:" << private_key;
  GetAPIRequest()->GetWireguardProfileCredentials(
      base::BindOnce(&BraveVPNOSConnectionAPIWireguard::OnGetProfileCredentials,
                     base::Unretained(this)),
      GetSubscriberCredential(local_prefs_), public_key, GetHostname());
}

void BraveVPNOSConnectionAPIWireguard::OnGetProfileCredentials(
    const std::string& profile_credential,
    bool success) {
  DCHECK(!IsCancelConnecting());
  LOG(ERROR) << profile_credential;
  if (!success) {
    VLOG(2) << __func__ << " : failed to get profile credential";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  // api_request_.reset();

  VLOG(2) << __func__ << " : received profile credential";

  absl::optional<base::Value> value =
      base::JSONReader::Read(profile_credential);
  if (value.has_value()) {
    LOG(ERROR) << profile_credential;
  }
}

}  // namespace brave_vpn
