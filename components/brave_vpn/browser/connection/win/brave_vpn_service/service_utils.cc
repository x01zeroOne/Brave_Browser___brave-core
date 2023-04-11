/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_utils.h"

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/scoped_sc_handle.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_constants.h"
#include "chrome/installer/util/install_service_work_item.h"

namespace brave_vpn {

namespace {

HRESULT HRESULTFromLastError() {
  const auto error_code = ::GetLastError();
  return (error_code != NO_ERROR) ? HRESULT_FROM_WIN32(error_code) : E_FAIL;
}

}  // namespace

bool InstallService() {
  base::FilePath exe_dir;
  base::PathService::Get(base::DIR_EXE, &exe_dir);
  base::CommandLine service_cmd(
      exe_dir.Append(brave_vpn::kBraveVPNServiceExecutable));
  service_cmd.AppendSwitchPath("log-file", base::FilePath(L"D:\\1\\wg.log"));
  installer::InstallServiceWorkItem install_service_work_item(
      brave_vpn::GetBraveVpnServiceName(),
      brave_vpn::GetBraveVpnServiceDisplayName(), SERVICE_DEMAND_START,
      service_cmd, base::CommandLine(base::CommandLine::NO_PROGRAM),
      brave_vpn::kBraveVpnServiceRegistryStoragePath,
      {brave_vpn::GetBraveVpnServiceClsid()},
      {brave_vpn::GetBraveVpnServiceIid()});
  install_service_work_item.set_best_effort(true);
  install_service_work_item.set_rollback_enabled(false);
  if (install_service_work_item.Do()) {
    return brave_vpn::ConfigureService(brave_vpn::GetBraveVpnServiceName());
  }
  return false;
}

bool ConfigureService(const std::wstring& service_name) {
  ScopedScHandle scm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
  if (!scm.IsValid()) {
    LOG(ERROR) << "::OpenSCManager failed. service_name: " << service_name
               << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }
  base::FilePath exe_path;
  if (!base::PathService::Get(base::FILE_EXE, &exe_path)) {
    return S_OK;
  }

  ScopedScHandle service(::CreateService(
      scm.Get(), service_name.c_str(), service_name.c_str(), SERVICE_ALL_ACCESS,
      SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
      exe_path.value().c_str(), NULL, NULL, L"Nsi\0TcpIp\0", NULL, NULL));
  if (!service.IsValid()) {
    LOG(ERROR) << "Failed to create service_name: " << service_name
               << ", error: " << std::hex << HRESULTFromLastError();
    return false;
  }
  SERVICE_SID_INFO info = {};
  info.dwServiceSidType = SERVICE_SID_TYPE_UNRESTRICTED;
  if (!ChangeServiceConfig2(service.Get(), SERVICE_CONFIG_SERVICE_SID_INFO,
                            &info)) {
    LOG(ERROR) << "ChangeServiceConfig2 failed:" << std::hex
               << HRESULTFromLastError();
    return false;
  }
  if (!SetServiceFailActions(service.Get())) {
    LOG(ERROR) << "SetServiceFailActions failed:" << std::hex
               << HRESULTFromLastError();
    return false;
  }
  return true;
}

bool SetServiceFailActions(SC_HANDLE service) {
  SC_ACTION failActions[] = {
      {SC_ACTION_RESTART, 1}, {SC_ACTION_RESTART, 1}, {SC_ACTION_RESTART, 1}};
  // The time after which to reset the failure count to zero if there are no
  // failures, in seconds.
  SERVICE_FAILURE_ACTIONS servFailActions = {
      .dwResetPeriod = 0,
      .lpRebootMsg = NULL,
      .lpCommand = NULL,
      .cActions = sizeof(failActions) / sizeof(SC_ACTION),
      .lpsaActions = failActions};
  return ChangeServiceConfig2(service, SERVICE_CONFIG_FAILURE_ACTIONS,
                              &servFailActions);
}

}  // namespace brave_vpn
