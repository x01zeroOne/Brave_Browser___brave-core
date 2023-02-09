/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

export const root = styled.div`
  display: flex;
  gap: 18px;
  justify-content: space-between;
  align-items: flex-start;
`

export const selector = styled.div`
  --amount-selector-border-color: #E2E3E7; /* Light/Divider/Subtle */

  border: 1px solid var(--amount-selector-border-color);
  border-radius: 8px;
  display: flex;
  overflow: hidden;

  button {
    ${mixins.buttonReset}
    padding: 12px 15px;
    min-width: 50px;
    border-left: 1px solid var(--amount-selector-border-color);
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    color: #6B7084; /* Light/Text/Secondary */

    &:hover {
      cursor: pointer;
    }
  }

  button:first-child {
    border: none;
  }

  button.selected {
    background: #F5F4FE; /* Light/Container/Interactive-background */
    color: #423EEE; /* Light/Text/Interactive */
  }
`

export const amount = styled.div``

export const primary = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 8px;
`

export const primarySymbol = styled.div`
  font-size: 14px;
  line-height: 24px;
  color: #6B7084; /* Light/Text/Secondary */
`

export const primaryAmount = styled.div`
  font-weight: 500;
  font-size: 28px;
  line-height: 44px;
  color: #1D1F25; /* Light/Text/Primary */
`

export const primaryLabel = styled.div`
  font-weight: 600;
  font-size: 14px;
  line-height: 24px;
  color: #6B7084; /* Light/Text/Secondary */
`

export const customInput = styled.div`
  input {
    border: none;
    margin: 0;
    background: #F4F6F8; /* Light/Container/Highlight */
    border-radius: 8px;
    padding: 6px 0;
    width: 3em;
    text-align: center;
    font-weight: 500;
    font-size: 28px;
    line-height: 32px;
    color: #1D1F25; /* Light/Text/Primary */
  }
`

export const secondary = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 10px;
`

export const secondaryAmount = styled.div`
  font-size: 14px;
  line-height: 24px;
  color: #6B7084; /* Light/Text/Secondary */
`

export const swap = styled.div`
  button {
    ${mixins.buttonReset}
    height: 16px;
    width: 16px;
    cursor: pointer;
  }
`
