/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useLocaleContext } from '../lib/locale_strings'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { VerifiedIcon } from './icons/verified_icon'

import * as urls from '../../shared/lib/rewards_urls'

import * as style from './verified_tooltip.style'

export function VerifiedTooltip () {
  const { getString } = useLocaleContext()
  return (
    <style.root>
      <style.content>
        <style.title>
          <style.checkmark><VerifiedIcon /></style.checkmark>
          <div>{getString('verifiedTooltipTitle')}</div>
        </style.title>
        <style.text>
          {getString('verifiedTooltipText')}
          <NewTabLink href={urls.tippingLearnMoreURL}>
            {getString('learnMoreLabel')}
          </NewTabLink>
        </style.text>
        <style.arrow />
      </style.content>
    </style.root>
  )
}
