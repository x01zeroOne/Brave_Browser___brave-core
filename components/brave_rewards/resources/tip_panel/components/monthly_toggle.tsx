/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useLocaleContext } from '../lib/locale_strings'
import { formatMessage } from '../../shared/lib/locale_context'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { InfoBox } from './info_box'
import { ToggleButton } from '../../shared/components/toggle_button'
import { InfoIcon } from './icons/info_icon'

import * as urls from '../../shared/lib/rewards_urls'

import * as style from './monthly_toggle.style'

interface Props {
  monthlySet: boolean
  checked: boolean
  onChange: (checked: boolean) => void
}

export function MonthlyToggle (props: Props) {
  const { getString } = useLocaleContext()
  if (props.monthlySet) {
    return (
      <InfoBox title={getString('monthlySetTitle')}>
        {
          formatMessage(getString('monthlySetText'), {
            tags: {
              $1: (content) => (
                <NewTabLink key='link' href={urls.settingsURL}>
                  {content}
                </NewTabLink>
              )
            }
          })
        }
      </InfoBox>
    )
  }
  return (
    <style.root>
      <style.label>
        {getString('monthlyToggleLabel')}
      </style.label>
      <style.info>
        <InfoIcon />
      </style.info>
      <ToggleButton checked={props.checked} onChange={props.onChange} />
    </style.root>
  )
}
