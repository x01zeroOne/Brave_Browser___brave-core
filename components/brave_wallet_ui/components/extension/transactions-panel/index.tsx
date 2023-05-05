// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// Types
import { BraveWallet, SerializableTransactionInfo } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { sortTransactionByDate } from '../../../utils/tx-utils'
import { PanelActions } from '../../../panel/actions'

// Hooks
import { useGetTransactionsQuery } from '../../../common/slices/api.slice'

// Components
import { TransactionsListItem } from '../'

// Styled Components
import { ScrollContainer } from '../../../stories/style'
import { CircleIconWrapper, Column, Row, VerticalSpace } from '../../shared/style'
import {
  FillerDescriptionText,
  FillerTitleText,
  FloatAboveBottomRightCorner,
  InfoCircleIcon,
  StyledWrapper,
  TransactionsIcon
} from './style'

export interface Props {
  selectedNetwork?: BraveWallet.NetworkInfo
  selectedAccountAddress?: string
  selectedAccountCoinType?: BraveWallet.CoinType
}

export const TransactionsPanel = ({
  selectedNetwork,
  selectedAccountAddress,
  selectedAccountCoinType,
}: Props) => {

  // redux
  const dispatch = useDispatch()

  // queries
  const { data: transactionList = [] } = useGetTransactionsQuery({
    address: selectedAccountAddress || null,
    chainId: null,
    coinType: selectedAccountCoinType || null
  })

  // TODO: update panel router to handler this navigation
  const viewTransactionDetail = React.useCallback(
    (transaction: SerializableTransactionInfo) => {
      dispatch(PanelActions.setSelectedTransactionId(transaction.id))
      dispatch(PanelActions.navigateTo('transactionDetails'))
    },
    []
  )

  // memos / computed
  const sortedNonRejectedTransactionList = React.useMemo(() => {
    return sortTransactionByDate(transactionList, 'descending')
  }, [transactionList])

  // render
  if (sortedNonRejectedTransactionList.length === 0) {
    return (
      <StyledWrapper hideScrollbar>
        <Column fullHeight padding='22px'>
          <Column>

            {/* Graphic */}
            <Row>
              <CircleIconWrapper>

                <TransactionsIcon
                  size={24}
                />

                <FloatAboveBottomRightCorner>
                  <CircleIconWrapper padding={2}>
                    <InfoCircleIcon />
                  </CircleIconWrapper>
                </FloatAboveBottomRightCorner>

              </CircleIconWrapper>
            </Row>

            <VerticalSpace space='16px' />

            <Column justifyContent='flex-start' gap='8px'>
              <FillerTitleText>
                {getLocale('braveWalletNoTransactionsYet')}
              </FillerTitleText>

              <FillerDescriptionText>
                {getLocale('braveWalletNoTransactionsYetDescription')}
              </FillerDescriptionText>
            </Column>

          </Column>
        </Column>
      </StyledWrapper>
    )
  }

  // render
  return (
    <ScrollContainer>
      <StyledWrapper>
        {sortedNonRejectedTransactionList.map((transaction) =>
          <TransactionsListItem
            key={transaction.id}
            onSelectTransaction={viewTransactionDetail}
            selectedNetwork={selectedNetwork}
            transaction={transaction}
          />
        )}
      </StyledWrapper>
    </ScrollContainer>
  )
}

export default TransactionsPanel
