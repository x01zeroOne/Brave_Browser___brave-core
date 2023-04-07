// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createApi } from '@reduxjs/toolkit/dist/query'
import { WalletState, PanelState, PageState } from '../../constants/types'

type ApiSliceRootState = ReturnType<ReturnType<typeof createApi>['reducer']>

export const apiStatePersistorWhitelist: Array<keyof ApiSliceRootState> = [
  'config',
  'mutations',
  'provided',
  'queries'
]

export const pageStatePersistorWhitelist: Array<keyof PageState> = [
  'enablingAutoPin',
  'isAutoPinEnabled',
  'nftMetadata',
  'nftMetadataError',
  'nftsPinningStatus',
  'pinStatusOverview',
  'portfolioPriceHistory',
  'selectedAsset',
  'selectedAssetCryptoPrice',
  'selectedAssetFiatPrice',
  'selectedAssetPriceHistory',
  'selectedCoinMarket',
  'selectedTimeline',
  'setupStillInProgress',
  'showAddModal',
  'showIsRestoring',
  'walletTermsAcknowledged'
]

export const panelStatePersistorWhitelist: Array<keyof PanelState> = [] // intentionally empty for now

export const walletStatePersistorWhitelist: Array<keyof WalletState> = [
  'accounts',
  'coinMarketData',
  'connectedAccounts',
  'defaultAccounts',
  'defaultCurrencies',
  'favoriteApps',
  'fullTokenList',
  'hasInitialized',
  'isFetchingPortfolioPriceHistory',
  'isFilecoinEnabled',
  'isLoadingCoinMarketData',
  'isPanelV2FeatureEnabled',
  'isSolanaEnabled',
  'isWalletBackedUp',
  'isWalletCreated',
  'knownTransactions',
  'onRampCurrencies',
  'portfolioPriceHistory',
  'selectedAccountFilter',
  'selectedAssetFilter',
  'selectedCurrency',
  'selectedNetworkFilter',
  'selectedPortfolioTimeline',
  'transactions',
  'userVisibleTokensInfo'
]