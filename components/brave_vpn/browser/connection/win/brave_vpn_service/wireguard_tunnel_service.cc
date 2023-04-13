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

#include "base/base64.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/scoped_sc_handle.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_constants.h"

namespace brave_vpn {

namespace {
constexpr wchar_t kBraveWireguardTunnelServiceName[] =
    L"BraveWireGuardTunnelService";
constexpr wchar_t kBraveWireguardConfig[] = L"brave.wireguard.conf";

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

int LaunchService(const std::wstring& config) {
  LOG(ERROR) << __func__ << ":" << config;
  if (!RemoveExistingWireguradService()) {
    return 1;
  }
  return CreateAndRunBraveWGService(config);
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

int CreateAndRunBraveWGService(const std::wstring& config) {
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
  service_cmd.AppendSwitchNative(brave_vpn::kConnectWGSwitchName, config);
  service_cmd.AppendSwitchPath("log-file",
                               base::FilePath(L"d:\\1\\tunnel.log"));
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

int RunWireGuardTunnelService(const std::wstring& encoded_config) {
  std::string config;
  if (!base::Base64Decode(base::WideToUTF8(encoded_config), &config)) {
    return 1;
  }
  base::ScopedTempDir temp_dir;
  if (!temp_dir.CreateUniqueTempDir()) {
    return 1;
  }

  base::FilePath temp_file_path(
      temp_dir.GetPath().Append(kBraveWireguardConfig));
  if (!base::WriteFile(temp_file_path, config)) {
    return 1;
  }

  typedef bool WireGuardTunnelService(const LPCWSTR settings);
  LOG(ERROR) << __func__ << ":" << config;
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

  LOG(ERROR) << __func__ << ": Config: " << temp_file_path;
  auto result = tunnel_proc(temp_file_path.value().c_str());

  if (!result) {
    LOG(ERROR) << __func__ << ": failed to activate tunnel service:"
               << logging::SystemErrorCodeToString(
                      logging::GetLastSystemErrorCode())
               << " -> " << std::hex << HRESULTFromLastError();
  }
  return result;
}

bool WireGuardGenerateKeypair(std::string* public_key,
                              std::string* private_key) {
  base::FilePath directory;
  if (!base::PathService::Get(base::DIR_EXE, &directory)) {
    VLOG(1) << __func__ << ": executable path not found, error: "
            << logging::SystemErrorCodeToString(
                   logging::GetLastSystemErrorCode());
    return false;
  }
  auto tunnel_dll_path = directory.Append(L"tunnel.dll").value();
  VLOG(1) << __func__ << ": Loading " << tunnel_dll_path;
  HMODULE tunnel_lib = LoadLibrary(tunnel_dll_path.c_str());
  if (!tunnel_lib) {
    VLOG(1) << __func__ << ": tunnel.dll not found, error: "
            << logging::SystemErrorCodeToString(
                   logging::GetLastSystemErrorCode());
    return false;
  }

  typedef bool WireGuardGenerateKeypair(uint8_t[32], uint8_t[32]);
  std::vector<uint8_t> public_key_bytes(32);
  std::vector<uint8_t> private_key_bytes(32);
  WireGuardGenerateKeypair* generate_proc =
      reinterpret_cast<WireGuardGenerateKeypair*>(
          GetProcAddress(tunnel_lib, "WireGuardGenerateKeypair"));
  if (!generate_proc) {
    VLOG(ERROR) << __func__ << ": WireGuardGenerateKeypair not found error: "
                << logging::SystemErrorCodeToString(
                       logging::GetLastSystemErrorCode());
    return false;
  }
  auto result =
      generate_proc(public_key_bytes.data(), private_key_bytes.data());

  if (result) {
    return false;
  }

  *public_key = base::Base64Encode(base::span<const uint8_t>(public_key_bytes));
  *private_key =
      base::Base64Encode(base::span<const uint8_t>(private_key_bytes));
  LOG(ERROR) << "public_key:" << *public_key << " private_key:" << *private_key;
  LOG(ERROR) << private_key->size();
  LOG(ERROR) << public_key->size();
  return true;
}

}  // namespace wireguard
}  // namespace brave_vpn
