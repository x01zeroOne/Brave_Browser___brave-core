// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

// shared styles
import { Row } from '../../../shared/style'

export const OriginURLText = styled.span`
  word-break: break-word;
  text-align: left;
  margin-bottom: 4px;
  color: ${leo.color.gray[60]};
  font-size: 12px;
  margin-bottom: 0px;
`

export const ContractOriginColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  text-align: left;
  gap: 4px;
`

export const InlineContractRow = styled(Row)`
  display: inline-flex;
  justify-content: flex-start;
  align-items: center;
  flex-direction: row;
  gap: 4px;
  text-align: left;
  font-size: 11px;
  vertical-align: center;
`

export const OriginIndicatorIconWrapper = styled.div`
  position: absolute;
  bottom: 4px;
  right: 0px;
`

export const OriginWarningIndicator = styled.div`
  width: 10px;
  height: 10px;
  border: 1.2px ${leo.color.container.background} solid;
  border-radius: 100%;
  background-color: ${leo.color.systemfeedback.errorIcon};
`
