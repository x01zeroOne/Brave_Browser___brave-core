/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ExternalWalletProvider } from '../../shared/lib/external_wallet'

export interface CreatorBanner {
  title: string
  description: string
  logo: string
  background: string
  links: Record<string, string>
  amounts: number[]
}

// TODO: Provider can be a Web3 provider, which means it's not really a
// provider...
export interface CreatorWallet {
  provider: ExternalWalletProvider
  address: string
}

export interface RewardsUser {
  balance: number
  walletProvider: ExternalWalletProvider | null
  walletAuthorized: boolean
}

export interface GlobalState {
  exchangeRate: number
  exchangeCurrency: string
  defaultAmounts: number[]
}

export interface ModelState {
  loading: boolean
  creatorBanner: CreatorBanner
  creatorWallets: CreatorWallet[]
  rewardsUser: RewardsUser
  globalState: GlobalState
  monthlyContributionSet: boolean
}

export function defaultState (): ModelState {
  return {
    loading: true,
    creatorBanner: {
      title: '',
      description: '',
      logo: '',
      background: '',
      links: {},
      amounts: []
    },
    creatorWallets: [],
    rewardsUser: {
      balance: 0,
      walletProvider: null,
      walletAuthorized: false
    },
    globalState: {
      exchangeRate: 1,
      exchangeCurrency: 'USD',
      defaultAmounts: [1, 5, 10]
    },
    monthlyContributionSet: false
  }
}

export type ModelStateListener = (state: ModelState) => void

export interface Model {
  state: ModelState
  addListener: (callback: ModelStateListener) => () => void
  onInitialRender: () => void
}
