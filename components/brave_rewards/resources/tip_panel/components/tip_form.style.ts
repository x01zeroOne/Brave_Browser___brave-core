/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import * as mixins from '../../shared/lib/css_mixins'

import formBackgroundURL from '../assets/form_background.svg'

export const root = styled.div`
  width: 496px;
  display: flex;
  flex-direction: column;
  gap: 24px;
`

export const card = styled.div`
  background:
    linear-gradient(180deg, rgba(255, 255, 255, 0) 304px, #FFFFFF 408px),
    linear-gradient(0deg, #FFFFFF 0, rgba(255, 255, 255, 0) 192px),
    no-repeat top center
      url(./${formBackgroundURL});
  box-shadow: 0px 4px 16px -1px rgba(0, 0, 0, 0.07);
  border-radius: 16px;
  display: flex;
  flex-direction: column;
`

export const title = styled.div`
  padding: 32px;
  font-weight: 500;
  font-size: 22px;
  line-height: 32px;
  color: #1D1F25; /* Light/Text/Primary */
`

export const inputPanel = styled.div`
  margin: 0 16px;
  background: #FFFFFF; /* Light/Container/Background */
  border-radius: 8px;
  padding: 32px;
  display: flex;
  flex-direction: column;
  gap: 32px;
`

export const buttons = styled.div`
  margin: 18px 32px 32px;
  display: flex;
  flex-direction: column;
  gap: 8px;

  button {
    ${mixins.buttonReset}
    padding: 16px 20px;
    border: 1px solid #E2E2FC; /* Extended/Light/Primary/20 */
    border-radius: 1000px;
    font-weight: 600;
    font-size: 14px;
    line-height: 20px;
    color: #423EEE; /* Light/Text/Interactive */

    &:hover {
      cursor: pointer;
    }
  }

  button.primary {
    /* Extended/White */
    color: #FFFFFF;
    background: #423EEE; /* Light/Interaction/Button-primary-background */
  }
`

export const terms = styled.div`
  text-align: center;
  font-weight: 400;
  font-size: 11px;
  line-height: 16px;
  color: #6B7084; /* Light/Text/Secondary */

  a {
    color: #1D1F25; /* Light/Text/Primary */
    text-decoration: none;
  }
`
