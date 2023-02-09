/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const root = styled.div`
  font-family: Poppins;
  display: flex;
  gap: 80px;
  position: relative;
  z-index: 1;

  /* Use :where to reduce selector specificity and allow overriding. */
  *:where(a) {
    color: #423EEE; /* Light/Text/Interactive */
    text-decoration: none;

    &:hover {
      text-decoration: underline;
    }
  }
`

export const creator = styled.div`
  flex: 1 1 auto;
  margin-top: 120px;
  margin-left: 80px;
`

export const form = styled.div`
  flex: 0 0 auto;
  margin-top: 24px;
  margin-right: 24px;
  margin-bottom: 32px;
`
