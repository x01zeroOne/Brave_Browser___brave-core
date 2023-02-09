/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useLocaleContext } from '../lib/locale_strings'
import { useModel } from '../lib/model_context'
import { formatMessage } from '../../shared/lib/locale_context'
import { getExternalWalletProviderName } from '../../shared/lib/external_wallet'
import { InfoBox } from './info_box'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { BalanceView } from './balance_view'
import { MonthlyToggle } from './monthly_toggle'
import { AmountInput } from './amount_input'

import * as urls from '../../shared/lib/rewards_urls'

import * as style from './tip_form.style'

export function TipForm () {
  const { getString } = useLocaleContext()
  const state = useModel((state) => state)
  const [, setSendAmount] = React.useState(0)
  const [monthlyChecked, setMonthlyChecked] = React.useState(false)

  function getAmountOptions () {
    if (state.creatorBanner.amounts.length > 0) {
      return state.creatorBanner.amounts
    }
    return state.globalState.defaultAmounts
  }

  function renderFormElements () {
    if (!state.rewardsUser.walletProvider) {
      return (
        <InfoBox title='Connect'>
          Connect your account
        </InfoBox>
      )
    }

    // TODO: Do we need to know how to distinguish between web3 and custodial
    // wallets here?
    const creatorHasWallet = state.creatorWallets.length > 0
    if (!creatorHasWallet) {
      return (
        <InfoBox title='Creator not set up'>
          The creator has not set up an account.
        </InfoBox>
      )
    }

    if (!state.rewardsUser.walletAuthorized) {
      return (
        <InfoBox title='Wallet not authorized'>
          Wallet not authorized.
        </InfoBox>
      )
    }

    const providersMatch = state.creatorWallets.some((wallet) => {
      return wallet.provider === state.rewardsUser.walletProvider
    })

    if (!providersMatch) {
      const creatorProvider = state.creatorWallets[0].provider
      return (
        <InfoBox title={getString('providerMismatchTitle')}>
          {
            formatMessage(getString('providerMismatchText'), [
              getExternalWalletProviderName(state.rewardsUser.walletProvider),
              getExternalWalletProviderName(creatorProvider)
            ])
          }
        </InfoBox>
      )
    }

    return (
      <>
        <AmountInput
          amountOptions={getAmountOptions()}
          userBalance={state.rewardsUser.balance}
          exchangeRate={state.globalState.exchangeRate}
          exchangeCurrency={state.globalState.exchangeCurrency}
          onAmountUpdated={setSendAmount}
        />
        <MonthlyToggle
          monthlySet={state.monthlyContributionSet}
          checked={monthlyChecked}
          onChange={setMonthlyChecked}
        />
      </>
    )
  }

  return (
    <style.root>
      <style.card>
        <style.title>
          {getString('sendFormTitle')}
        </style.title>
        <style.inputPanel>
          <BalanceView
            userBalance={state.rewardsUser.balance}
            walletProvider={state.rewardsUser.walletProvider}
          />
          {renderFormElements()}
        </style.inputPanel>
        <style.buttons>
          <button className='primary'>
            {getString('sendButtonLabel')}
          </button>
          <button>
            {getString('web3ButtonLabel')}
          </button>
        </style.buttons>
      </style.card>
      <style.terms>
        <div>
          {getString('feeNotice')}
        </div>
        <div>
          {
            formatMessage(getString('termsOfService'), {
              tags: {
                $1: (content) => (
                  <NewTabLink key='terms' href={urls.termsOfServiceURL}>
                    {content}
                  </NewTabLink>
                ),
                $3: (content) => (
                  <NewTabLink key='pp' href={urls.privacyPolicyURL}>
                    {content}
                  </NewTabLink>
                )
              }
            })
          }
        </div>
      </style.terms>
    </style.root>
  )
}
