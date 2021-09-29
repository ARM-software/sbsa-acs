/** @file
 * Copyright (c) 2016-2018, 2021 Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_val.h"
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM    (AVS_PE_TEST_NUM_BASE  +  14)
#define TEST_DESC  "Check number of Breakpoints       "

static
void payload()
{
  uint64_t data = 0;
  int32_t  breakpointcount;
  uint32_t context_aware_breakpoints = 0;
  uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  data = val_pe_reg_read(ID_AA64DFR0_EL1);

  /* bits 15:12 for Number of breakpoints - 1 */
  breakpointcount = VAL_EXTRACT_BITS(data, 12, 15) + 1;

  if (breakpointcount < 6) {
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 1));
      return;
  }

  /*bits [31:28] Number of breakpoints that are context-aware, minus 1*/
  context_aware_breakpoints = VAL_EXTRACT_BITS(data, 28, 31) + 1;
  if (context_aware_breakpoints > 1)
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 1));
  else
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 2));


  return;

}

/**
  @brief   Check for the number of breakpoints available
**/
uint32_t
c014_entry(uint32_t num_pe)
{
  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      /* execute payload on present PE and then execute on other PE */
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}

