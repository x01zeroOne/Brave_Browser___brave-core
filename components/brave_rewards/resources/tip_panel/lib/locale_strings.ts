/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Locale, LocaleContext } from '../../shared/lib/locale_context'

export const localeStrings = {
  balanceLabel: 'Rewards balance',
  monthlyToggleLabel: 'Set as monthly contribution',
  feeNotice: 'Brave collects 5% of the sent amount as a processing fee.',
  termsOfService: 'By proceeding, you agree to the $1Terms of Service$2 and $3Privacy Policy$4.',
  sendFormTitle: 'Support this creator',
  sendButtonLabel: 'Send',
  web3ButtonLabel: 'Send with Web3 Wallet',
  verifiedTooltipTitle: 'Verified Creator',
  verifiedTooltipText: 'This Creator has registered with Brave and can receive contributions from Brave Rewards users.',
  learnMoreLabel: 'Learn more',
  customAmountLabel: 'Custom',
  monthlySetTitle: 'You\'re already sending monthly contributions to this creator',
  monthlySetText: 'You can send more now, and you can cancel your monthly contributions to this Creator by removing them from $1Monthly Contributions$2.',
  providerMismatchTitle: 'Can\'t send your contribution',
  providerMismatchText: 'You\'re connected to $1, but this creator is connected to $2. This means your tip can\'t reach this creator.'
}

export type StringKey = keyof typeof localeStrings

export function useLocaleContext () {
  return React.useContext<Locale<StringKey>>(LocaleContext)
}
