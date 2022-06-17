/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_UNIT_TEST_SUITE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_UNIT_TEST_SUITE_H_

#define ChromeUnitTestSuite ChromeUnitTestSuite_ChromiumImpl
#include "src/chrome/test/base/chrome_unit_test_suite.h"
#undef ChromeUnitTestSuite

class ChromeUnitTestSuite : public ChromeUnitTestSuite_ChromiumImpl {
 public:
  ChromeUnitTestSuite(int argc, char** argv);

  // base::TestSuite overrides:
  void Initialize() override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_UNIT_TEST_SUITE_H_

