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

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 33)
#define TEST_DESC  "Check Max payload size supported  "
#define TEST_RULE  "RE_REC_1, IE_REG_2, IE_REG_4"

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  int32_t max_payload_value;
  uint32_t dp_type;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  uint32_t cap_base;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  tbl_index = 0;
  test_fails = 0;

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is RCiEP/ RCEC/ iEP. Else move to next BDF. */
      if ((dp_type != iEP_EP) && (dp_type != iEP_RP)
           && (dp_type != RCEC) && (dp_type != RCiEP))
          continue;

      val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

      /* Retrieve the addr of PCI express capability (10h) */
      val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &cap_base);

      /* Read Device Capabilities register(04h) present in PCIE capability struct(10h) */
      val_pcie_read_cfg(bdf, cap_base + DCAPR_OFFSET, &reg_value);

      /* Extract Max payload Size Supported value */
      max_payload_value = (reg_value >> DCAPR_MPSS_SHIFT) & DCAPR_MPSS_MASK;

      /* If test runs for atleast an endpoint */
      test_skip = 0;

      /* Valid payload size between 000b (129-bytes) to 101b (4096 bytes) */
      if (!((max_payload_value >= 0x00) && (max_payload_value <= 0x05)))
      {
          val_print(ACS_PRINT_ERR, "\n        BDF 0x%x", bdf);
          val_print(ACS_PRINT_ERR, " Cap Ptr Value: 0x%x", max_payload_value);
          test_fails++;
      }
  }

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
p033_entry(uint32_t num_pe)
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
