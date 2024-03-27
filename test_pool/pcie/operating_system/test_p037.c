/** @file
 * Copyright (c) 2019-2024, Arm Limited or its affiliates. All rights reserved.
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

#include "val/common/include/acs_val.h"
#include "val/sbsa/include/sbsa_val_interface.h"

#include "val/sbsa/include/sbsa_acs_pcie.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_memory.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 37)
#define TEST_DESC  "Check OBFF supported rule         "
#define TEST_RULE  "IE_REG_2"

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

      /* Check entry is iEP endpoint */
      if (dp_type == iEP_EP)
      {
          val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

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

          /* If test runs for atleast an endpoint */
          test_skip = 0;

          /* As per SBSA spec iRP must have same value as of iEP */
          if (ep_obff_support != rp_obff_support)
           {
              val_print(ACS_PRINT_ERR, "\n OBFF Support level of iEP and it's iRP is not same", 0);
              val_print(ACS_PRINT_DEBUG, "\n    iEP 0x%x", bdf);
              val_print(ACS_PRINT_DEBUG, " OBFF support %d", ep_obff_support);
              val_print(ACS_PRINT_DEBUG, "\n    iRP 0x%x", rp_bdf);
              val_print(ACS_PRINT_DEBUG, " OBFF support %d", rp_obff_support);
              test_fails++;
          }
      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG, "\n       No iEP_EP type device found. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
p037_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
