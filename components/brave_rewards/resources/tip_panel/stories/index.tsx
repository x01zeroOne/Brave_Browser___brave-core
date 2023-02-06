/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, createLocaleContextForTesting } from '../../shared/lib/locale_context'
import { createStateManager } from '../../shared/lib/state_manager'

import { Model, ModelState } from '../lib/model'
import { localeStrings } from './locale_strings'
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
  const stateManager = createStateManager<ModelState>({})
  return {
    get state () { return stateManager.getState() },
    addListener (listener) { return stateManager.addListener(listener) },
    onInitialRender () { actionLogger('onInitialRender') }
  }
}

export function TipPanel () {
  const [model] = React.useState(() => createModel())
  return (
    <div className='brave-theme-light'>
      <LocaleContext.Provider value={locale}>
        <App model={model} />
      </LocaleContext.Provider>
    </div>
  )
}
