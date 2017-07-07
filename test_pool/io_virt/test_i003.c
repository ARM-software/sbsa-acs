/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

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

#include "val/include/sbsa_avs_smmu.h"

#define TEST_NUM   (AVS_SMMU_TEST_NUM_BASE + 3)
#define TEST_DESC  "SMMU Compatibility Check          "

static
void
payload()
{

  uint64_t data;
  uint32_t num_smmu;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);

  if (num_smmu == 0) {
      val_print(AVS_PRINT_ERR, "\n\t No SMMU Controllers are discovered ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  while (num_smmu--)
  {
      if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) == 2) {
          val_print(AVS_PRINT_INFO, "\n\t Detected SMMUv2 ", 0);
          data = val_smmu_read_cfg(SMMUv2_IDR0, num_smmu);
          if ((data & BIT29) == 0) {
              val_print(AVS_PRINT_ERR, "\n\t Stage 2 Translation not supported ", 0);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
              return;
          }
      }
      else{
          val_print(AVS_PRINT_INFO, "\n\t Detected SMMUv3 ", 0);
          data = val_smmu_read_cfg(SMMUv3_IDR0, num_smmu);
          //Check Stage2 translation support
          if ((data & BIT0) == 0) {
              val_print(AVS_PRINT_ERR, "\n\t Stage 2 Translation not supported ", 0);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
              return;
          }

          // Check COHACC
          if ((data & BIT4) == 0) {
              val_print(AVS_PRINT_ERR, "\n\t IO-Coherent access not supported  ", 0);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
              return;
          }

          data = val_smmu_read_cfg(SMMUv3_AIDR, num_smmu);
          if (((data & 0xFF) != 0) && ((data & 0xFF) != 0x1)) {
              val_print(AVS_PRINT_ERR, "\n\t Invalid Architecture Revision %x  ", data);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 04));
              return;
          }
      }

      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  }

}

uint32_t
i003_entry(uint32_t num_pe)
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
