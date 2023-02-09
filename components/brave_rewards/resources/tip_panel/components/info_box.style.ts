/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  display: flex;
  align-items: flex-start;
  gap: 18px;
  background: #F0F7FC; /* Light/SystemFeedback/Info-Background */
  border-radius: 8px;
  padding: 24px;
`

export const icon = styled.div`
  flex: 0 0 26px;
`

export const content = styled.div`
  flex: 1 1 auto;
  display: flex;
  flex-direction: column;
  gap: 4px;
`

export const title = styled.div`
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  color: #1D1F25; /* Light/Text/Primary */
`

export const text = styled.div`
  font-size: 12px;
  line-height: 18px;
  color: #1D1F25; /* Light/Text/Primary */
`
