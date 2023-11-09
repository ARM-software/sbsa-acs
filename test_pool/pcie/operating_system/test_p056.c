/** @file
 * Copyright (c) 2020-2023, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 56)
#define TEST_DESC  "Check iEP-RootPort P2P Support    "
#define TEST_RULE  "IE_ACS_2"

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
  uint32_t iep_rp_bdf;
  uint32_t curr_bdf_failed = 0;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Check If PCIe Hierarchy supports P2P */
  if (val_pcie_p2p_support() == NOT_IMPLEMENTED) {
    val_print(AVS_PRINT_DEBUG, "\n       The test is applicable only if the system supports", 0);
    val_print(AVS_PRINT_DEBUG, "\n       P2P traffic. If the system supports P2P, pass the", 0);
    val_print(AVS_PRINT_DEBUG, "\n       command line option '-p2p' while running the binary", 0);
    val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  if (val_pcie_p2p_support())
  {
      val_print(AVS_PRINT_DEBUG, "\n       PCIe P2P unsupported ", 0);
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
      return;
  }

  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  test_fails = 0;

  /* Check for all the function present in bdf table */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is iEP_EP */
      if (dp_type == iEP_EP)
      {
          val_print(AVS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);
          /* Check If iEP_EP supports P2P with others. */
          if (val_pcie_dev_p2p_support(bdf))
              continue;

          /* If test runs for atleast an endpoint */
          test_skip = 0;

          /* Find iEP_RP for this iEP_EP */
          if (val_pcie_get_rootport(bdf, &iep_rp_bdf))
          {
              val_print(AVS_PRINT_ERR, "\n       Root Port Not found for iEP_EP 0x%x", bdf);
              test_fails++;
              continue;
          }

          /* Read the ACS Capability */
          if (val_pcie_find_capability(iep_rp_bdf, PCIE_ECAP, ECID_ACS, &cap_base) != PCIE_SUCCESS)
          {
              val_print(AVS_PRINT_ERR, "\n       ACS Capability not supported, Bdf : 0x%x", iep_rp_bdf);
              test_fails++;
              continue;
          }

          val_pcie_read_cfg(iep_rp_bdf, cap_base + ACSCR_OFFSET, &acs_data);

          /* Extract ACS source validation bit */
          data = VAL_EXTRACT_BITS(acs_data, 0, 0);
          if (data == 0) {
              val_print(AVS_PRINT_DEBUG, "\n       Source validation not supported, Bdf : 0x%x", iep_rp_bdf);
              curr_bdf_failed++;
          }

          /* Extract ACS translation blocking bit */
          data = VAL_EXTRACT_BITS(acs_data, 1, 1);
          if (data == 0) {
              val_print(AVS_PRINT_DEBUG, "\n       Translation blocking not supported, Bdf : 0x%x", iep_rp_bdf);
              curr_bdf_failed++;
          }

          /* Extract ACS P2P request redirect bit */
          data = VAL_EXTRACT_BITS(acs_data, 2, 2);
          if (data == 0) {
              val_print(AVS_PRINT_DEBUG, "\n       P2P request redirect not supported, Bdf : 0x%x", iep_rp_bdf);
              curr_bdf_failed++;
          }

          /* Extract ACS P2P completion redirect bit */
          data = VAL_EXTRACT_BITS(acs_data, 3, 3);
          if (data == 0) {
              val_print(AVS_PRINT_DEBUG, "\n       P2P completion redirect not supported, Bdf : 0x%x", iep_rp_bdf);
              curr_bdf_failed++;
          }

          /* Extract ACS upstream forwarding bit */
          data = VAL_EXTRACT_BITS(acs_data, 4, 4);
          if (data == 0) {
              val_print(AVS_PRINT_DEBUG, "\n       Upstream forwarding not supported, Bdf : 0x%x", iep_rp_bdf);
              curr_bdf_failed++;
          }

          /* If iEP_RP supports ACS then it must have AER Capability */
          if (val_pcie_find_capability(iep_rp_bdf, PCIE_ECAP, ECID_AER, &cap_base) != PCIE_SUCCESS)
          {
              val_print(AVS_PRINT_DEBUG, "\n       AER Capability not supported, Bdf : 0x%x", iep_rp_bdf);
              curr_bdf_failed++;
          }

          if(curr_bdf_failed > 0) {
              val_print(AVS_PRINT_ERR, "\n       ACS Capability Check Failed, Bdf : 0x%x", iep_rp_bdf);
              curr_bdf_failed = 0;
              test_fails++;
          }
      }
  }

  if (test_skip == 1) {
      val_print(AVS_PRINT_DEBUG,
           "\n       No iEP_EP type device found with P2P support. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 03));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p056_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
