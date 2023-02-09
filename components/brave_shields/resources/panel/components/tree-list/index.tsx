// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import DataContext from '../../state/context'
import { getLocale } from '../../../../../common/locale'
import TreeNode from './tree-node'
import { ViewType, ResourceInfo, ResourceType, ResourceState } from '../../state/component_types'
import Button from '$web-components/button'
import getPanelBrowserAPI from '../../api/panel_browser_api'

interface Props {
  blockedList: ResourceInfo[]
  allowedList?: ResourceInfo[]
  type: ResourceType
  totalBlockedCount: number
  blockedCountTitle: string
}

function groupByOrigin (data: ResourceInfo[]) {
  const map: Map<string, ResourceInfo[]> = new Map()

  const includesDupeOrigin = (searchOrigin: string) => {
    const results = data.map(entry => new URL(entry.url.url).origin)
      .filter(entry => entry.includes(searchOrigin))
    return results.length > 1
  }

  data.forEach(entry => {
    const url = new URL(entry.url.url)
    const origin = url.origin
    const items = map.get(origin)

    if (items) {
      items.push(entry)
      return // continue
    }

    // If the origin's full url is the resource itself then we show the full url as parent
    map.set(includesDupeOrigin(origin) ? origin : url.href.replace(/\/$/, ''), [])
  })

  return map
}

function getScriptsOriginsWithState (data: ResourceInfo[], state: ResourceState): string[] {
  const list: string[] = []

  data.forEach(entry => {
    const url = new URL(entry.url.url)
    if (list.includes(url.origin) || entry.state !== state) {
      return // continue
    }

    list.push(url.origin)
  })

  return list
}

function TreeList (props: Props) {
  const { siteBlockInfo, setViewType } = React.useContext(DataContext)
  const mappedBlockedScripts = React.useMemo(() => groupByOrigin(props.blockedList), [props.blockedList])
  const allowAllScripts = () => {
    const origins: string[] = getScriptsOriginsWithState(props.blockedList, ResourceState.Blocked)
    getPanelBrowserAPI().dataHandler.allowScriptsOnce(origins)
  }
  return (
    <S.Box>
      <S.HeaderBox>
        <S.SiteTitleBox>
          <S.FavIconBox>
            <img key={siteBlockInfo?.faviconUrl.url} src={siteBlockInfo?.faviconUrl.url} />
          </S.FavIconBox>
          <S.SiteTitle>{siteBlockInfo?.host}</S.SiteTitle>
        </S.SiteTitleBox>
        <S.Grid>
          <span>{props.totalBlockedCount}</span>
          <span>{props.blockedCountTitle}</span>
          <span>{<a href="#" onClick={() => allowAllScripts()}>
              {getLocale('braveShieldsAllowScriptsAll')}
            </a>
          }</span>
        </S.Grid>
      </S.HeaderBox>
      <S.TreeBox>
        <div>
          {[...mappedBlockedScripts.keys()].map((origin, idx) => {
            return (<TreeNode
              key={idx}
              host={origin}
              type={props.type}
              resourceList={mappedBlockedScripts.get(origin) ?? []}
            />)
          })}
        </div>
      </S.TreeBox>
      <S.Footer>
        <Button
          aria-label="Back to previous screen"
          onClick={() => setViewType?.(ViewType.Main)}
        >
          <svg fill="currentColor" viewBox="0 0 32 32" aria-hidden="true"><path d="M28 15H6.28l4.85-5.25a1 1 0 0 0-.05-1.42 1 1 0 0 0-1.41.06l-6.4 6.93a.7.7 0 0 0-.1.16.75.75 0 0 0-.09.15 1 1 0 0 0 0 .74.75.75 0 0 0 .09.15.7.7 0 0 0 .1.16l6.4 6.93a1 1 0 0 0 1.41.06 1 1 0 0 0 .05-1.42L6.28 17H28a1 1 0 0 0 0-2z"/></svg>
          <span>{getLocale('braveShieldsStandalone')}</span>
        </Button>
      </S.Footer>
    </S.Box>
  )
}

export default TreeList
