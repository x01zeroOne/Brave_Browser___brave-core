/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'

import { LocaleContext, createLocaleContextForTesting } from '../../shared/lib/locale_context'
import { createStateManager } from '../../shared/lib/state_manager'

import { Model, ModelState, defaultState } from '../lib/model'
import { localeStrings } from '../lib/locale_strings'
import { App } from '../components/app'

export default {
  title: 'Rewards/Tipping'
}

const locale = createLocaleContextForTesting(localeStrings)

function actionLogger (name: string) {
  return (...args: any[]) => {
    console.log(name, ...args)
  }
}

function createModel (): Model {
  const stateManager = createStateManager<ModelState>({
    ...defaultState(),
    loading: false,
    monthlyContributionSet: false,
    creatorBanner: {
      title: 'Brave Software',
      description:
        'Thanks for stopping by. Brave is on a mission to fix the web by ' +
        'giving users a safer, faster and better browsing experience ' +
        'while growing support for content creators through a new ' +
        'attention-based ecosystem of rewards. Join us. It’s time to fix the ' +
        'web together!',
      logo: 'https://rewards.brave.com/LH3yQwkb78iP28pJDSSFPJwU',
      background: '',
      links: {
        twitter: 'https://twitter.com/brave',
        youtube: 'https://www.youtube.com/bravesoftware'
      },
      amounts: []
    },
    creatorWallets: [
      { provider: 'uphold', address: '' }
    ],
    rewardsUser: {
      balance: 18.75,
      walletAuthorized: true,
      walletProvider: 'uphold'
    }
  })

  return {
    get state () { return stateManager.getState() },
    addListener (listener) { return stateManager.addListener(listener) },
    onInitialRender () { actionLogger('onInitialRender') }
  }
}

const style = {
  panelFrame: styled.div`
    background: #FFFFFF;
    box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, 0.35);
    border-radius: 0px 0px 16px 16px;
  `
}

export function TipPanel () {
  const [model] = React.useState(() => createModel())
  return (
    <style.panelFrame className='brave-theme-light'>
      <LocaleContext.Provider value={locale}>
        <App model={model} />
      </LocaleContext.Provider>
    </style.panelFrame>
  )
}
