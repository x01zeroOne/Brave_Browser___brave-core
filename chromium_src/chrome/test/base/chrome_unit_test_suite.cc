/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/test/base/chrome_unit_test_suite.h"

#include "brave/common/resource_bundle_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/test/base/testing_brave_browser_process.h"
#include "build/build_config.h"
#include "chrome/install_static/product_install_details.h"

#define ChromeUnitTestSuite ChromeUnitTestSuite_ChromiumImpl
#include "src/chrome/test/base/chrome_unit_test_suite.cc"
#undef ChromeUnitTestSuite

ChromeUnitTestSuite::ChromeUnitTestSuite(int argc, char** argv)
    : ChromeUnitTestSuite_ChromiumImpl(argc, argv) {}

void ChromeUnitTestSuite::Initialize() {
#if BUILDFLAG(IS_WIN) && defined(OFFICIAL_BUILD)
  // When ChromeExtensionsBrowserClient is initialized, it needs
  install_static::InitializeProductDetailsForPrimaryModule();
#endif
  ChromeUnitTestSuite_ChromiumImpl::Initialize();

  brave::InitializeResourceBundle();
  brave::RegisterPathProvider();
}
