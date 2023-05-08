// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as SkusInternalsMojo from 'gen/brave/components/skus/common/skus_internals.mojom.m.js'
import * as React from 'react'
import { render } from 'react-dom'

const API = SkusInternalsMojo.SkusInternals.getRemote()

function App() {
  const [skusState, setSkusState] = React.useState('')

  const resetSkusState = () => {
    API.resetSkusState()
    setSkusState('')
  }

  const getSkusState = () => {
    API.getSkusState().then((r: any) => {
      setSkusState(r.response)
    })
  }

  return (
    <div>
      <h2>Skus internals</h2>
      <div>
        <input type="button" value="Reset skus state" onClick={() => { resetSkusState() }} />
        <input type="button" value="Fetch skus state" onClick={() => { getSkusState() }} />
      </div>
      <div>
        <input type="text" value={skusState} readOnly />
      </div>
    </div>
  )
}

// import { App } from './components/app'

document.addEventListener('DOMContentLoaded', () => {
  render(<App />, document.getElementById('root'))
})
