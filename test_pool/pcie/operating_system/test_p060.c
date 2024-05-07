/** @file
 * Copyright (c) 2021-2024, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 60)
#define TEST_RULE  "RE_PCI_1"
#define TEST_DESC  "Check RCiEP Hdr type & link Cap       "

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
  pcie_device_bdf_table *bdf_tbl_ptr;
  uint32_t hdr_type;
  uint32_t link_cap_sup;

  fail_cnt = 0;
  tbl_index = 0;
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);
      if ((dp_type == RCiEP) || (dp_type == RCEC)) {

          val_print(ACS_PRINT_ERR, "\n       BDF - 0x%x", bdf);
          /* If test runs for atleast an endpoint */
          test_skip = 0;

          /* Extract Hdr Type */
          hdr_type = val_pcie_function_header_type(bdf);
          /* Type must be 0 for RCiEP*/
          if (hdr_type != TYPE0_HEADER) {
              val_print(ACS_PRINT_ERR, "\n       Invalid HDR TYPE 0x%x", hdr_type);
              fail_cnt++;
              continue;
          }

          link_cap_sup = val_pcie_link_cap_support(bdf);
          if (link_cap_sup != 0) {
              val_print(ACS_PRINT_ERR, "\n       Invalid Link Capabilities 0x%x", link_cap_sup);
              fail_cnt++;
          }

      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG, "\n       No RCiEP/ RCEC type device found. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 1));
  }
  else if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 1));
}

uint32_t
p060_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from the PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
