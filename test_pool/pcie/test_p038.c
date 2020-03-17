/** @file
 * Copyright (c) 2019, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 38)
#define TEST_DESC  "Check CTRS and CTDS rule          "

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t dp_type;
  uint32_t cap_base;
  uint32_t ctrs_value;
  uint32_t ctds_value;
  uint32_t test_fails;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  tbl_index = 0;
  test_fails = 0;

  /* Check for all the function present in bdf table */
  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is iRP endpoint */
      if (dp_type == iEP_RP)
      {
          /* If rootport invovled in transaction forwarding, move to next */
          if (val_pcie_get_rp_transaction_frwd_support(bdf))
             continue;

          /* Read rootport Completion Timeout Ranges supported bit value */
          val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &cap_base);
          val_pcie_read_cfg(bdf, cap_base + DCAP2R_OFFSET, &reg_value);
          ctrs_value = (reg_value >> DCAP2R_CTRS_SHIFT) & DCAP2R_CTRS_MASK;

          /* Read rootport Completion Timeout Disable supported bit value */
          val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &cap_base);
          val_pcie_read_cfg(bdf, cap_base + DCAP2R_OFFSET, &reg_value);
          ctds_value = (reg_value >> DCAP2R_CTDS_SHIFT) & DCAP2R_CTDS_MASK;

          /* CTRS and CTDS bit is handwired to 0, if transaction forwarding not support */
          if ((ctrs_value != 0) || (ctds_value !=0))
          {
              val_print(AVS_PRINT_DEBUG, "\n    BDF 0x%x", bdf);
              val_print(AVS_PRINT_DEBUG, " ctrs %d", ctrs_value);
              val_print(AVS_PRINT_DEBUG, " ctds %d", ctds_value);
              test_fails++;
          }
     }
  }

  if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p038_entry(uint32_t num_pe)
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
