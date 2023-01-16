/** @file
 * Copyright (c) 2021-2023, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 59)
#define TEST_RULE  "RE_PCI_2"
#define TEST_DESC  "Check RCEC Class code and Ext Cap     "

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t dp_type;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t fail_cnt;
  uint32_t test_skip = 1;
  uint32_t cap_base;
  pcie_device_bdf_table *bdf_tbl_ptr;
  uint32_t reg_value;

  fail_cnt = 0;
  tbl_index = 0;
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);
      if (dp_type == RCEC) {
          /* If test runs for atleast an endpoint */
          test_skip = 0;

          /* Read Function's Class Code */
          val_pcie_read_cfg(bdf, TYPE01_RIDR, &reg_value);

          if ((RCEC_BASE_CLASS != ((reg_value >> CC_BASE_SHIFT) & CC_BASE_MASK)) ||
                (RCEC_SUB_CLASS != ((reg_value >> CC_SUB_SHIFT) & CC_SUB_MASK)) ||
                (RCEC_PGMING_IF != ((reg_value >> CC_PGM_IF_SHIFT) & CC_PGM_IF_MASK)))
          {
              val_print(AVS_PRINT_ERR, "        Class code mismatch for bdf: 0x%x\n", bdf);
              val_print(AVS_PRINT_ERR, "        dp_type: 0x%x\n", dp_type);
              val_print(AVS_PRINT_ERR, "        CCR: 0x%x\n", reg_value);
              fail_cnt++;
          }

          /* If Root Complex Event
            Collector Endpoint Association Extended Capability not supported for RCEC, test fails*/
          if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_RCECEA, &cap_base) != PCIE_SUCCESS) {
              fail_cnt++;
              continue;
          }

      }
  }

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 1));
  else if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 1));
}

uint32_t
p059_entry(uint32_t num_pe)
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
