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

#include "val/include/sbsa_avs_val.h"
#include "val/include/val_interface.h"

#include "val/include/sbsa_avs_iovirt.h"
#include "val/include/sbsa_avs_smmu.h"
#include "val/include/sbsa_avs_pcie.h"

#define TEST_NUM   (AVS_SMMU_TEST_NUM_BASE + 14)
#define TEST_RULE  "S_L7SM_03, S_L7SM_04"
#define TEST_DESC  "Check SMMUv3 PMU Extension        "

static
void
payload()
{

  uint32_t num_smmu;
  uint32_t num_pmcg = 0;
  uint32_t i = 0;
  uint32_t smmu_version;
  uint64_t smmu_base = 0;
  uint64_t pmcg_base = 0;
  uint64_t pmcg_node_ref  = 0;
  uint64_t num_pmcg_count = 0;
  uint64_t num_pmcg_found = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  if (g_sbsa_level < 6) {
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);
  num_pmcg = val_iovirt_get_pmcg_info(PMCG_NUM_CTRL, 0);

  if (num_smmu == 0) {
      val_print(AVS_PRINT_DEBUG, "\n       No SMMU Controllers are discovered ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
      return;
  }

  while (num_smmu--) {
      smmu_version = val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu);
      if (smmu_version != 3) {
          val_print(AVS_PRINT_DEBUG,
                    "\n       Valid for only SMMU v3, smmu version %d", smmu_version);
          continue;
      }

      smmu_base = val_smmu_get_info(SMMU_CTRL_BASE, num_smmu);

      num_pmcg_found = 0;
      /* Each SMMUv3 should contain atleast 1 PMCG*/
      for (i = 0; i < num_pmcg; i++) {
          pmcg_node_ref = val_iovirt_get_pmcg_info(PMCG_NODE_REF, i);
          if (smmu_base ==  pmcg_node_ref) {
              pmcg_base = val_iovirt_get_pmcg_info(PMCG_CTRL_BASE, i);
              /*Check if SMMU_PMCG_CFGR.NCTR > 4*/
              num_pmcg_count = VAL_EXTRACT_BITS(val_mmio_read(pmcg_base + SMMU_PMCG_CFGR), 0, 5);
              /*No of counters in a group is SMMU_PMCG_CFGR.NCTR + 1*/
              num_pmcg_count++;
              /* Each PMCG should have atleast 4 counters*/
              if (num_pmcg_count < 4) {
                  val_print(AVS_PRINT_ERR, "\n       PMCG has less then 4 counters", 0);
                  val_print(AVS_PRINT_ERR,
                            "\n       No of PMCG counters :%d       ", num_pmcg_count);
                  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
                  return;
              }
              num_pmcg_found++;
          }
      }

      if (num_pmcg_found == 0) {
          val_print(AVS_PRINT_ERR, "\n       PMU Extension not implimented for SMMUv3", 0);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
          return;
      }

  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
i014_entry(uint32_t num_pe)
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
