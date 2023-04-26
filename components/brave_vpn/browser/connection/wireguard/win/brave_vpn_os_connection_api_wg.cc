/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_os_connection_api_wg.h"

#include "base/check_is_test.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "net/base/network_change_notifier.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn {

using ConnectionState = mojom::ConnectionState;


std::unique_ptr<BraveVPNOSConnectionAPI> CreateBraveVPNWireguardConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel) {
  return std::make_unique<BraveVPNOSConnectionAPIWireguard>(url_loader_factory,
                                                      local_prefs, channel);
}

BraveVPNOSConnectionAPIWireguard::BraveVPNOSConnectionAPIWireguard(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel)
    : local_prefs_(local_prefs),
      url_loader_factory_(url_loader_factory),
      region_data_manager_(url_loader_factory_, local_prefs_) {
  DCHECK(url_loader_factory_ && local_prefs_);
  // Safe to use Unretained here because |region_data_manager_| is owned
  // instance.
  region_data_manager_.set_selected_region_changed_callback(base::BindRepeating(
      &BraveVPNOSConnectionAPIWireguard::NotifySelectedRegionChanged,
      base::Unretained(this)));
  region_data_manager_.set_region_data_ready_callback(
      base::BindRepeating(&BraveVPNOSConnectionAPIWireguard::NotifyRegionDataReady,
                          base::Unretained(this)));
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

BraveVPNOSConnectionAPIWireguard::~BraveVPNOSConnectionAPIWireguard() {}

/*
void BraveVPNOSConnectionAPIWireguard::DisconnectImpl(const std::string& name) {
  // TODO(spylogsster)
  LOG(ERROR) << __func__;
  brave_vpn::internal::StopVpnWGService(base::BindOnce(
      &BraveVPNOSConnectionAPIWireguard::OnWireguardServiceRemoved,
      weak_factory_.GetWeakPtr()));
}
*/
void BraveVPNOSConnectionAPIWireguard::LoadSubscriberCredentials() {
  VLOG(1) << __func__;
}

void BraveVPNOSConnectionAPIWireguard::OnWireguardServiceRemoved(bool success) {
  if (!success) {
    VLOG(1) << __func__ << " : failed to get correct credentials";

    return;
  }

}

std::string BraveVPNOSConnectionAPIWireguard::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEnvironment);
}

void BraveVPNOSConnectionAPIWireguard::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  // It's rare but sometimes Brave doesn't get vpn status update from OS.
  // Checking here will make vpn status update properly in that situation.
  VLOG(1) << __func__ << " : " << type;
  CheckConnection();
}

BraveVpnAPIRequest* BraveVPNOSConnectionAPIWireguard::GetAPIRequest() {
  if (!url_loader_factory_) {
    CHECK_IS_TEST();
    return nullptr;
  }

  if (!api_request_) {
    api_request_ = std::make_unique<BraveVpnAPIRequest>(url_loader_factory_);
  }

  return api_request_.get();
}

void BraveVPNOSConnectionAPIWireguard::FetchProfileCredentials() {
  if (!GetAPIRequest()) {
    return;
  }

  // Get profile credentials it to create OS VPN entry.
  VLOG(1) << __func__ << " : request profile credential:"
          << GetBraveVPNPaymentsEnv(GetCurrentEnvironment());

  brave_vpn::internal::WireGuardGenerateKeypair(base::BindOnce(
      &BraveVPNOSConnectionAPIWireguard::OnWireguardKeypairGenerated,
      weak_factory_.GetWeakPtr()));
}

void BraveVPNOSConnectionAPIWireguard::UpdateAndNotifyConnectionStateChange(
    ConnectionState state) {
  // this is a simple state machine for handling connection state
  if (connection_state_ == state)
    return;


  connection_state_ = state;
  for (auto& obs : observers_)
    obs.OnConnectionStateChanged(connection_state_);
}

void BraveVPNOSConnectionAPIWireguard::OnWireguardKeypairGenerated(
    brave_vpn::internal::WireguardKeyPair key_pair) {
  if (!key_pair.has_value()) {
    VLOG(1) << __func__ << " : failed to get keypair";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }
  const auto [public_key, private_key] = key_pair.value();

  GetAPIRequest()->GetWireguardProfileCredentials(
      base::BindOnce(&BraveVPNOSConnectionAPIWireguard::OnGetProfileCredentials,
                     base::Unretained(this), private_key),
      GetSubscriberCredential(local_prefs_), public_key, GetHostname());
}

void BraveVPNOSConnectionAPIWireguard::OnGetProfileCredentials(
    const std::string& client_private_key,
    const std::string& profile_credential,
    bool success) {

  if (!success) {
    VLOG(1) << __func__ << " : failed to get profile credential";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  absl::optional<base::Value> value =
      base::JSONReader::Read(profile_credential);
  if (value.has_value()) {
    auto* server_public_key =
        value->GetDict().FindStringByDottedPath("server-public-key");
    auto* mapped_pi4_address =
        value->GetDict().FindStringByDottedPath("mapped-ipv4-address");
    if (!server_public_key || !mapped_pi4_address) {
      VLOG(1) << __func__ << " : failed to get correct credentials";
      UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
      return;
    }
    auto vpn_server_hostname = GetHostname();
    LOG(ERROR) << profile_credential;
    auto config = brave_vpn::internal::CreateWireguardConfig(
        client_private_key, *server_public_key, vpn_server_hostname,
        *mapped_pi4_address, "1.1.1.1");
    if (!config.has_value()) {
      VLOG(1) << __func__ << " : failed to get correct credentials";
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
    VLOG(1) << __func__ << " : failed to get correct credentials";
    return;
  }

}

mojom::ConnectionState BraveVPNOSConnectionAPIWireguard::GetConnectionState() const {
  return connection_state_;
}

void BraveVPNOSConnectionAPIWireguard::ResetConnectionState() {
  
}

void BraveVPNOSConnectionAPIWireguard::RemoveVPNConnection() {
  
}

void BraveVPNOSConnectionAPIWireguard::Connect() {
  
}

void BraveVPNOSConnectionAPIWireguard::Disconnect() {
  
}

void BraveVPNOSConnectionAPIWireguard::ToggleConnection() {
  
}

void BraveVPNOSConnectionAPIWireguard::CheckConnection() {
}

void BraveVPNOSConnectionAPIWireguard::ResetConnectionInfo() {
  
}

std::string BraveVPNOSConnectionAPIWireguard::GetHostname() const {
  return std::string();
}

void BraveVPNOSConnectionAPIWireguard::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveVPNOSConnectionAPIWireguard::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BraveVPNOSConnectionAPIWireguard::SetConnectionState(mojom::ConnectionState state) {
  
}

std::string BraveVPNOSConnectionAPIWireguard::GetLastConnectionError() const {
  return std::string();
}

BraveVPNRegionDataManager& BraveVPNOSConnectionAPIWireguard::GetRegionDataManager() {
  return region_data_manager_;
}

void BraveVPNOSConnectionAPIWireguard::SetSelectedRegion(const std::string& name) {  
}

void BraveVPNOSConnectionAPIWireguard::NotifyRegionDataReady(bool ready) const {
  for (auto& obs : observers_) {
    obs.OnRegionDataReady(ready);
  }
}

void BraveVPNOSConnectionAPIWireguard::NotifySelectedRegionChanged(
    const std::string& name) const {
  for (auto& obs : observers_) {
    obs.OnSelectedRegionChanged(name);
  }
}


}  // namespace brave_vpn
