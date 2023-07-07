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

#include "val/include/sbsa_avs_val.h"
#include "val/include/val_interface.h"

#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_smmu.h"
#include "val/include/sbsa_avs_pcie.h"

#define TEST_NUM   (AVS_SMMU_TEST_NUM_BASE + 4)
#define TEST_RULE  "B_SMMU_20"
#define TEST_DESC  "SMMU Revision,S-EL2 support Hyp   "

static
void
payload()
{
  /* Check SMMU Revision & S-EL2 Support for Hypervisor */
  uint32_t num_smmu;
  uint32_t index;
  uint32_t s_el2;
  uint32_t smmu_rev;
  uint32_t minor;
  uint32_t s2p;

  index = val_pe_get_index_mpid(val_pe_get_mpid());
  s_el2 = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 36, 39);

  if (!s_el2) {
      val_print(AVS_PRINT_ERR, "\n       Secure EL2 not implemented", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 1));
      return;
  }

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);
  if (num_smmu == 0) {
      val_print(AVS_PRINT_ERR, "\n       No SMMU Controllers are discovered ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 2));
      return;
  }

  while (num_smmu--) {
      smmu_rev = val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu);

      if (smmu_rev < 3) {
          val_print(AVS_PRINT_ERR,
                    "\n       SMMUv2 or lower detected: revision must be v3.2 or higher  ", 0);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 1));
          return;
      } else {
          minor = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_AIDR, num_smmu), 0, 3);
          if (minor < 2) {
              val_print(AVS_PRINT_ERR,
                  "\n       SMMUv3.%d detected: revision must be v3.2 or higher  ", minor);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 2));
              return;
          }
          s2p = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_IDR0, num_smmu), 0, 0);
          if (!s2p) {
              val_print(AVS_PRINT_ERR,
                        "\n       SMMUv3.%d detected: but Stage 2 "
                        "translation not supported  ", minor);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 3));
              return;
          }
      }

  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 1));
}

uint32_t
i004_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
