/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Model, ModelStateListener } from './model'

export const ModelContext = React.createContext({} as Model)

// A helper hook for listening to model state changes
export function useModelListener (model: Model, listener: ModelStateListener) {
  React.useEffect(() => model.addListener(listener), [model])
}
