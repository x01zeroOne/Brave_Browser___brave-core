// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/dist/query'

// Constants
import { BraveWallet } from '../../../constants/types'

// Utils
import { getLocale } from '$web-common/locale'

// Hooks
import { useTransactionParser } from '../../../common/hooks/transaction-parser'
import { useTransactionsNetwork } from '../../../common/hooks/use-transactions-network'
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import { useGetTransactionsQuery } from '../../../common/slices/api.slice'

// Actions
import * as WalletPanelActions from '../../../panel/actions/wallet_panel_actions'

// Components
import { Panel } from '../index'
import { TransactionSubmittedOrSigned } from './submitted_or_signed'
import { TransactionComplete } from './complete'
import { TransactionFailed } from './failed'
import { Loader } from './common/common.style'
import { Skeleton } from '../../shared/loading-skeleton/styles'

interface Props {
  transactionId: string
}

export function TransactionStatus (props: Props) {
  const { transactionId } = props

  // queries
  const { tx, error: errorDetailContent } =
    useGetTransactionsQuery(
      transactionId
        ? {
            address: null,
            chainId: null,
            coinType: BraveWallet.CoinType.ETH // TODO: change to null
          }
        : skipToken,
      {
        selectFromResult: (res) => ({
          isLoading: res.isLoading,
          tx: res.data?.find((tx) => tx.id === transactionId),
          error: res.error as string | undefined
        }),
        skip: !transactionId
      }
    )

  // hooks
  const dispatch = useDispatch()
  const transactionsNetwork = useTransactionsNetwork(tx)
  const transactionParser = useTransactionParser(transactionsNetwork)
  const { transactionsQueueLength } = usePendingTransactions()

  // memos
  const transactionIntent = React.useMemo(() => {
    return tx ? transactionParser(tx).intent : ''
  }, [tx, transactionParser])

  // methods
  const viewTransactionDetail = () => dispatch(WalletPanelActions.navigateTo('transactionDetails'))
  const onClose = () =>
    dispatch(WalletPanelActions.setSelectedTransactionId(undefined))
  const completePrimaryCTAText =
    transactionsQueueLength === 0
      ? getLocale('braveWalletButtonClose')
      : getLocale('braveWalletButtonNext')

  // render
  if (!tx) {
    return <Skeleton />
  }

  if (tx.txStatus === BraveWallet.TransactionStatus.Submitted ||
      tx.txStatus === BraveWallet.TransactionStatus.Signed) {
    return (
      <TransactionSubmittedOrSigned
        headerTitle={transactionIntent}
        transaction={tx}
        onClose={onClose}
      />
    )
  }

  if (tx.txStatus === BraveWallet.TransactionStatus.Confirmed) {
    return (
      <TransactionComplete
        headerTitle={transactionIntent}
        description={getLocale('braveWalletTransactionCompleteDescription')}
        isPrimaryCTADisabled={false}
        onClose={onClose}
        onClickSecondaryCTA={viewTransactionDetail}
        onClickPrimaryCTA={onClose}
        primaryCTAText={completePrimaryCTAText}
      />
    )
  }

  if (tx.txStatus === BraveWallet.TransactionStatus.Error) {
    return (
      <TransactionFailed
        headerTitle={transactionIntent}
        isPrimaryCTADisabled={false}
        errorDetailTitle={getLocale('braveWalletTransactionFailedModalSubtitle')}
        errorDetailContent={errorDetailContent}
        onClose={onClose}
        onClickPrimaryCTA={onClose}
      />
    )
  }

  return (
    <Panel navAction={onClose} title={transactionIntent} headerStyle='slim'>
      <Loader />
    </Panel>
  )
}
