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

#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/common/include/acs_iovirt.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 64)
#define TEST_DESC  "Check ATS support for RC              "
#define TEST_RULE  "GPU_04"

static void payload(void)
{

  uint32_t pe_index;
  uint32_t num_pcie_rc;
  uint32_t rc_ats_attr;
  uint32_t rc_ats_supp;
  uint32_t test_fails;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  test_fails = 0;

  num_pcie_rc = val_iovirt_get_pcie_rc_info(NUM_PCIE_RC, 0);

  /* Get the number of Root Complex in the system */
  if (!num_pcie_rc) {
     val_print(ACS_PRINT_DEBUG, "\n       Skip because no PCIe RC detected  ", 0);
     val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 1));
     return;
  }

  /* For each Root Complex, check if it supports ATS capability.
   * This information should be obtained from ACPI-IORT table for UEFI based
   * systems and platform config file for Baremetal based system
   */
  while (num_pcie_rc) {
      num_pcie_rc--;   // Index is one lesser than the component number being accessed
      rc_ats_attr = val_iovirt_get_pcie_rc_info(RC_ATS_ATTRIBUTE, num_pcie_rc);
      rc_ats_supp = rc_ats_attr & 1;

      if (!rc_ats_supp)
      {
          val_print(ACS_PRINT_ERR, "\n       ATS Capability Not Present for RC: %x", num_pcie_rc);
          test_fails++;
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
