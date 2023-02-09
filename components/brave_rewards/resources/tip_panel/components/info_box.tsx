/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { InfoBoxIcon } from './icons/info_box_icon'

import * as style from './info_box.style'

interface Props {
  title: string
  children: React.ReactNode
}

export function InfoBox (props: Props) {
  return (
    <style.root>
      <style.icon>
        <InfoBoxIcon />
      </style.icon>
      <style.content>
        <style.title>
          {props.title}
        </style.title>
        <style.text>
          {props.children}
        </style.text>
      </style.content>
    </style.root>
  )
}
