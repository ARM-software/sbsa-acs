/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#include "../include/pal_common_support.h"

/**
  @brief  Check the hid and copy the full path of hid

  @param  hid      hardware ID to get the path for
  @param  hid_path 2D array in which the path is copied

  @return 1 if test fails, 0 if test passes
**/
uint32_t
pal_get_device_path(const char *hid, char hid_path[][MAX_NAMED_COMP_LENGTH])
{
    return 1;
}

/**
  @brief  Platform defined method to check if CATU is behind an ETR device

  @param  etr_path  full path of ETR device

  @return 0 - Success, NOT_IMPLEMENTED - API not implemented, Other values - Failure
**/
uint32_t
pal_smmu_is_etr_behind_catu(char *etr_path)
{
    return NOT_IMPLEMENTED;
}

