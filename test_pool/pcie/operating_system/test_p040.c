/** @file
 * Copyright (c) 2020-2023, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_memory.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 40)
#define TEST_DESC  "Check Rootport ATS and PRI rule   "
#define TEST_RULE  "IE_SMU_1, IE_SMU_2"

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dp_type;
  uint32_t cap_base;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  test_fails = 0;

  /* Check for all the function present in bdf table */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is rootport */
      if (dp_type == iEP_RP)
      {
          val_print(AVS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

          /* If test runs for atleast an endpoint */
          test_skip = 0;

          /* If ATS capability support for RP, test fails */
          if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_ATS, &cap_base) == PCIE_SUCCESS) {
            val_print(AVS_PRINT_ERR, "\n       BDF - 0x%x does not support ATS Capability  ", bdf);
            test_fails++;
          }

          /* If PRI capability support for RP, test fails */
          if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_PRI, &cap_base) == PCIE_SUCCESS) {
            val_print(AVS_PRINT_ERR, "\n       BDF - 0x%x does not support PRI Capability  ", bdf);
            test_fails++;
          }
      }
  }

  if (test_skip == 1) {
      val_print(AVS_PRINT_DEBUG, "\n       No iEP_RP/ RP type device found. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p040_entry(uint32_t num_pe)
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
