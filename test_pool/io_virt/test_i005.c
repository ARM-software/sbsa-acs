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
#include "val/include/val_interface.h"

#include "val/include/sbsa_avs_iovirt.h"

#define TEST_NUM   (AVS_SMMU_TEST_NUM_BASE + 5)
#define TEST_DESC  "SMMUv2 unique intr per ctxt bank  "

static
void
payload()
{
  uint32_t num_smmu;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);

  if (num_smmu == 0) {
      val_print(AVS_PRINT_ERR, "\n       No SMMU Controllers are discovered ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 3));
      return;
  }

  while (num_smmu--) {
      if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) == 3) {
          val_print(AVS_PRINT_WARN, "\n       Not valid for SMMU v3             ", 0);
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 2));
          return;
      }

      if(!val_iovirt_check_unique_ctx_intid(num_smmu)) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 1));
          return;
      }
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 0));
}

uint32_t
i005_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
