// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation } from 'react-router-dom'

// Types
import { WalletRoutes } from '../../../constants/types'

// Selectors
import { WalletSelectors } from '../../../common/selectors'

// Hooks
import { useSafeWalletSelector } from '../../../common/hooks/use-safe-selector'

// Options
import { AllNavOptions } from '../../../options/nav-options'

// Components
import { WalletNav } from '../wallet-nav/wallet-nav'
import {
  FeatureRequestButton
} from '../../shared/feature-request-button/feature-request-button'
import {
  TabHeader
} from '../../../page/screens/shared-screen-components/tab-header/tab-header'

// Styles
import {
  Wrapper,
  LayoutCardWrapper,
  ContainerCard,
  StaticBackground,
  BackgroundGradientWrapper,
  BackgroundGradientTopLayer,
  BackgroundGradientMiddleLayer,
  BackgroundGradientBottomLayer,
  BlockForHeight,
  FeatureRequestButtonWrapper,
  CardHeaderWrapper,
  CardHeader,
  CardHeaderShadow,
  CardHeaderContentWrapper
} from './wallet-page-wrapper.style'

export interface Props {
  wrapContentInBox?: boolean
  cardWidth?: number
  noPadding?: boolean
  noCardPadding?: boolean
  hideBackground?: boolean
  hideNav?: boolean
  hideHeader?: boolean
  cardHeader?: JSX.Element | undefined | null
  children?: React.ReactNode
}

export const WalletPageWrapper = (props: Props) => {
  const {
    children,
    cardWidth,
    noPadding,
    noCardPadding,
    wrapContentInBox,
    cardHeader,
    hideBackground,
    hideNav,
    hideHeader
  } = props

  // Routing
  const { pathname: walletLocation } = useLocation()

  // Wallet Selectors (safe)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)

  // State
  const [headerShadowOpacity, setHeaderShadowOpacity]
    = React.useState<number>(0)
  const [headerDividerOpacity, setHeaderDividerOpacity]
    = React.useState<number>(1)
  const [headerHeight, setHeaderHeight] = React.useState<number>(0)

  // Refs
  let scrollRef = React.useRef<HTMLDivElement | null>(null)
  const headerRef = React.createRef<HTMLDivElement>()

  // Computed

  const headerTitle = AllNavOptions.find((option) =>
    walletLocation.includes(option.route))?.name ?? ''

  React.useEffect(() => {
    // Keeps track of the Header height to update
    // the card top position and headers shadow.
    setHeaderHeight(headerRef?.current?.clientHeight ?? 0)
  }, [headerRef?.current?.clientHeight])

  const onScroll = () => {
    const scrollPosition = scrollRef.current
    if (scrollPosition !== null) {
      const { scrollTop } = scrollPosition
      if (scrollTop === 0) {
        setHeaderShadowOpacity(0)
        setHeaderDividerOpacity(1)
        return
      }
      if (scrollTop <= 64) {
        setHeaderShadowOpacity((scrollTop / 8) * 0.01)
        setHeaderDividerOpacity(
          (100 - ((100 / 64) * scrollTop)) * 0.01
        )
        return
      }
      setHeaderShadowOpacity(0.08)
      setHeaderDividerOpacity(0)
    }
  }

  return (
    <>
      <StaticBackground />
      {!hideBackground &&
        <BackgroundGradientWrapper>
          <BackgroundGradientTopLayer />
          <BackgroundGradientMiddleLayer />
          <BackgroundGradientBottomLayer />
        </BackgroundGradientWrapper>
      }
      <Wrapper noPadding={noPadding}>
        {
          isWalletCreated &&
          !isWalletLocked &&
          !hideHeader &&
          <TabHeader title={headerTitle} />
        }
        {
          isWalletCreated &&
          !isWalletLocked &&
          !hideNav &&
          <WalletNav isSwap={walletLocation === WalletRoutes.Swap} />
        }
        {!isWalletLocked &&
          <FeatureRequestButtonWrapper>
            <FeatureRequestButton />
          </FeatureRequestButtonWrapper>
        }
        <BlockForHeight />

        {wrapContentInBox ? (
          <LayoutCardWrapper
            ref={scrollRef}
            onScroll={onScroll}
            hideCardHeader={!cardHeader}
            headerHeight={headerHeight}
          >
            {cardHeader &&
              <CardHeaderWrapper>
                <CardHeaderShadow
                  headerHeight={headerHeight}
                />
              </CardHeaderWrapper>
            }

            <ContainerCard
              noPadding={noCardPadding}
              maxWidth={cardWidth}
              hideCardHeader={!cardHeader}
            >
              {children}
            </ContainerCard>

            {cardHeader &&
              <CardHeaderWrapper
                ref={headerRef}
              >
                <CardHeader
                  shadowOpacity={headerShadowOpacity}
                >
                  <CardHeaderContentWrapper
                    dividerOpacity={headerDividerOpacity}
                  >
                    {cardHeader}
                  </CardHeaderContentWrapper>
                </CardHeader>
              </CardHeaderWrapper>
            }
          </LayoutCardWrapper>
        ) : (
          children
        )}
      </Wrapper>
    </>
  )
}

export default WalletPageWrapper
