// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <windows.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/process/memory.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/process_startup_helper.h"
#include "chrome/install_static/product_install_details.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_constants.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_main.h"
#include "brave/components/brave_vpn/browser/connection/win/brave_vpn_service/service_utils.h"

namespace {
const char kLogFile[] = "log-file";
}  // namespace

int main(int argc, char* argv[]) {
  // Initialize the CommandLine singleton from the environment.
  base::CommandLine::Init(argc, argv);
  auto* command_line = base::CommandLine::ForCurrentProcess();
  logging::LoggingSettings settings;
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
  base::FilePath log_file_path;
  if (command_line->HasSwitch(kLogFile)) {
    settings.logging_dest |= logging::LOG_TO_FILE;
    log_file_path = command_line->GetSwitchValuePath(kLogFile);
    settings.log_file_path = log_file_path.value().c_str();
  }
  logging::InitLogging(settings);
  // The exit manager is in charge of calling the dtors of singletons.
  base::AtExitManager exit_manager;
  // Make sure the process exits cleanly on unexpected errors.
  base::win::RegisterInvalidParamHandler();
  
  base::win::SetupCRT(*command_line);
  install_static::InitializeProductDetailsForPrimaryModule();
  // Register vpn helper service in the system.
  if (command_line->HasSwitch(brave_vpn::kBraveWgServiceInstall)) {
    auto success =
        brave_vpn::ConfigureService(brave_vpn::GetVpnServiceName());
    return success ? 0 : 1;
  }

  // Run the service.
  brave_vpn::ServiceMain* service = brave_vpn::ServiceMain::GetInstance();
  if (!service->InitWithCommandLine(command_line)) {
    VLOG(1) << __func__ << ": unable to init";
    return 1;
  }
  VLOG(1) << __func__ << ": Start";
  return service->Start();
}
