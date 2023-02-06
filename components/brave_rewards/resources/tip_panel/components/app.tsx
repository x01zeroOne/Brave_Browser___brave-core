/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Model } from '../lib/model'
import { ModelContext } from '../lib/model_context'

import * as style from './app.style'

interface Props {
  model: Model
}

export function App (props: Props) {
  React.useEffect(() => { props.model.onInitialRender() }, [])
  return (
    <ModelContext.Provider value={props.model}>
      <style.root>
        Hello World
      </style.root>
    </ModelContext.Provider>
  )
}
