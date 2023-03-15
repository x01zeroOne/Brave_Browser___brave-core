/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { combineReducers, configureStore } from '@reduxjs/toolkit'
import { setupListeners } from '@reduxjs/toolkit/query/react'
import { persistStore } from 'redux-persist'

// constants

// async handlers
import walletPageAsyncHandler from './async/wallet_page_async_handler'
import walletAsyncHandler from '../common/async/handlers'

// api
import getWalletPageApiProxy from './wallet_page_api_proxy'
import { persistedWalletApiReducer, walletApi } from '../common/slices/api.slice'

// reducers
import { persistedWalletReducer } from '../common/slices/wallet.slice'
import accountsTabReducer from './reducers/accounts-tab-reducer'
import { persistedPageReducer } from './reducers/page_reducer'

const combinedReducer = combineReducers({
  accountsTab: accountsTabReducer,
  page: persistedPageReducer,
  wallet: persistedWalletReducer,
  [walletApi.reducerPath]: persistedWalletApiReducer
})


export const store = configureStore({
  reducer: combinedReducer,
  middleware: (getDefaultMiddleware) => getDefaultMiddleware({
    serializableCheck: false
  }).concat(
    walletAsyncHandler,
    walletPageAsyncHandler,
    walletApi.middleware
  )
})

export type PageRootState = ReturnType<typeof combinedReducer>
export type WalletPageRootStore = ReturnType<typeof store.getState>
export type RootStoreState = ReturnType<typeof store.getState>

const proxy = getWalletPageApiProxy()
proxy.addJsonRpcServiceObserver(store)
proxy.addKeyringServiceObserver(store)
proxy.addTxServiceObserver(store)
proxy.addBraveWalletServiceObserver(store)
proxy.addBraveWalletPinServiceObserver(store)
proxy.addBraveWalletAutoPinServiceObserver(store)

// needs to be enabled when persisting API slices
// otherwise, stale data will be shown when reloading the app
setupListeners(store.dispatch)

export const walletPageApiProxy = proxy

export const persistor = persistStore(store)

export default store
