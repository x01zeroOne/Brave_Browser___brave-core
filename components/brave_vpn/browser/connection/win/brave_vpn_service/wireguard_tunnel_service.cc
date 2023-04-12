/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/wireguard_tunnel_service.h"

#include <windows.h>
#include <winsvc.h>
#include <chrono>
#include <string>
#include <thread>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/scoped_sc_handle.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_constants.h"

namespace brave_vpn {

namespace {
constexpr wchar_t kBraveWireguardTunnelServiceName[] =
    L"BraveWireGuardTunnelService";

bool IsRunning(SC_HANDLE service) {
  SERVICE_STATUS service_status = {0};
  if (!::QueryServiceStatus(service, &service_status)) {
    return false;
  }
  return service_status.dwCurrentState == SERVICE_RUNNING;
}

HRESULT HRESULTFromLastError() {
  const auto error_code = ::GetLastError();
  return (error_code != NO_ERROR) ? HRESULT_FROM_WIN32(error_code) : E_FAIL;
}

}  // namespace

namespace wireguard {

int LaunchService(const base::FilePath& config_path) {
  LOG(ERROR) << __func__ << ":" << config_path;
  if (!RemoveExistingWireguradService()) {
    return 1;
  }
  return CreateAndRunBraveWGService(config_path);
}

bool RemoveExistingWireguradService() {
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
  if (!scm.IsValid()) {
    VLOG(1) << "::OpenSCManager failed. service_name: "
            << kBraveWireguardTunnelServiceName << ", error: " << std::hex
            << HRESULTFromLastError();
    return false;
  }
  ScopedScHandle service(::OpenService(
      scm.Get(), kBraveWireguardTunnelServiceName, SERVICE_ALL_ACCESS));

  if (service.IsValid()) {
    if (IsRunning(service.Get())) {
      SERVICE_STATUS stt;
      if (!ControlService(service.Get(), SERVICE_CONTROL_STOP, &stt)) {
        VLOG(1) << "ControlService failed";
        return false;
      }
    }
    if (!DeleteService(service.Get())) {
      VLOG(1) << "DeleteService failed";
      return false;
    }
  }
  return true;
}

int CreateAndRunBraveWGService(const base::FilePath& config_path) {
  LOG(ERROR) << __func__;
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
  if (!scm.IsValid()) {
    VLOG(1) << "::OpenSCManager failed. service_name: "
            << kBraveWireguardTunnelServiceName << ", error: " << std::hex
            << HRESULTFromLastError();
    return 1;
  }
  base::FilePath directory;
  if (!base::PathService::Get(base::DIR_EXE, &directory)) {
    return 1;
  }
  base::CommandLine service_cmd(
      directory.Append(brave_vpn::kBraveVPNServiceExecutable));
  service_cmd.AppendSwitchPath(brave_vpn::kConnectWGSwitchName, config_path);
  LOG(ERROR) << "Service command line:" << service_cmd.GetCommandLineString();
  ScopedScHandle service(::CreateService(
      scm.Get(), kBraveWireguardTunnelServiceName,
      kBraveWireguardTunnelServiceName, SERVICE_ALL_ACCESS,
      SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
      service_cmd.GetCommandLineString().c_str(), NULL, NULL, L"Nsi\0TcpIp\0",
      NULL, NULL));
  if (!service.IsValid()) {
    VLOG(1) << "::CreateService failed. service_name: "
            << kBraveWireguardTunnelServiceName << ", error: 0x"
            << ::GetLastError();
    return 1;
  }

  SERVICE_SID_INFO info;
  info.dwServiceSidType = SERVICE_SID_TYPE_UNRESTRICTED;
  if (!ChangeServiceConfig2(service.Get(), SERVICE_CONFIG_SERVICE_SID_INFO,
                            &info)) {
    VLOG(1) << "Failed to configure service 0x" << std::hex
            << HRESULTFromLastError();
    return 1;
  }

  if (!StartService(service.Get(), 0, NULL)) {
    VLOG(1) << "Failed to start service 0x" << std::hex
            << HRESULTFromLastError();
    return 1;
  }

  return !DeleteService(service.Get());
}

int RunWireGuardTunnelService(const base::FilePath& config_path) {
  typedef bool WireGuardTunnelService(const LPCWSTR settings);
  LOG(ERROR) << __func__ << ":" << config_path;
  base::FilePath directory;
  if (!base::PathService::Get(base::DIR_EXE, &directory)) {
    return 1;
  }
  auto tunnel_dll_path = directory.Append(L"tunnel.dll").value();
  LOG(ERROR) << __func__ << ": Loading " << tunnel_dll_path;
  HMODULE tunnel_lib = LoadLibrary(tunnel_dll_path.c_str());
  if (!tunnel_lib) {
    LOG(ERROR) << __func__ << ": tunnel.dll not found, error: "
               << logging::SystemErrorCodeToString(
                      logging::GetLastSystemErrorCode());
    return 1;
  }

  WireGuardTunnelService* tunnel_proc =
      reinterpret_cast<WireGuardTunnelService*>(
          GetProcAddress(tunnel_lib, "WireGuardTunnelService"));
  if (!tunnel_proc) {
    LOG(ERROR) << __func__ << ": WireGuardTunnelService not found error: "
               << logging::SystemErrorCodeToString(
                      logging::GetLastSystemErrorCode());
    return 1;
  }

  LOG(ERROR) << __func__ << ": Config: " << config_path;
  auto result = tunnel_proc(config_path.value().c_str());

  if (!result) {
    LOG(ERROR) << __func__ << ": failed to activate tunnel service:"
               << logging::SystemErrorCodeToString(
                      logging::GetLastSystemErrorCode())
               << " -> " << std::hex << HRESULTFromLastError();
  }
  return result;
}

}  // namespace wireguard
}  // namespace brave_vpn
