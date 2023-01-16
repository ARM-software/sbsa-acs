/** @file
 * Copyright (c) 2016-2018, 2022-2023 Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PE_TEST_NUM_BASE  +  3)
#define TEST_RULE  "S_L3PE_03"
#define TEST_DESC  "Check AARCH64 implementation      "

static
void
payload(void)
{
  uint64_t data = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  data = val_pe_reg_read(ID_AA64PFR0_EL1);

  /* bits 1:0, 5:4, 9:8 and 13:12 must not be zero */
  if ((data & 0x3) && (data & 0x30) && (data & 0x300) && (data & 0x3000))
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  else
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));

  return;

}

uint32_t
c003_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}

