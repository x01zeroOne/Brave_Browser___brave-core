// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import MainPanel from './components/main-panel'
import TreeList from './components/tree-list'
import { MakeResourceInfoList, ResourceState, ResourceType, ViewType } from './state/component_types'
import DataContext from './state/context'
import styled from 'styled-components'
import { getLocale } from '../../../common/locale'

const Box = styled.div`
  position: relative;
`

function Container () {
  const { siteBlockInfo, viewType } = React.useContext(DataContext)
  const detailView = viewType !== ViewType.Main && siteBlockInfo

  const renderDetailView = () => {
    if (viewType === ViewType.AdsList && detailView) {
      return (<TreeList
        blockedList={MakeResourceInfoList(siteBlockInfo?.adsList, ResourceType.Ad, ResourceState.Blocked)}
        type={ResourceType.Ad}
        totalBlockedCount={siteBlockInfo?.adsList.length}
        blockedCountTitle={getLocale('braveShieldsTrackersAndAds')}
      />)
    }

    if (viewType === ViewType.HttpsList && detailView) {
      return (<TreeList
        type={ResourceType.Http}
        blockedList={MakeResourceInfoList(siteBlockInfo?.httpRedirectsList, ResourceType.Http, ResourceState.Blocked)}
        totalBlockedCount={siteBlockInfo?.httpRedirectsList.length}
        blockedCountTitle={getLocale('braveShieldsConnectionsUpgraded')}
      />)
    }

    if (viewType === ViewType.ScriptsList && detailView) {
      return (<TreeList
        blockedList={MakeResourceInfoList(siteBlockInfo?.blockedJsList, ResourceType.Script, ResourceState.Blocked)}
        allowedList={MakeResourceInfoList(siteBlockInfo?.allowedJsList, ResourceType.Script, ResourceState.AllowedOnce)}
        type={ResourceType.Script}
        totalBlockedCount={siteBlockInfo?.blockedJsList.length}
        blockedCountTitle={getLocale('braveShieldsBlockedScriptsLabel')}
      />)
    }

    return null
  }

  return (
    <Box>
      {renderDetailView()}
      <MainPanel />
    </Box>
  )
}

export default Container
