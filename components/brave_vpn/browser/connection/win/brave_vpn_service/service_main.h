/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_SERVICE_MAIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_SERVICE_MAIN_H_

#include <windows.h>

#include "base/functional/callback.h"
#include "base/no_destructor.h"

namespace base {
class CommandLine;
}  // namespace base

namespace brave_vpn {

class ServiceMain {
 public:
  static ServiceMain* GetInstance();

  ServiceMain(const ServiceMain&) = delete;
  ServiceMain& operator=(const ServiceMain&) = delete;

  // This function parses the command line and selects the action routine.
  bool InitWithCommandLine(const base::CommandLine* command_line);

  // Start() is the entry point called by WinMain.
  int Start();

  // Creates an out-of-proc WRL Module.
  void CreateWRLModule();

  // Registers the Service COM class factory object so other applications can
  // connect to it. Returns the registration status.
  HRESULT RegisterClassObject();

  // Unregisters the Service COM class factory object.
  void UnregisterClassObject();
  HRESULT CreateWGConnection();

 private:
  friend class base::NoDestructor<ServiceMain>;

  ServiceMain();
  ~ServiceMain();

  void SignalExit();

  // This function handshakes with the service control manager and starts
  // the service.
  int RunAsService();

  // Runs the service on the service thread.
  void ServiceMainImpl();

  // Runs as a local server for testing purposes. RunInteractive returns an
  // HRESULT, not a Win32 error code.
  int RunInteractive();

  // The control handler of the service.
  static void WINAPI ServiceControlHandler(DWORD control);

  // The main service entry point.
  static void WINAPI ServiceMainEntry(DWORD argc, wchar_t* argv[]);
  
  // Calls ::SetServiceStatus().
  void SetServiceStatus(DWORD state);

  // Handles object registration, message loop, and unregistration. Returns
  // when all registered objects are released.
  HRESULT Run();
  // Calls ::CoInitializeSecurity to allow all users to create COM objects
  // within the server.
  static HRESULT InitializeComSecurity();

  // The action routine to be executed.
  int (ServiceMain::*run_routine_)();

  // Identifier of registered class objects used for unregistration.
  DWORD cookies_[1];
  SERVICE_STATUS_HANDLE service_status_handle_;
  SERVICE_STATUS service_status_;
  base::OnceClosure quit_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_WIN_BRAVE_VPN_SERVICE_SERVICE_MAIN_H_
