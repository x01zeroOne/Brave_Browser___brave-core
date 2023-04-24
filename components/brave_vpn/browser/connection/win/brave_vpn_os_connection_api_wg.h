/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_OS_CONNECTION_API_WG_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_OS_CONNECTION_API_WG_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api_base.h"
#include "brave/components/brave_vpn/browser/connection/win/wireguard_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

class PrefService;

namespace brave_vpn {

class BraveVPNOSConnectionAPIWireguard : public BraveVPNOSConnectionAPIBase {
 public:
  BraveVPNOSConnectionAPIWireguard(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs,
      version_info::Channel channel);

  BraveVPNOSConnectionAPIWireguard(const BraveVPNOSConnectionAPIWireguard&) =
      delete;
  BraveVPNOSConnectionAPIWireguard& operator=(
      const BraveVPNOSConnectionAPIWireguard&) = delete;
  ~BraveVPNOSConnectionAPIWireguard() override;

  void FetchProfileCredentials() override;

  // BraveVPNOSConnectionAPIBase interfaces:
  void CreateVPNConnectionImpl(const BraveVPNConnectionInfo& info) override;
  void RemoveVPNConnectionImpl(const std::string& name) override;
  void ConnectImpl(const std::string& name) override;
  void DisconnectImpl(const std::string& name) override;
  void CheckConnectionImpl(const std::string& name) override;

 protected:
  void OnWireguardKeypairGenerated(
      brave_vpn::internal::WireguardKeyPair key_pair);
  // Subclass should add platform dependent impls.
  void OnGetProfileCredentials(const std::string& private_key,
                               const std::string& profile_credential,
                               bool success);
  void OnWireguardServiceLaunched(bool success);
  void OnWireguardServiceRemoved(bool success);

 private:
  raw_ptr<PrefService> local_prefs_ = nullptr;
  base::WeakPtrFactory<BraveVPNOSConnectionAPIWireguard> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_OS_CONNECTION_API_WG_H_
