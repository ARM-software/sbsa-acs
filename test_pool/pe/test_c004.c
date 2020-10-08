/** @file
 * Copyright (c) 2016-2018, 2020 Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PE_TEST_NUM_BASE  +  04)
#define TEST_DESC  "Check PE Granule Support          "

static
void
payload()
{
  uint64_t data = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  data = val_pe_reg_read(ID_AA64MMFR0_EL1);

  /* PEs must support 4k, 64k granule for Stage 1 & Stage 2.
   * 1. Implementation before v8.5
   *    ID_AA64MMFR0_EL1 [43:36] should be RES0.
   *    Check For TGran4[31:28] == 0 & TGran64[27:24] == 0.
   * 2. Implementation After v8.5
   *    ID_AA64MMFR0_EL1 [43:36] should not be 0.
   *    Check For TGran4[31:28] == 0 & TGran64[27:24] == 0.
   *    Check For TGran4_2[43:40] == 0x2 & TGran64_2[39:36] == 0x2.
   */

  if (VAL_EXTRACT_BITS(data, 36, 43) == 0) {
    /* Implementation before Arm v8.5 */
    if (VAL_EXTRACT_BITS(data, 24, 31) == 0)
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
    else
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));

  } else {
    /* Implementation after Arm v8.5 */
    if ((VAL_EXTRACT_BITS(data, 24, 31) == 0) &&
        (VAL_EXTRACT_BITS(data, 36, 43) == 0x22))
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
    else
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));

  }
}

uint32_t
c004_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}


