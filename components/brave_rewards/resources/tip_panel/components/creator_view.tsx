/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useModel } from '../lib/model_context'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { VerifiedIcon } from './icons/verified_icon'
import { LaunchIcon } from './icons/launch_icon'
import { getSocialIcon } from './icons/social_icon'
import { VerifiedTooltip } from './verified_tooltip'

import * as style from './creator_view.style'

export function CreatorView () {
  const creatorBanner = useModel(state => state.creatorBanner)

  const styleProps = {}

  if (creatorBanner.background) {
    styleProps['--creator-background-image-url'] =
      `url("${creatorBanner.background}")`
  }
  if (creatorBanner.logo) {
    styleProps['--creator-avatar-image-url'] = `url("${creatorBanner.logo}")`
  }

  function renderLink (platform: string, url: string) {
    const Icon = getSocialIcon(platform)
    if (!Icon) {
      return null
    }
    return (
      <NewTabLink key={platform} href={url}>
        <Icon />
      </NewTabLink>
    )
  }

  const renderedLinks = Object.entries(creatorBanner.links).map(
    ([key, value]) => renderLink(key, value))

  return (
    <style.root style={styleProps as React.CSSProperties}>
      <style.background />
      <style.avatar />
      <style.title>
        <style.name>
          {creatorBanner.title}
        </style.name>
        <style.verifiedCheck>
          <VerifiedIcon />
          <div className='tooltip'>
            <VerifiedTooltip />
          </div>
        </style.verifiedCheck>
      </style.title>
      <style.text>
        {creatorBanner.description}
      </style.text>
      <style.links>
        {renderedLinks}
        {renderedLinks.length > 0 && <style.linkDivider />}
        <NewTabLink href=''>
          <LaunchIcon />
        </NewTabLink>
      </style.links>
    </style.root>
  )
}
