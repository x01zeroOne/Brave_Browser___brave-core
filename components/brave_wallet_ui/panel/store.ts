/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { combineReducers, configureStore } from '@reduxjs/toolkit'
import { setupListeners } from '@reduxjs/toolkit/query/react'
import { persistStore } from 'redux-persist'

// handlers
import walletPanelAsyncHandler from './async/wallet_panel_async_handler'
import walletAsyncHandler from '../common/async/handlers'

// api
import getWalletPanelApiProxy from './wallet_panel_api_proxy'

// reducers
import {
  persistedWalletApiReducer,
  walletApi
} from '../common/slices/api.slice'
import { persistedWalletReducer } from '../common/slices/wallet.slice'
import { panelReducer } from './reducers/panel_reducer'

const combinedReducer = combineReducers({
  panel: panelReducer,
  wallet: persistedWalletReducer,
  [walletApi.reducerPath]: persistedWalletApiReducer
})

const store = configureStore({
  reducer: combinedReducer,
  middleware: (getDefaultMiddleware) => getDefaultMiddleware({
    serializableCheck: false
  }).concat(
    walletAsyncHandler,
    walletPanelAsyncHandler,
    walletApi.middleware
  )
})

export type PanelRootState = ReturnType<typeof combinedReducer>

const proxy = getWalletPanelApiProxy()
proxy.addJsonRpcServiceObserver(store)
proxy.addKeyringServiceObserver(store)
proxy.addTxServiceObserver(store)
proxy.addBraveWalletServiceObserver(store)

// enables refetchOnMount and refetchOnReconnect behaviors
// without this, cached data will not refresh when reloading the app
setupListeners(store.dispatch)

export const walletPanelApiProxy = proxy

export const persistor = persistStore(store)

export default store
