// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createApi } from '@reduxjs/toolkit/dist/query'
import { createTransform } from 'redux-persist'
import { QueryState } from '@reduxjs/toolkit/dist/query/core/apiState'

// types
import type { WalletState, PanelState, PageState } from '../../constants/types'
import type {
  WalletApiQueryEndpointName,
  WalletApiSliceState
} from '../slices/api.slice'

type Subset<T, U extends T> = U;

type ApiSliceRootState = ReturnType<ReturnType<typeof createApi>['reducer']>

type BlacklistedApiSliceRootStateKeys = Subset<
  keyof ApiSliceRootState,
  | 'mutations' // do not persist mutations

  // do not persist subscriptions
  // since subscribing components may no longer exist
  | 'subscriptions'
>

export const apiStatePersistorWhitelist: Array<
  Exclude<keyof ApiSliceRootState, BlacklistedApiSliceRootStateKeys>
> = [
  'config', // persist the cache configuration
  'queries' // fulfilled (whitelisted) queries
]

type BlacklistedWalletApiQueryEndpointName = Subset<
  WalletApiQueryEndpointName,
  | 'getAllPendingTransactions' // prevent tampering with Pending Txs
  | 'getDefaultAccountAddresses' // defaults may have changed since last launch
  | 'getDefaultFiatCurrency' // defaults may have changed since last launch
  | 'getGasEstimation1559' // network fees constantly change
  | 'getSelectedAccountAddress' // selections may have changed
  | 'getSelectedChain' // selections may have changed
  | 'getSelectedPendingTransactionId' // selections may have changed
  | 'getSolanaEstimatedFee' // network fees constantly change
>

type WhitelistedWalletApiQueryEndpointName = Exclude<
  WalletApiQueryEndpointName,
  BlacklistedWalletApiQueryEndpointName
>

const apiEndpointCacheWhitelist: WhitelistedWalletApiQueryEndpointName[] = [
  'getAccountInfosRegistry', // persist wallet addresses
  'getAccountTokenCurrentBalance', // persist current balances
  'getAddressByteCode', // persist contract bytecode lookups
  'getAllTransactionInfosForAddressCoinType', // persist completed txs for address
  'getAllTransactionsForAddressCoinType', // persist completed txs for address
  'getCombinedTokenBalanceForAllAccounts', // persist current combined balances
  'getERC721Metadata', // persist nft metadata
  'getNetworksRegistry', // persist list of networks
  'getSwapSupportedNetworkIds', // persist which networks support Brave Swap
  'getTokenBalancesForChainId', // persist token balances on each chain
  'getTokenSpotPrice', // persist token spot price
  'getTokensRegistry', // persist known tokens registry
  'getUserTokensRegistry', // persist user tokens registry
]

type BlacklistedPageStateKey = Subset<
  keyof PageState,
  | 'enablingAutoPin' // unused state
  | 'hasInitialized' // unused state
  | 'importAccountError' // don't persist import errors
  | 'importWalletAttempts' // do not rely on persisted storage for attempts
  | 'importWalletError' // don't persist import errors
  | 'invalidMnemonic' // don't persist errors
  | 'isAutoPinEnabled' // selection may have changed
  | 'isCryptoWalletsInitialized' // selection may have changed
  | 'isImportWalletsCheckComplete' // importable wallets may have changed
  | 'isLocalIpfsNodeRunning' // selection may have changed
  | 'isMetaMaskInitialized' // selection may have changed
  | 'mnemonic' // do not store private data
  | 'nftMetadataError' // do not persist errors
  | 'nftsPinningStatus' // pinning may have been disabled since last launch
  | 'pinStatusOverview' // unused state
  | 'setupStillInProgress' // start onboarding again if not completed
  | 'showRecoveryPhrase' // do not show private data on app relaunch
>

export const pageStatePersistorWhitelist: Array<
  Exclude<
    keyof PageState,
    BlacklistedPageStateKey
  >
> = [
  'isFetchingNFTMetadata', // save NFT metadata
  'isFetchingPriceHistory', // save price history
  'nftMetadata', // save NFT metadata
  'portfolioPriceHistory', // save portfolio historical price data
  'selectedAsset', // save asset selection
  'selectedAssetCryptoPrice', // save crypto price of selected asset
  'selectedAssetFiatPrice', // save fiat price of selected asset
  'selectedAssetPriceHistory', // save price history of selected asset
  'selectedCoinMarket', // save selection
  'selectedTimeline', // save selection
  'showAddModal', // allow resuming adding an asset
  'showIsRestoring', // allow resuming wallet restoration
  'walletTermsAcknowledged' // persist terms acknowledgment
]

type BlacklistedPanelStateKey = Subset<
  keyof PanelState,
  | 'addChainRequest' // prevent tampering with pending requests
  | 'connectingAccounts' // prevent tampering with pending connections
  | 'connectToSiteOrigin' // prevent tampering with pending connections
  | 'decryptRequest' // prevent tampering with pending requests
  | 'getEncryptionPublicKeyRequest' // prevent tampering with pending requests
  | 'hardwareWalletCode' // prevent tampering with pending connections
  | 'hasInitialized' // unused state
  | 'lastSelectedPanel' // ux choice, always launch to home panel
  | 'panelTitle'  // ux choice, always launch to home panel
  | 'selectedPanel'  // ux choice, always launch to home panel
  | 'selectedTransaction' // prevent tampering with pending transactions
  | 'signAllTransactionsRequests' // prevent tampering with pending transactions
  | 'signMessageData' // prevent tampering with pending signature requests
  | 'signTransactionRequests' // prevent tampering with pending sign requests
  | 'suggestedTokenRequest' // prevent tampering token additions
  | 'switchChainRequest' // prevent tampering with switch chain requests
>

// intentionally empty for now
export const panelStatePersistorWhitelist: Array<
  Exclude<keyof PanelState, BlacklistedPanelStateKey>
> = []

type BlacklistedWalletStateKey = Subset<
  keyof WalletState,
  | 'activeOrigin' // prevent tampering with active origin
  | 'addUserAssetError' // don't persist errors
  | 'assetAutoDiscoveryCompleted' // always check for new assets
  | 'coinMarketData' // always show latest market data
  | 'connectedAccounts' // prevent tampering with connections
  | 'defaultAccounts' // may have changed since app relaunch
  | 'defaultCurrencies' // may have changed from Web3 settings
  | 'defaultEthereumWallet' // may have changed since app relaunch
  | 'defaultSolanaWallet' // may have changed since app relaunch
  | 'favoriteApps' // prevent tampering with favorites
  | 'gasEstimates' // gas prices change constantly
  | 'hasFeeEstimatesError' // skip persisting errors
  | 'hasIncorrectPassword' // skip persisting errors
  | 'isFilecoinEnabled' // may have changed from flags
  | 'isLoadingCoinMarketData' // always show latest market data
  | 'isMetaMaskInstalled' // may have changed since app relaunch
  | 'isNftPinningFeatureEnabled' // may have changed from Web3 settings
  | 'isPanelV2FeatureEnabled' // may have changed from flags
  | 'isSolanaEnabled' // may have changed from flags
  | 'isSolanaEnabled' // may have changed from flags
  | 'isWalletLocked' // prevent unlock without password
  | 'knownTransactions' // unused state
  | 'passwordAttempts' // prevent tampering with attempts count
  | 'solFeeEstimates' // gas prices change constantly
  | 'transactionProviderErrorRegistry' // skip persisting errors

>

export const walletStatePersistorWhitelist: Array<
  Exclude<keyof WalletState, BlacklistedWalletStateKey>
> = [
  'accounts', // save wallet addresses
  'fullTokenList', // save known tokens list
  'hasInitialized', // save initialization state
  'isFetchingPortfolioPriceHistory', // save loading status of price history
  'isWalletBackedUp', // save acknowledgement of wallet backup
  'isWalletCreated', // save that the wallet was created
  'onRampCurrencies', // save onramp currency list
  'portfolioPriceHistory', // save portfolio price history
  'selectedAccountFilter', // save selection
  'selectedAssetFilter', // save selection
  'selectedCurrency', // save selection
  'selectedNetworkFilter', // save selection
  'selectedPortfolioTimeline', // save selection
  'transactions', // save transactions list
  'userVisibleTokensInfo' // save user tokens list
]

/**
 * Used to transform the state before persisting the value to storage
 */
export const privacyAndSecurityTransform = createTransform<
  WalletApiSliceState[keyof WalletApiSliceState],
  WalletApiSliceState[keyof WalletApiSliceState],
  WalletApiSliceState,
  WalletApiSliceState
>(
  // transform state before it is serialized and persisted
  (stateToSerialize, key) => {
    if (key === 'queries') {
      return getWhitelistedQueryData(
        stateToSerialize as WalletApiSliceState['queries']
      )
    }
    return {}
  },
  // transform before rehydration
  (stateToRehydrate, key) => {
    if (key === 'queries') {
      return getWhitelistedQueryData(
        stateToRehydrate as WalletApiSliceState['queries']
      )
    }
    return {}
  },
  {
    whitelist: ['queries'] as Array<
      keyof ReturnType<ReturnType<typeof createApi>['reducer']>
    >
  }
)

function getWhitelistedQueryData(stateToSerialize: QueryState<any>) {
  return (Object.entries(stateToSerialize) || [])
    .filter(([queryKey, queryValue]) => {
      return apiEndpointCacheWhitelist.includes(
        (queryValue?.endpointName ||
          '') as WhitelistedWalletApiQueryEndpointName
      )
    })
    .reduce(
      (acc, [queryKey, queryValue]) => ({ ...acc, [queryKey]: queryValue }),
      {}
    )
}
