// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as EthereumBlockies from 'ethereum-blockies'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/dist/query'

// Hooks
import { useExplorer } from '../../../common/hooks'
import {
  useGetNetworkQuery,
  useGetTransactionsQuery,
  walletApi
} from '../../../common/slices/api.slice'
import { useUnsafeUISelector } from '../../../common/hooks/use-safe-selector'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import {
  getTransactionStatusString,
  getTransactionToAddress,
  isFilecoinTransaction,
  isSolanaTransaction,
  isSwapTransaction
} from '../../../utils/tx-utils'
import { toProperCase } from '../../../utils/string-utils'
import Amount from '../../../utils/amount'
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { getLocale } from '../../../../common/locale'
import { UISelectors } from '../../../common/selectors'
import { serializedTimeDeltaToJSDate } from '../../../utils/datetime-utils'

// Constants
import {
  BraveWallet,
  // WalletAccountType,
  DefaultCurrencies,
} from '../../../constants/types'

// Styled Components
import {
  StyledWrapper,
  OrbContainer,
  FromCircle,
  ToCircle,
  DetailRow,
  DetailTitle,
  DetailButton,
  StatusRow,
  BalanceColumn,
  TransactionValue,
  PanelDescription,
  SpacerText,
  FromToRow,
  AccountNameText,
  ArrowIcon,
  AlertIcon
} from './style'
import {
  DetailTextDarkBold,
  DetailTextDark
} from '../shared-panel-styles'
import Header from '../../buy-send-swap/select-header'
import { StatusBubble } from '../../shared/style'
import { TransactionStatusTooltip } from '../transaction-status-tooltip'
import { Tooltip } from '../../shared'
import { Skeleton } from '../../shared/loading-skeleton/styles'
import { useTransactionParser } from '../../../common/hooks/transaction-parser'


export interface Props {
  transactionId: string
  visibleTokens: BraveWallet.BlockchainToken[]
  transactionSpotPrices: BraveWallet.AssetPrice[]
  defaultCurrencies: DefaultCurrencies
  onBack: () => void
}

const TransactionDetailPanel = (props: Props) => {
  // props
  const {
    transactionId,
    defaultCurrencies,
    onBack,
  } = props

  // redux
  const dispatch = useDispatch()
  const transactionProviderErrorRegistry = useUnsafeUISelector(
    UISelectors.transactionProviderErrorRegistry
  )

  // queries
  const { transaction } = useGetTransactionsQuery(
    {
      address: null,
      chainId: null,
      coinType: null
    },
    {
      selectFromResult: (res) => ({
        transactions: res.data,
        transaction: res.data?.find((tx) => tx.id === transactionId)
      }),
      skip: !transactionId
    }
  )

  const { data: transactionsNetwork } = useGetNetworkQuery(
    transaction
      ? {
          chainId: transaction.chainId,
          coin: getCoinFromTxDataUnion(transaction.txDataUnion)
        }
      : skipToken
  )

  // custom hooks
  const parseTransaction = useTransactionParser(transactionsNetwork)

  // memos
  const transactionDetails = React.useMemo(
    () => transaction ? parseTransaction(transaction) : undefined,
    [transaction, parseTransaction]
  )

  const { txType } = transaction || {}
  const {
    erc721BlockchainToken,
    fiatValue,
    gasFee,
    gasFeeFiat,
    isApprovalUnlimited,
    value: normalizedTransferredValue,
    recipient,
    recipientLabel,
    senderLabel,
    symbol,
  } = transactionDetails || {}

  const fromOrb = React.useMemo(() => {
    return EthereumBlockies.create({
      seed: transaction?.fromAddress.toLowerCase(),
      size: 8,
      scale: 16
    }).toDataURL()
  }, [transaction?.fromAddress])

  const to = transaction ? getTransactionToAddress(transaction) : ''

  const toOrb = React.useMemo(() => {
    return EthereumBlockies.create({
      seed: to.toLowerCase(),
      size: 8,
      scale: 16
    }).toDataURL()
  }, [to])

  const transactionTitle = React.useMemo((): string => {
    if (!transaction) {
      return ''
    }
    if (isSwapTransaction(transaction)) {
      return toProperCase(getLocale('braveWalletSwap'))
    }
    if (transaction?.txType === BraveWallet.TransactionType.ERC20Approve) {
      return toProperCase(getLocale('braveWalletApprovalTransactionIntent'))
    }
    return toProperCase(getLocale('braveWalletTransactionSent'))
  }, [transaction])

  const transactionValue = React.useMemo((): string => {
    if (txType !== undefined || !normalizedTransferredValue) {
      return ''
    }

    if (
      txType === BraveWallet.TransactionType.ERC721TransferFrom ||
      txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
    ) {
      return erc721BlockchainToken?.name + ' ' + erc721BlockchainToken?.tokenId
    }

    if (
      txType === BraveWallet.TransactionType.ERC20Approve &&
      isApprovalUnlimited
    ) {
      return `${getLocale('braveWalletTransactionApproveUnlimited')} ${symbol}`
    }

    return new Amount(normalizedTransferredValue).formatAsAsset(
      undefined,
      symbol
    )
  }, [
    erc721BlockchainToken,
    isApprovalUnlimited,
    symbol,
    txType,
    normalizedTransferredValue
  ])

  const transactionFiatValue = React.useMemo((): string => {
    if (!txType !== undefined || !fiatValue) {
      return ''
    }

    if (
      txType !== BraveWallet.TransactionType.ERC721TransferFrom &&
      txType !== BraveWallet.TransactionType.ERC721SafeTransferFrom &&
      txType !== BraveWallet.TransactionType.ERC20Approve
    ) {
      return new Amount(fiatValue).formatAsFiat(defaultCurrencies.fiat)
    }
    return ''
  }, [fiatValue, txType, defaultCurrencies.fiat])

  const isSolanaTxn = transaction ? isSolanaTransaction(transaction) : undefined
  const isFilTransaction = transaction
    ? isFilecoinTransaction(transaction)
    : undefined
  const txError = transactionProviderErrorRegistry[transactionId]

  // methods
  const onClickViewOnBlockExplorer = useExplorer(transactionsNetwork)

  const onClickRetryTransaction = () => {
    if (transaction) {
      dispatch(
        walletApi.endpoints.retryTransaction.initiate({
          chainId: transaction.chainId,
          coinType: getCoinFromTxDataUnion(transaction.txDataUnion),
          fromAddress: transaction.fromAddress,
          transactionId: transaction.id
        })
      )
    }
  }

  const onClickSpeedupTransaction = () => {
    if (transaction) {
      dispatch(
        walletApi.endpoints.speedupTransaction.initiate({
          chainId: transaction.chainId,
          coinType: getCoinFromTxDataUnion(transaction.txDataUnion),
          fromAddress: transaction.fromAddress,
          transactionId: transaction.id
        })
      )
    }
  }

  const onClickCancelTransaction = () => {
    if (transaction) {
      dispatch(
        walletApi.endpoints.cancelTransaction.initiate({
          chainId: transaction.chainId,
          coinType: getCoinFromTxDataUnion(transaction.txDataUnion),
          fromAddress: transaction.fromAddress,
          transactionId: transaction.id
        })
      )
    }
  }

  // render
  if (!transaction) {
    return <Skeleton />
  }

  return (
    <StyledWrapper>
      <Header
        title={getLocale('braveWalletTransactionDetails')}
        onBack={onBack}
      />
      <OrbContainer>
        <FromCircle orb={fromOrb} />
        <ToCircle orb={toOrb} />
      </OrbContainer>
      <FromToRow>
        <Tooltip
          text={transaction.fromAddress}
          isAddress={true}
          position="left"
        >
          <AccountNameText>{senderLabel}</AccountNameText>
        </Tooltip>
        <ArrowIcon />
        <Tooltip text={recipient} isAddress={true} position="right">
          <AccountNameText>{recipientLabel}</AccountNameText>
        </Tooltip>
      </FromToRow>
      <PanelDescription>{transactionTitle}</PanelDescription>
      <TransactionValue>{transactionValue}</TransactionValue>
      <PanelDescription>{transactionFiatValue}</PanelDescription>
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailStatus')}
        </DetailTitle>
        <StatusRow>
          <StatusBubble status={transaction?.txStatus} />
          <DetailTextDarkBold>
            {getTransactionStatusString(transaction?.txStatus)}
          </DetailTextDarkBold>

          {transaction?.txStatus === BraveWallet.TransactionStatus.Error &&
            txError && (
              <TransactionStatusTooltip
                text={`${txError.code}: ${txError.message}`}
              >
                <AlertIcon />
              </TransactionStatusTooltip>
            )}
        </StatusRow>
      </DetailRow>
      {/* Will remove this conditional for solana once https://github.com/brave/brave-browser/issues/22040 is implemented. */}
      {!isSolanaTxn && (
        <DetailRow>
          <DetailTitle>
            {getLocale('braveWalletAllowSpendTransactionFee')}
          </DetailTitle>
          <BalanceColumn>
            <DetailTextDark>
              {gasFee && transactionsNetwork ? (
                new Amount(gasFee)
                  .divideByDecimals(transactionsNetwork.decimals)
                  .formatAsAsset(6, transactionsNetwork.symbol)
              ) : (
                <Skeleton />
              )}
            </DetailTextDark>
            <DetailTextDark>
              {gasFeeFiat ? (
                new Amount(gasFeeFiat).formatAsFiat(defaultCurrencies.fiat)
              ) : (
                <Skeleton />
              )}
            </DetailTextDark>
          </BalanceColumn>
        </DetailRow>
      )}
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailDate')}
        </DetailTitle>
        <DetailTextDark>
          {serializedTimeDeltaToJSDate(transaction.createdTime).toUTCString()}
        </DetailTextDark>
      </DetailRow>
      {![
        BraveWallet.TransactionStatus.Rejected,
        BraveWallet.TransactionStatus.Error
      ].includes(transaction?.txStatus) && (
        <DetailRow>
          <DetailTitle>
            {getLocale('braveWalletTransactionDetailHash')}
          </DetailTitle>
          <DetailButton
            onClick={onClickViewOnBlockExplorer('tx', transaction?.txHash)}
          >
            {reduceAddress(transaction?.txHash)}
          </DetailButton>
        </DetailRow>
      )}
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailNetwork')}
        </DetailTitle>
        <DetailTextDark>{transactionsNetwork?.chainName ?? ''}</DetailTextDark>
      </DetailRow>

      {[
        BraveWallet.TransactionStatus.Approved,
        BraveWallet.TransactionStatus.Submitted
      ].includes(transaction?.txStatus) &&
        !isSolanaTxn &&
        !isFilTransaction && (
          <DetailRow>
            <DetailTitle />
            <StatusRow>
              <DetailButton onClick={onClickSpeedupTransaction}>
                {getLocale('braveWalletTransactionDetailSpeedUp')}
              </DetailButton>
              <SpacerText>|</SpacerText>
              <DetailButton onClick={onClickCancelTransaction}>
                {getLocale('braveWalletButtonCancel')}
              </DetailButton>
            </StatusRow>
          </DetailRow>
        )}
      {transaction?.txStatus === BraveWallet.TransactionStatus.Error &&
        !isSolanaTxn &&
        !isFilTransaction && (
          <DetailRow>
            <DetailTitle />
            <StatusRow>
              <DetailButton onClick={onClickRetryTransaction}>
                {getLocale('braveWalletTransactionRetry')}
              </DetailButton>
            </StatusRow>
          </DetailRow>
        )}
    </StyledWrapper>
  )
}

export default TransactionDetailPanel
