// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { create } from 'ethereum-blockies'
import { skipToken } from '@reduxjs/toolkit/dist/query'

// actions
import { UIActions } from '../slices/ui.slice'

// utils
import Amount from '../../utils/amount'
import { findAccountName } from '../../utils/account-utils'
import { getLocale } from '../../../common/locale'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'
import { reduceAddress } from '../../utils/reduce-address'
import { UISelectors, WalletSelectors } from '../selectors'
import { selectPendingTransactions } from '../slices/entities/transaction.entity'

// Custom Hooks
import { useTransactionParser } from './transaction-parser'
import usePricing from './pricing'
import useTokenInfo from './token'
import { useLib } from './useLib'
import {
  useSafeUISelector,
  useSafeWalletSelector,
  useUnsafeWalletSelector
} from './use-safe-selector'
import {
  useGetGasEstimation1559Query,
  useGetNetworkQuery,
  useGetSolanaEstimatedFeeQuery,
  useGetTransactionsQuery,
  walletApi
} from '../slices/api.slice'

// Constants
import {
  BraveWallet,
  SerializableTransactionInfo
} from '../../constants/types'
import {
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionNonceType
} from '../constants/action_types'
import { isSolanaTransaction, sortTransactionByDate } from '../../utils/tx-utils'
import { MAX_UINT256 } from '../constants/magics'

const emptyPendingTxs: SerializableTransactionInfo[] = []

export const usePendingTransactionsQuery = (
  arg: Parameters<typeof useGetTransactionsQuery>[0]
) => {
  return useGetTransactionsQuery(arg, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      transactions: res.data || emptyPendingTxs,
      pendingTransactions: res.data
        ? selectPendingTransactions(res.data)
        : emptyPendingTxs
    })
  })
}

export const usePendingTransactions = () => {
  // redux
  const dispatch = useDispatch()
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const selectedPendingTransactionId = useSafeUISelector(
    UISelectors.selectedPendingTransactionId
  )
  const visibleTokens = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )
  const transactionSpotPrices = useUnsafeWalletSelector(
    WalletSelectors.transactionSpotPrices
  )
  const fullTokenList = useUnsafeWalletSelector(WalletSelectors.fullTokenList)
  const hasFeeEstimatesError = useSafeWalletSelector(
    WalletSelectors.hasFeeEstimatesError
  )

  // queries
  const { pendingTransactions } = usePendingTransactionsQuery({
    address: null,
    chainId: null,
    coinType: null
  })

  const transactionInfo = React.useMemo(() => {
    return (
      pendingTransactions.find(
        (tx) => tx.id === selectedPendingTransactionId
      ) || pendingTransactions[0]
    )
  }, [pendingTransactions, selectedPendingTransactionId])

  const txCoinType = transactionInfo
    ? getCoinFromTxDataUnion(transactionInfo.txDataUnion)
    : undefined

  const { data: transactionsNetwork } = useGetNetworkQuery(
    transactionInfo && txCoinType !== undefined
      ? {
          chainId: transactionInfo.chainId,
          coin: txCoinType
        }
      : undefined,
    { skip: !transactionInfo && txCoinType !== undefined }
  )

  const { data: gasEstimates } = useGetGasEstimation1559Query(undefined, {
    refetchOnFocus: true,
    pollingInterval: 15000,
    refetchOnMountOrArgChange: 15000,
    refetchOnReconnect: true,
  })

  useGetSolanaEstimatedFeeQuery(
    transactionInfo && txCoinType === BraveWallet.CoinType.SOL
      ? {
          chainId: transactionInfo.chainId,
          txId: transactionInfo.id
        }
      : skipToken,
    {
      refetchOnFocus: true,
      pollingInterval: 15000,
      refetchOnMountOrArgChange: 15000,
      refetchOnReconnect: true,
      skip: !transactionInfo || txCoinType !== BraveWallet.CoinType.SOL
    }
  )

  // custom hooks
  const { getBlockchainTokenInfo, getERC20Allowance } = useLib()
  const parseTransaction = useTransactionParser(transactionsNetwork)
  const { findAssetPrice } = usePricing(transactionSpotPrices)
  const {
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress
  } = useTokenInfo(getBlockchainTokenInfo, visibleTokens, fullTokenList, transactionsNetwork)

  // state
  const [erc20AllowanceResult, setERC20AllowanceResult] = React.useState<
    string | undefined
  >(undefined)

  // computed state
  const transactionDetails = React.useMemo(() => {
    return transactionInfo ? parseTransaction(transactionInfo) : undefined
  }, [parseTransaction, transactionInfo])

  const { suggestedMaxPriorityFeeChoices, baseFeePerGas } = React.useMemo(
    () => ({
      suggestedMaxPriorityFeeChoices: [
        gasEstimates?.slowMaxPriorityFeePerGas || '0',
        gasEstimates?.avgMaxPriorityFeePerGas || '0',
        gasEstimates?.fastMaxPriorityFeePerGas || '0'
      ],
      baseFeePerGas: gasEstimates?.baseFeePerGas || '0'
    }),
    [gasEstimates]
  )


  const transactionQueueNumber = pendingTransactions.findIndex(tx => tx.id === transactionInfo?.id) + 1
  const transactionsQueueLength = pendingTransactions.length

  const isERC20Approve = transactionInfo?.txType === BraveWallet.TransactionType.ERC20Approve
  const isERC721SafeTransferFrom = transactionInfo?.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  const isERC721TransferFrom = transactionInfo?.txType === BraveWallet.TransactionType.ERC721TransferFrom

  const isSolanaTxn = transactionInfo && isSolanaTransaction(transactionInfo)
  const isSolanaDappTransaction = transactionInfo?.txType && [
    BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
    BraveWallet.TransactionType.SolanaDappSignTransaction
  ].includes(transactionInfo.txType)

  const isAssociatedTokenAccountCreation =
    transactionInfo?.txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation

  const isFilecoinTransaction =
    transactionInfo?.txType === BraveWallet.TransactionType.Other &&
    transactionInfo?.txDataUnion?.filTxData

  // methods
  const onEditAllowanceSave = React.useCallback((allowance: string) => {
    if (transactionInfo?.id && transactionDetails) {
      dispatch(
        walletApi.endpoints.updateUnapprovedTransactionSpendAllowance.initiate({
          chainId: transactionInfo.chainId,
          txMetaId: transactionInfo.id,
          spenderAddress: transactionDetails.approvalTarget || '',
          allowance: new Amount(allowance)
            .multiplyByDecimals(transactionDetails.decimals)
            .toHex()
        })
      )
    }
  }, [transactionInfo?.id, transactionDetails])

  const updateUnapprovedTransactionNonce = React.useCallback(
    (args: UpdateUnapprovedTransactionNonceType) => {
      dispatch(
        walletApi.endpoints.updateUnapprovedTransactionNonce.initiate(args)
      )
    },
    []
  )

  const queueNextTransaction = React.useCallback(() => {
    // if id hasn't been set, start at beginning of tx list
    const currentIndex = selectedPendingTransactionId
      ? pendingTransactions.findIndex(
          (tx) => tx.id === selectedPendingTransactionId
        )
      : 0

    const nextIndex = currentIndex + 1

    const newSelectedPendingTransactionId =
      nextIndex === pendingTransactions.length // at end of list?
        ? pendingTransactions[0]?.id // go to first item in list
        : pendingTransactions[nextIndex]?.id // go to next item in list

    dispatch(
      UIActions.setPendingTransactionId(newSelectedPendingTransactionId)
    )
  }, [selectedPendingTransactionId, pendingTransactions])

  const rejectAllTransactions = React.useCallback(
    () => dispatch(walletApi.endpoints.rejectAllTransactions.initiate()),
    []
  )

  const updateUnapprovedTransactionGasFields = React.useCallback(
    (payload: UpdateUnapprovedTransactionGasFieldsType) => {
      dispatch(
        walletApi.endpoints.updateUnapprovedTransactionGasFields.initiate(
          payload
        )
      )
    },
    []
  )

  // List of all transactions that belong to the same group as the selected
  // pending transaction.
  const groupTransactions = React.useMemo(
    () =>
      transactionInfo?.groupId && transactionInfo?.fromAddress
        ? sortTransactionByDate(
            pendingTransactions.filter(
              (txn) => txn.groupId === transactionInfo.groupId
            )
          )
        : [],
    [transactionInfo, pendingTransactions]
  )

  const unconfirmedGroupTransactionIds = React.useMemo(() =>
    groupTransactions
      .filter(txn => txn.txStatus !== BraveWallet.TransactionStatus.Confirmed)
      .map(txn => txn.id),
    [groupTransactions])

  // Position of the selected pending transaction in the group, if exists.
  const selectedPendingTransactionGroupIndex = React.useMemo(() =>
    groupTransactions.findIndex(txn => transactionInfo?.id === txn.id),
    [groupTransactions, transactionInfo])

  // The selected pending transaction can only be approved if:
  //   - it does not belong to a transaction group
  //   - it is the first unconfirmed transaction in the group
  const canSelectedPendingTransactionBeApproved = React.useMemo(() =>
    unconfirmedGroupTransactionIds.findIndex(idx => transactionInfo?.id === idx) <= 0,
    [transactionInfo, unconfirmedGroupTransactionIds])

  // memos
  const fromOrb = React.useMemo(() => {
    return create({
      seed: transactionInfo?.fromAddress.toLowerCase(),
      size: 8,
      scale: 16
    }).toDataURL()
  }, [transactionInfo?.fromAddress])

  const toOrb = React.useMemo(() => {
    return create({
      seed: transactionDetails?.recipient.toLowerCase(),
      size: 8,
      scale: 10
    }).toDataURL()
  }, [transactionDetails?.recipient])

  const fromAccountName = React.useMemo(() => {
    return findAccountName(accounts, transactionInfo?.fromAddress ?? '') ?? reduceAddress(transactionInfo?.fromAddress ?? '')
  }, [accounts, transactionInfo?.fromAddress])

  const transactionTitle = React.useMemo(
    (): string =>
      isSolanaDappTransaction
        ? getLocale('braveWalletApproveTransaction')
        : transactionDetails?.isSwap
          ? getLocale('braveWalletSwap')
          : getLocale('braveWalletSend')
    , [isSolanaDappTransaction, transactionDetails?.isSwap])

  const isLoadingGasFee =
    // FIL has gas info provided by txDataUnion
    !transactionDetails?.isFilecoinTransaction &&
    transactionDetails?.gasFee === ''

  const isConfirmButtonDisabled = React.useMemo(() => {
    if (hasFeeEstimatesError || isLoadingGasFee) {
      return true
    }

    if (!transactionDetails) {
      return true
    }

    return (
      !!transactionDetails?.sameAddressError ||
      !!transactionDetails?.contractAddressError ||
      transactionDetails?.insufficientFundsForGasError === undefined ||
      transactionDetails?.insufficientFundsError === undefined ||
      !!transactionDetails?.insufficientFundsForGasError ||
      !!transactionDetails?.insufficientFundsError ||
      !!transactionDetails?.missingGasLimitError ||
      !canSelectedPendingTransactionBeApproved
    )
  }, [transactionDetails, hasFeeEstimatesError, isLoadingGasFee])

  const {
    currentTokenAllowance,
    isCurrentAllowanceUnlimited
  } = React.useMemo(() => {
    if (!transactionDetails || erc20AllowanceResult === undefined) {
      return {
        currentTokenAllowance: undefined,
        isCurrentAllowanceUnlimited: false
      }
    }
    
    const currentTokenAllowance = new Amount(erc20AllowanceResult)
      .divideByDecimals(transactionDetails.decimals)
      .format()

    const isCurrentAllowanceUnlimited = erc20AllowanceResult === MAX_UINT256

    return {
      currentTokenAllowance,
      isCurrentAllowanceUnlimited
    }
  }, [erc20AllowanceResult])

  // effects
  React.useEffect(() => {
    let subscribed = true
    if (transactionInfo?.txType !== BraveWallet.TransactionType.ERC20Approve) {
      return
    }

    if (!transactionDetails?.approvalTarget) {
      return
    }

    getERC20Allowance(
      transactionDetails.recipient,
      transactionInfo.fromAddress,
      transactionDetails.approvalTarget
    )
      .then((result) => {
        subscribed && setERC20AllowanceResult(result)
      })
      .catch((e) => console.error(e))

    // cleanup
    return () => {
      subscribed = false
    }
  }, [
    transactionInfo?.txType,
    transactionInfo?.fromAddress,
    transactionDetails,
    getERC20Allowance
  ])

  React.useEffect(() => {
    if (
      transactionDetails?.recipient &&
      transactionInfo?.txType === BraveWallet.TransactionType.ERC20Approve
    ) {
      onFindTokenInfoByContractAddress(transactionDetails.recipient)
    }
  }, [transactionDetails?.recipient, transactionInfo?.txType])

  return {
    baseFeePerGas,
    currentTokenAllowance,
    isCurrentAllowanceUnlimited,
    findAssetPrice,
    foundTokenInfoByContractAddress,
    fromAccountName,
    fromAddress: transactionInfo?.fromAddress ?? '',
    fromOrb,
    isConfirmButtonDisabled,
    isERC20Approve,
    isERC721SafeTransferFrom,
    isERC721TransferFrom,
    isSolanaDappTransaction,
    isSolanaTransaction: isSolanaTxn,
    isAssociatedTokenAccountCreation,
    isFilecoinTransaction,
    onEditAllowanceSave,
    queueNextTransaction,
    rejectAllTransactions,
    suggestedMaxPriorityFeeChoices,
    toOrb,
    transactionDetails,
    transactionQueueNumber,
    transactionsNetwork,
    transactionsQueueLength,
    transactionTitle,
    sendOptions: transactionInfo?.txDataUnion.solanaTxData?.sendOptions,
    updateUnapprovedTransactionGasFields,
    updateUnapprovedTransactionNonce,
    groupTransactions,
    selectedPendingTransactionGroupIndex,
    hasFeeEstimatesError,
    selectedPendingTransaction: transactionInfo,
    isLoadingGasFee
  }
}
