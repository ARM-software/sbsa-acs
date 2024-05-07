/** @file
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (ACS_PER_TEST_NUM_BASE + 01)
#define TEST_DESC  "Check EA Capability                   "
#define TEST_RULE  "S_L4PCI_2"

static
void
payload(void)
{

  uint64_t num_ecam;
  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t enable_value;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  uint32_t cap_base;
  uint32_t status;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  tbl_index = 0;
  test_fails = 0;

  num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);

  if (num_ecam == 0) {
      val_print(ACS_PRINT_ERR, "\n       No ECAMs discovered              ", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

      /* If test runs for atleast an endpoint */
      test_skip = 0;

      /* Retrieve the addr of Enhanced Allocation capability (14h) and check if the
       * capability structure is not supported.
       */
      status = val_pcie_find_capability(bdf, PCIE_CAP, CID_EA, &cap_base);
      if (status == PCIE_CAP_NOT_FOUND)
          continue;

      /* Read Entry type register(08h) present in Enhanced Allocation capability struct(10h) */
      val_pcie_read_cfg(bdf, cap_base + EA_ENTRY_TYPE_OFFSET, &reg_value);

      /* Extract enable value */
      enable_value = (reg_value >> EA_ENTRY_TYPE_ENABLE_SHIFT) & EA_ENTRY_TYPE_ENABLE_MASK;
      if (enable_value)
      {
          val_print(ACS_PRINT_ERR, "\n       Failed. BDF 0x%x Supports Enhanced Allocation", bdf);
          test_fails++;
      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG,
               "\n       Found no Endpoint with PCIe Capability. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
p062_entry(uint32_t num_pe)
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
