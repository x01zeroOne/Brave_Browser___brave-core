/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_main.h"

#include <windows.h>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/run_loop.h"
#include "base/path_service.h"
#include "base/task/single_thread_task_executor.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_utils.h"

namespace brave_vpn {
namespace {
HRESULT HRESULTFromLastError() {
  const auto error_code = ::GetLastError();
  return (error_code != NO_ERROR) ? HRESULT_FROM_WIN32(error_code) : E_FAIL;
}

// Command line switch "--console" runs the service interactively for
// debugging purposes.
constexpr char kConsoleSwitchName[] = "console";
}  // namespace

ServiceMain* ServiceMain::GetInstance() {
  static base::NoDestructor<ServiceMain> instance;
  return instance.get();
}

bool ServiceMain::InitWithCommandLine(const base::CommandLine* command_line) {
  VLOG(1) << __func__ << ":" << command_line->GetCommandLineString();
  const base::CommandLine::StringVector args = command_line->GetArgs();
  if (!args.empty()) {
    LOG(ERROR) << "No positional parameters expected.";
    return false;
  }

  // Run interactively if needed.
  if (command_line->HasSwitch(kConsoleSwitchName)) {
    run_routine_ = &ServiceMain::RunInteractive;
  }
  return true;
}

// Start() is the entry point called by WinMain.
int ServiceMain::Start() {
  return (this->*run_routine_)();
}

ServiceMain::ServiceMain()
    : run_routine_(&ServiceMain::RunAsService),
      service_status_handle_(nullptr),
      service_status_() {
  service_status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  service_status_.dwCurrentState = SERVICE_STOPPED;
  service_status_.dwControlsAccepted = SERVICE_ACCEPT_STOP;
}

ServiceMain::~ServiceMain() = default;

int ServiceMain::RunAsService() {
  VLOG(1) << __func__;
  const std::wstring& service_name(brave_vpn::GetVpnServiceName());
  const SERVICE_TABLE_ENTRY dispatch_table[] = {
      {const_cast<LPTSTR>(service_name.c_str()),
       &ServiceMain::ServiceMainEntry},
      {nullptr, nullptr}};
  
  if (!::StartServiceCtrlDispatcher(dispatch_table)) {
    service_status_.dwWin32ExitCode = ::GetLastError();
    LOG(ERROR) << "Failed to connect to the service control manager:"
               << service_status_.dwWin32ExitCode;
  }

  return service_status_.dwWin32ExitCode;
}

void ServiceMain::ServiceMainImpl() {
  VLOG(1) << __func__ << " BraveVPN Service started";
  service_status_handle_ =
      ::RegisterServiceCtrlHandler(brave_vpn::GetVpnServiceName().c_str(),
                                   &ServiceMain::ServiceControlHandler);
  if (service_status_handle_ == nullptr) {
    LOG(ERROR) << "RegisterServiceCtrlHandler failed";
    return;
  }
  SetServiceStatus(SERVICE_RUNNING);
  service_status_.dwWin32ExitCode = ERROR_SUCCESS;
  service_status_.dwCheckPoint = 0;
  service_status_.dwWaitHint = 0;

  // When the Run function returns, the service has stopped.
  const HRESULT hr = Run();
  if (FAILED(hr)) {
    service_status_.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    service_status_.dwServiceSpecificExitCode = hr;
  }

  SetServiceStatus(SERVICE_STOPPED);
}

int ServiceMain::RunInteractive() {
  return Run();
}

// static
void ServiceMain::ServiceControlHandler(DWORD control) {
  ServiceMain* self = ServiceMain::GetInstance();
  switch (control) {
    case SERVICE_CONTROL_STOP:
      self->SignalExit();
      break;

    default:
      break;
  }
}

// static
void WINAPI ServiceMain::ServiceMainEntry(DWORD argc, wchar_t* argv[]) {
  ServiceMain::GetInstance()->ServiceMainImpl();
}

void ServiceMain::SetServiceStatus(DWORD state) {
  ::InterlockedExchange(&service_status_.dwCurrentState, state);
  ::SetServiceStatus(service_status_handle_, &service_status_);
}

HRESULT ServiceMain::Run() {
  VLOG(1) << __func__;
 
  typedef bool WireGuardTunnelService(const LPCWSTR settings);
  base::FilePath directory;
  if (!base::PathService::Get(base::DIR_EXE, &directory))
    return S_OK;
  auto tunnel_dll_path = directory.Append(L"tunnel.dll").value();
  VLOG(1) << __func__ << ": Loading " << tunnel_dll_path;
  HMODULE tunnel_lib = LoadLibrary(tunnel_dll_path.c_str());
  if (!tunnel_lib) {
      VLOG(1) << __func__ << ": tunnel.dll not found, error: " <<  logging::SystemErrorCodeToString(logging::GetLastSystemErrorCode());
      return S_OK;
  }

  WireGuardTunnelService* tunnel_proc = reinterpret_cast<WireGuardTunnelService*>(GetProcAddress(tunnel_lib, "WireGuardTunnelService"));
  if (!tunnel_proc) {
      VLOG(1) << __func__ << ": WireGuardTunnelService not found error: " << logging::SystemErrorCodeToString(logging::GetLastSystemErrorCode());
      return S_OK;
  }

  auto config_path = directory.Append(L"brave.conf").value();
  VLOG(1) << __func__ << ": Brave " << config_path;
  auto result = tunnel_proc(config_path.c_str());
  
  if (!result) {
    VLOG(1) << __func__ << ": failed to activate tunnel service:" << logging::SystemErrorCodeToString(logging::GetLastSystemErrorCode()) << " -> " << std::hex << HRESULTFromLastError();
  }
  return S_OK;
}

void ServiceMain::SignalExit() {
  VLOG(1) << __func__;
}

}  // namespace brave_vpn
