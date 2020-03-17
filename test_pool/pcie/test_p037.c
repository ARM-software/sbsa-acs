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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 37)
#define TEST_DESC  "Check OBFF supported rule         "

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t rp_bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t dp_type;
  uint32_t cap_base;
  uint32_t ep_obff_support;
  uint32_t rp_obff_support;
  uint32_t test_fails;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  test_fails = 0;

  /* Check for all the function present in bdf table */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is iEP endpoint */
      if (dp_type == iEP_EP)
      {
          /* Read endpoint OBFF supported bit value */
          val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &cap_base);
          val_pcie_read_cfg(bdf, cap_base + DCAP2R_OFFSET, &reg_value);
          ep_obff_support = (reg_value >> DCAP2R_OBFF_SHIFT) & DCAP2R_OBFF_MASK;

          /* Get the rootport of ARI device */
          rp_bdf = bdf_tbl_ptr->device[tbl_index].rp_bdf;

          /* Read rootport OBFF supported bit value */
          val_pcie_find_capability(rp_bdf, PCIE_CAP, CID_PCIECS, &cap_base);
          val_pcie_read_cfg(rp_bdf, cap_base + DCAP2R_OFFSET, &reg_value);
          rp_obff_support = (reg_value >> DCAP2R_OBFF_SHIFT) & DCAP2R_OBFF_MASK;

          /* As per SBSA spec iRP must have same value as of iEP */
          if (ep_obff_support != rp_obff_support)
           {
              val_print(AVS_PRINT_DEBUG, "\n    iEP 0x%x", bdf);
              val_print(AVS_PRINT_DEBUG, " OBFF support %d", ep_obff_support);
              val_print(AVS_PRINT_DEBUG, "\n    iRP 0x%x", rp_bdf);
              val_print(AVS_PRINT_DEBUG, " OBFF support %d", rp_obff_support);
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
p037_entry(uint32_t num_pe)
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
