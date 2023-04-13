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
  brave_vpn::internal::StopVpnWGService(base::BindOnce(
      &BraveVPNOSConnectionAPIWireguard::OnWireguardServiceRemoved,
      weak_factory_.GetWeakPtr()));
}

void BraveVPNOSConnectionAPIWireguard::OnWireguardServiceRemoved(bool success) {
  if (!success) {
    VLOG(2) << __func__ << " : failed to get correct credentials";
    BraveVPNOSConnectionAPIBase::OnConnectFailed();
    return;
  }
  BraveVPNOSConnectionAPIBase::OnDisconnected();
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
    brave_vpn::internal::WireguardKeyPair key_pair) {
  if (!key_pair.has_value()) {
    return;
  }
  const auto [public_key, private_key] = key_pair.value();
  LOG(ERROR) << "public_key:" << public_key << " private_key:" << private_key;
  GetAPIRequest()->GetWireguardProfileCredentials(
      base::BindOnce(&BraveVPNOSConnectionAPIWireguard::OnGetProfileCredentials,
                     base::Unretained(this), private_key),
      GetSubscriberCredential(local_prefs_), public_key, GetHostname());
}

void BraveVPNOSConnectionAPIWireguard::OnGetProfileCredentials(
    const std::string& client_private_key,
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
  // {"api-auth-token":"eWmvsyLteSyHaOXUXFRLPDZZtvLgUGhP","client-id":"d059b2b48cf207ae","mapped-ipv4-address":"10.128.160.169","mapped-ipv6-address":"","server-public-key":"bcxGDtkUoqf+BhUzrl08a/s4Bf9baWZMI3xkHzbIYWE="}
  absl::optional<base::Value> value =
      base::JSONReader::Read(profile_credential);
  if (value.has_value()) {
    auto* server_public_key =
        value->GetDict().FindStringByDottedPath("server-public-key");
    auto* mapped_pi4_address =
        value->GetDict().FindStringByDottedPath("mapped-ipv4-address");
    if (!server_public_key || !mapped_pi4_address) {
      VLOG(2) << __func__ << " : failed to get correct credentials";
      UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
      return;
    }
    auto vpn_server_hostname = GetHostname();
    LOG(ERROR) << profile_credential;
    auto config = brave_vpn::internal::CreateWireguardConfig(
        client_private_key, *server_public_key, vpn_server_hostname,
        *mapped_pi4_address, "1.1.1.1");
    if (!config.has_value()) {
      VLOG(2) << __func__ << " : failed to get correct credentials";
      UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
      return;
    }
    brave_vpn::internal::StartVpnWGService(
        config.value(),
        base::BindOnce(
            &BraveVPNOSConnectionAPIWireguard::OnWireguardServiceLaunched,
            weak_factory_.GetWeakPtr()));
  }
}

void BraveVPNOSConnectionAPIWireguard::OnWireguardServiceLaunched(
    bool success) {
  if (!success) {
    VLOG(2) << __func__ << " : failed to get correct credentials";
    BraveVPNOSConnectionAPIBase::OnConnectFailed();
    return;
  }
  BraveVPNOSConnectionAPIBase::OnConnected();
}

}  // namespace brave_vpn
