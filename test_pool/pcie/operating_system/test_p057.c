/** @file
 * Copyright (c) 2020-2024, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 57)
#define TEST_RULE  "IE_ACS_1, RE_ACS_1, RE_ACS_2"
#define TEST_DESC  "Check RCiEP, iEP_EP P2P Supp          "

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dp_type;
  uint32_t cap_base = 0;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  uint32_t acs_data;
  uint32_t data;
  uint8_t p2p_support_flag = 0;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  test_fails = 0;

  /* Check for all the function present in bdf table */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is RCiEP or iEP end point */
      if ((dp_type == RCiEP) || (dp_type == iEP_EP))
      {
          val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

          /* Check if the EP Supports Multifunction */
          if (val_pcie_multifunction_support(bdf))
              continue;

          /* Check If Endpoint supports P2P with other Functions. */
          if (val_pcie_dev_p2p_support(bdf))
              continue;

          /* If test runs for atleast an endpoint */
          test_skip = 0;

          /* Read the ACS Capability */
          if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_ACS, &cap_base) != PCIE_SUCCESS)
          {
              val_print(ACS_PRINT_ERR, "\n       ACS Capability not supported, Bdf : 0x%x", bdf);
              test_fails++;
              continue;
          }

          val_pcie_read_cfg(bdf, cap_base + ACSCR_OFFSET, &acs_data);

          /* Extract ACS p2p Request Redirect bit */
          data = VAL_EXTRACT_BITS(acs_data, 2, 2);
          if (data == 0) {
              val_print(ACS_PRINT_DEBUG, "\n       Request Redirect P2P not supported", 0);
              p2p_support_flag++;
          }

          /* Extract ACS p2p Completion Redirect bit */
          data = VAL_EXTRACT_BITS(acs_data, 3, 3);
          if (data == 0) {
              val_print(ACS_PRINT_DEBUG, "\n       Completion Redirect P2P not supported", 0);
              p2p_support_flag++;
          }

          /* Extract ACS p2p Direct Translated bit */
          data = VAL_EXTRACT_BITS(acs_data, 6, 6);
          if (data == 0) {
              val_print(ACS_PRINT_DEBUG, "\n       Direct Translated P2P not supported", 0);
              p2p_support_flag++;
          }

          if (p2p_support_flag > 0) {
              val_print(ACS_PRINT_ERR, "\n       P2P not supported for bdf: %d", bdf);
              p2p_support_flag = 0;
              test_fails++;
          }
          /* If device supports ACS then it must have AER Capability */
          if (val_pcie_find_capability(bdf, PCIE_ECAP, ECID_AER, &cap_base) != PCIE_SUCCESS)
          {
              val_print(ACS_PRINT_ERR, "\n       AER Capability not supported, Bdf : 0x%x", bdf);
              test_fails++;
          }
      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG,
      "\n       No RCiEP/ iEP_EP type device with Multifunction and P2P support.Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
p057_entry(uint32_t num_pe)
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
