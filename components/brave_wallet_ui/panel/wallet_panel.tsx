// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { PersistGate } from 'redux-persist/integration/react'
import { initLocale } from 'brave-ui'

import { loadTimeData } from '../../common/loadTimeData'
import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import store, { persistor, walletPanelApiProxy } from './store'
import * as WalletActions from '../common/actions/wallet_actions'
import Container from './container'
import { LibContext } from '../common/context/lib.context'
import * as Lib from '../common/async/lib'
import { ApiProxyContext } from '../common/context/api-proxy.context'
import { setIconBasePath } from '@brave/leo/react/icon'
setIconBasePath('chrome://resources/brave-icons')
import LoadingSkeleton from '../components/shared/loading-skeleton'

const onPanelPageRefreshInitiated = async function (
  event: KeyboardEvent
): Promise<void> {
  if ((event.ctrlKey || event.metaKey) && event.code === 'KeyR') {
    await persistor.purge()
  }
}

function App () {
  // state
  const [initialThemeType, setInitialThemeType] =
    React.useState<chrome.braveTheme.ThemeType>()
  
  // effects
  React.useEffect(() => {
    chrome.braveTheme.getBraveThemeType(setInitialThemeType)
  }, [])

  React.useEffect(() => {
    // clear the cache on force-refresh of panel
    document.addEventListener('keydown', onPanelPageRefreshInitiated)

    // cleanup
    return () => {
      document.removeEventListener('keydown', onPanelPageRefreshInitiated)
    }
  }, [])

  // render
  return (
    <Provider store={store}>
      {initialThemeType && (
        <BraveCoreThemeProvider
          initialThemeType={initialThemeType}
          dark={walletDarkTheme}
          light={walletLightTheme}
        >
          <ApiProxyContext.Provider value={walletPanelApiProxy}>
            <LibContext.Provider value={Lib}>
              <PersistGate
                persistor={persistor}
                loading={<LoadingSkeleton width={'80%'} height={'80%'} />}
              >
                <Container />
              </PersistGate>
            </LibContext.Provider>
          </ApiProxyContext.Provider>
        </BraveCoreThemeProvider>
      )}
    </Provider>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  store.dispatch(WalletActions.initialize())
  render(<App />, document.getElementById('mountPoint'))
}

document.addEventListener('DOMContentLoaded', initialize)
