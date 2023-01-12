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
#include "val/include/val_interface.h"

#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_smmu.h"

#define TEST_NUM   (AVS_SMMU_TEST_NUM_BASE + 15)
#define TEST_RULE  "B_SMMU_11, B_SMMU_22"
#define TEST_DESC  "Check system for MPAM support     "

static
void
payload()
{

  uint32_t num_smmu;
  uint32_t smmu_rev;
  uint32_t minor;
  uint32_t index;
  uint32_t pe_mpam, mpam;
  uint32_t frac;
  uint32_t max_id;

  index = val_pe_get_index_mpid(val_pe_get_mpid());
  // Major MPAM revision supported by the PE (i.e. MPAM v1.x)
  pe_mpam = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 40, 43);
  // Minor MPAM revision supported by the PE (i.e. MPAM vx.1)
  frac = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR1_EL1), 16, 19);

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);
  if (num_smmu == 0) {
    val_print(AVS_PRINT_DEBUG, "\n       No SMMU Controllers are discovered                 ", 0);
    val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 3));
    return;
  }

  if (!(pe_mpam || frac)) {
    val_print(AVS_PRINT_DEBUG, "\n       No MPAM controlled resources present               ", 0);
    val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 3));
    return;
  }

  while (num_smmu--) {
        smmu_rev = val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu);
        if (smmu_rev < 3) {
                // MPAM support not required for SMMUv2 and below
                val_print(AVS_PRINT_DEBUG, "\n       SMMU revision v2 or lower detected  ", 0);
                val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 4));
                return;
        }
        else {
                minor = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_AIDR, num_smmu), 0, 3);
                // Check if MPAM is supported for any security state (only for SMMU v3.2+)
                if (minor >= 2) {
                        // SMMU general MPAM support (in either Secure/Non-Secure state)
                        mpam = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_IDR3, num_smmu),
                              7, 7);
                        // Check if MPAM is supported for any of the NS resources (max part ID)
                        max_id = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_MPAMIDR,
                              num_smmu), 0, 15);
                        if (!(mpam && max_id)) {
                                val_print(AVS_PRINT_ERR,
                                          "\n       SMMU without MPAM support detected  ", 0);
                                val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 4));
                                return;
                        }
                }
                else {
                        // MPAM support not required for SMMUv3.0/3.1
                        val_print(AVS_PRINT_WARN,
                              "\n       SMMU revision v3.0/3.1 detected  ", 0);
                        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 4));
                        return;
                }
        }
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 1));
}

uint32_t
i015_entry(uint32_t num_pe)
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
