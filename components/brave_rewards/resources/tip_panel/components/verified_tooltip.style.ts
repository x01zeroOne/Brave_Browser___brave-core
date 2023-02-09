/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  --tooltip-width: 280px;
  position: absolute;
  bottom: calc(100% - 10px);
  left: calc(50% - var(--tooltip-width) / 2);
  width: var(--tooltip-width);
  padding-bottom: 20px;
`

export const content = styled.div`
  position: relative;
  background: #FFFFFF; /* Extended/White */
  box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, 0.08);
  border-radius: 8px;
  padding: 24px;
  display: flex;
  flex-direction: column;
  gap: 8px;
`

export const arrow = styled.div`
  background: #FFFFFF; /* Extended/White */
  transform: rotate(45deg);
  width: 10px;
  height: 10px;
  position: absolute;
  bottom: -5px;
  left: calc(50% - 5px);
`

export const title = styled.div`
  display: flex;
  align-items: center;
  gap: 9px;
  font-weight: 400;
  font-size: 14px;
  line-height: 24px;
  color: #1D1F25; /* Light/Text/Primary */
`

export const checkmark = styled.div`
  width: 15px;
  height: 18px;
`

export const text = styled.div`
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  color: #6B7084; /* Light/Text/Secondary */
`
