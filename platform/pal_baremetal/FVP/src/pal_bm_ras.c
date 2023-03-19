/** @file
 * Copyright (c) 2023 Arm Limited or its affiliates. All rights reserved.
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

#include "include/pal_common_support.h"

/**
  @brief  API to check support for Poison

  @param  None

  @return  0 - Poison storage & forwarding not supported
           1 - Poison storage & forwarding supported
**/
uint32_t
pal_ras_check_plat_poison_support()
{
  return 0;
}

/**
  @brief  Platform Defined way of setting up the Error Environment

  @param  in_param  - Error Input Parameters.
  @param  *out_param  - Parameters returned from platform to be used in the test.

  @return  0 - Success, NOT_IMPLEMENTED - API not implemented, Other values - Failure
**/
uint32_t
pal_ras_setup_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param)
{
  /* Platform Defined way of setting up the Error Environment */
  return NOT_IMPLEMENTED;
}

/**
  @brief  Platform Defined way of injecting up the Error Environment

  @param  in_param  - Error Input Parameters.
  @param  *out_param  - Parameters returned from platform to be used in the test.

  @return  0 - Success, NOT_IMPLEMENTED - API not implemented, Other values - Failure
**/
uint32_t
pal_ras_inject_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param)
{
  return NOT_IMPLEMENTED;
}

