/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStateManager } from '../../shared/lib/state_manager'

import { Model, ModelState } from './model'
import { TipPanelProxy } from './tip_panel_proxy'

export function createModel (): Model {
  const stateManager = createStateManager<ModelState>({})
  const proxy = TipPanelProxy.getInstance()

  return {

    get state () {
      return stateManager.getState()
    },

    addListener (listener) {
      return stateManager.addListener(listener)
    },

    onInitialRender () {
      proxy.handler.showUI()
    }

  }
}
