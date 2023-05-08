/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useActions, useRewardsData } from '../lib/redux_hooks'
import { PlatformContext } from '../lib/platform_context'
import { LocaleContext } from '../../shared/lib/locale_context'
import { LayoutContext } from '../lib/layout_context'
import { isExternalWalletProviderAllowed } from '../../shared/lib/external_wallet'

import PageWallet from './pageWallet'

import { VBATNotice, shouldShowVBATNotice } from '../../shared/components/vbat_notice'
import { AdsPanel } from './ads_panel'
import { AutoContributePanel } from './auto_contribute_panel'
import { TipsPanel } from './tips_panel'
import { MonthlyTipsPanel } from './monthly_tips_panel'
import { SettingsOptInForm, RewardsTourModal } from '../../shared/components/onboarding'
import { ProviderRedirectModal } from './provider_redirect_modal'
import { GrantList } from './grant_list'
import { SidebarPromotionPanel } from './sidebar_promotion_panel'
import { UnsupportedRegionNotice } from './unsupported_region_notice'
import { BatIcon } from '../../shared/components/icons/bat_icon'
import { SettingsIcon } from '../../shared/components/icons/settings_icon'

import * as style from './settings.style'

export function Settings () {
  const { isAndroid } = React.useContext(PlatformContext)
  const layoutKind = React.useContext(LayoutContext)
  const { getString } = React.useContext(LocaleContext)
  const actions = useActions()
  const rewardsData = useRewardsData((data) => data)

  const [showRewardsTour, setShowRewardsTour] = React.useState(false)

  class ScrollRefs {
    top: HTMLDivElement | null = null;
    adsPanel: HTMLDivElement | null = null;
    autoContributePanel: HTMLDivElement | null = null;
    contributionsPanel: HTMLDivElement | null = null;
  }
  const scrollRefs = React.useRef(new ScrollRefs());

  const scrollToRef = (ref: HTMLDivElement | null) => {
    if (ref) {
      ref.scrollIntoView()
    }
  }

  const isAdsSettingsPath = () => {
    return location.pathname === '/ads-settings'
  }

  const isAutoContributePath = () => {
    return location.pathname === '/auto-contribute'
  }

  const isContributionsPath = () => {
    return location.pathname === '/contributions'
  }

  const isMonthlyStatementsPath = () => {
    return location.pathname === '/monthly-statements'
  }

  const isResetPath = () => {
    return location.pathname === '/reset'
  }

  const handleURL = () => {
    const { pathname } = location

    // Used to enable Rewards directly from the Welcome UI.
    if (pathname === '/enable') {
      actions.enableRewards()
      return true
    }

    if (pathname.includes('authorization')) {
      actions.connectExternalWallet(pathname, location.search)
      return true
    }

    if (isAdsSettingsPath()) {
      scrollToRef(scrollRefs.current.adsPanel)
      return false;
    }

    if (isAutoContributePath()) {
      scrollToRef(scrollRefs.current.autoContributePanel)
      return false;
    }

    if (isContributionsPath()) {
      scrollToRef(scrollRefs.current.contributionsPanel)
      return false;
    }

    if (isMonthlyStatementsPath()) {
      scrollToRef(scrollRefs.current.top)
      return false;
    }

    if (isResetPath()) {
      scrollToRef(scrollRefs.current.top)
      actions.onModalResetOpen()
      return false;
    }

    return false
  }

  React.useEffect(() => {
    actions.getUserType()
    actions.getIsUnsupportedRegion()
    const date = new Date()
    actions.getBalanceReport(date.getMonth() + 1, date.getFullYear())
    actions.getTipTable()
    actions.getPendingContributions()
    actions.getStatement()
    actions.getAdsData()
    actions.getExcludedSites()
    actions.getCountryCode()
    actions.getRewardsParameters()
    actions.getContributionAmount()
    actions.getAutoContributeProperties()
    actions.getBalance()
    actions.fetchPromotions()
    actions.getExternalWallet()
    actions.getOnboardingStatus()
    actions.getEnabledInlineTippingPlatforms()

    if (handleURL()) {
      history.replaceState({}, '', '/')
    }
  }, [rewardsData.initializing])

  React.useEffect(() => {
    const id = setInterval(() => { actions.getBalance() }, 60000)
    return () => { clearInterval(id) }
  }, [rewardsData.initializing])

  React.useEffect(() => {
    actions.getContributeList()
    actions.getReconcileStamp()
  }, [rewardsData.enabledContribute])

  const onTakeTour = () => { setShowRewardsTour(true) }

  const canConnectAccount = () => {
    const {
      currentCountryCode,
      externalWalletProviderList,
      parameters
    } = rewardsData

    return externalWalletProviderList.some((provider) => {
      const regionInfo = parameters.walletProviderRegions[provider] || null
      return isExternalWalletProviderAllowed(currentCountryCode, regionInfo)
    })
  }

  const renderRewardsTour = () => {
    if (!showRewardsTour) {
      return null
    }

    const { adsData, externalWallet } = rewardsData

    const onDone = () => {
      setShowRewardsTour(false)
    }

    const onAdsPerHourChanged = (adsPerHour: number) => {
      actions.onAdsSettingSave('adsPerHour', adsPerHour)
    }

    const onConnectAccount = () => {
      actions.onModalConnectOpen()
    }

    const canAutoContribute =
      !(externalWallet && externalWallet.type === 'bitflyer')

    return (
      <RewardsTourModal
        layout={layoutKind}
        firstTimeSetup={false}
        adsPerHour={adsData.adsPerHour}
        canAutoContribute={canAutoContribute}
        canConnectAccount={canConnectAccount()}
        onAdsPerHourChanged={onAdsPerHourChanged}
        onConnectAccount={onConnectAccount}
        onDone={onDone}
        onClose={onDone}
      />
    )
  }

  const onManageClick = () => { actions.onModalResetOpen() }

  function renderUnsupportedRegionNotice () {
    return (
      <div>
        <style.unsupportedRegionNoticeTitle>
          <style.header>
            <style.title>
              <BatIcon />{getString('braveRewards')}
            </style.title>
          </style.header>
        </style.unsupportedRegionNoticeTitle>
        <style.unsupportedRegionNotice>
          <UnsupportedRegionNotice />
        </style.unsupportedRegionNotice>
      </div>
    )
  }

  function renderOnboarding () {
    const onEnable = () => {
      actions.enableRewards()
    }

    return (
      <style.onboarding>
        <SettingsOptInForm onTakeTour={onTakeTour} onEnable={onEnable} />
      </style.onboarding>
    )
  }

  function renderVBATNotice () {
    const { vbatDeadline } = rewardsData.parameters
    if (!shouldShowVBATNotice(rewardsData.userType, vbatDeadline)) {
      return null
    }

    const onConnect = () => { actions.onModalConnectOpen() }

    return (
      <style.vbatNotice>
        <VBATNotice
          vbatDeadline={vbatDeadline}
          canConnectAccount={canConnectAccount()}
          declaredCountry={rewardsData.currentCountryCode}
          onConnectAccount={onConnect}
        />
      </style.vbatNotice>
    )
  }

  function renderContent () {
    // Do not display content until the user's onboarding status has been
    // determined.
    if (rewardsData.showOnboarding === null) {
      return null
    }

    if (rewardsData.isUnsupportedRegion) {
      return renderUnsupportedRegionNotice()
    }

    if (rewardsData.showOnboarding) {
      // On Android a native modal is displayed when this page is accessed and
      // the user has not opted-in to Rewards. For backward-compatibility with
      // the previous Android-specific settings page, display the page content
      // underneath the modal. Note that this behavior will need to change when
      // Android is updated to force the user through onboarding before
      // displaying content.
      if (!isAndroid) {
        return renderOnboarding()
      }
    }

    return (
      <style.content ref={el => scrollRefs.current.top = el}>
        <style.main>
          <style.header>
            <style.title>
              <BatIcon />{getString('braveRewards')}
            </style.title>
            {
              !isAndroid &&
                <style.manageAction>
                  <button
                    onClick={onManageClick}
                    data-test-id='manage-wallet-button'
                  >
                    <SettingsIcon />{getString('reset')}
                  </button>
                </style.manageAction>
            }
          </style.header>
          {renderVBATNotice()}
          <style.settingGroup ref={el => scrollRefs.current.adsPanel = el}>
            <AdsPanel showConfigOnLoad={isAdsSettingsPath()} />
          </style.settingGroup>
          {
            rewardsData.userType !== 'unconnected' &&
            <style.settingGroup data-test-id='auto-contribute-settings'
              ref={el => scrollRefs.current.autoContributePanel = el}>
                <AutoContributePanel
                  showConfigOnLoad={isAutoContributePath()} />
              </style.settingGroup>
          }
          {
            rewardsData.userType !== 'unconnected' &&
              <>
                <style.settingGroup
                  ref={el => scrollRefs.current.contributionsPanel = el}>
                  <TipsPanel showConfigOnLoad={isContributionsPath()} />
                </style.settingGroup>
                <style.settingGroup>
                  <MonthlyTipsPanel />
                </style.settingGroup>
              </>
          }
        </style.main>
        <style.sidebar>
          {rewardsData.userType !== 'unconnected' && <GrantList />}
          <PageWallet layout={layoutKind}
            showActivityOnLoad={isMonthlyStatementsPath()} />
          <SidebarPromotionPanel onTakeRewardsTour={onTakeTour} />
        </style.sidebar>
      </style.content>
    )
  }

  return (
    <style.root className={`layout-${layoutKind}`}>
      <ProviderRedirectModal />
      {renderRewardsTour()}
      {renderContent()}
    </style.root>
  )
}
