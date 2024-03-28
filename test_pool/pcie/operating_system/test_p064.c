/** @file
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
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
#include "val/common/include/val_interface.h"

#include "val/common/include/acs_memory.h"
#include "val/sbsa/include/sbsa_acs_pcie.h"
#include "val/sbsa/include/sbsa_acs_pe.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 64)
#define TEST_DESC  "Check ATS & Page Req for all RP   "
#define TEST_RULE  "GPU_04"

static void payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dp_type;
  uint32_t cap_base;
  uint32_t test_fails;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  test_fails = 0;

  /* Check for all the device present in bdf table */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check port type is Root port */
      if (dp_type == RP)
      {
         val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);
         /* For All Root Ports ATS Capability must be present. */
         if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_ATS, &cap_base) != PCIE_SUCCESS)
         {
             val_print(ACS_PRINT_ERR, "\n       ATS Capability Not Present, Bdf : 0x%x", bdf);
             test_fails++;
         }

         /* For All Roots Ports Page Reguest Extended Capability must be present. */
         if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_PRI, &cap_base) != PCIE_SUCCESS)
         {
             val_print(ACS_PRINT_ERR, "\n       Page Request Not Present, Bdf : 0x%x", bdf);
             test_fails++;
         }

      }
  }

  if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t p064_entry(uint32_t num_pe)
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
