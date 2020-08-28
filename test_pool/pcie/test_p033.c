/** @file
 * Copyright (c) 2019-2020, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 33)
#define TEST_DESC  "Check Max payload size supported  "

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t max_payload_value;
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
          val_print(AVS_PRINT_ERR, "\n        BDF 0x%x", bdf);
          val_print(AVS_PRINT_ERR, " Cap Ptr Value: 0x%x", max_payload_value);
          test_fails++;
      }
  }

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p033_entry(uint32_t num_pe)
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
