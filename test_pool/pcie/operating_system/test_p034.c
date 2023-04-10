/** @file
 * Copyright (c) 2019, 2021-2023 Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 34)
#define TEST_DESC  "Check BAR memory space & Type rule"
#define TEST_RULE  "RE_BAR_3"

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t hdr_type;
  uint32_t max_bar;
  uint32_t addr_type;
  uint32_t bar_index;
  uint32_t  dp_type;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  tbl_index = 0;
  test_fails = 0;

  /* Check for all the function present in bdf table */
  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check for RCiEP, iEP, RCEC*/
      if (dp_type == RCiEP || dp_type == iEP_EP || dp_type == iEP_RP || dp_type == RCEC)
      {
          val_print(AVS_PRINT_DEBUG, "\n       BDF - 0x%x ", bdf);
          /* Extract Hdr Type */
          hdr_type = val_pcie_function_header_type(bdf);
          val_print(AVS_PRINT_INFO, "\n       HDR TYPE 0x%x ", hdr_type);

          max_bar = 0;
          /* For Type0 header max bars 6, type1 header max bars 2 */
          if (hdr_type == TYPE0_HEADER)
             max_bar = TYPE0_MAX_BARS;
          else if (hdr_type == TYPE1_HEADER)
             max_bar = TYPE1_MAX_BARS;
          val_print(AVS_PRINT_INFO, "\n       MAX BARS 0x%x ", max_bar);

          for (bar_index = 0; bar_index < max_bar; bar_index++)
          {
              /* Read BAR0 register */
              val_pcie_read_cfg(bdf, TYPE01_BAR + (bar_index * BAR_BASE_SHIFT), &reg_value);

              /* If BAR not in use skip the BAR */
              if (reg_value == 0)
                  continue;

              /* If test runs for atleast an endpoint */
              test_skip = 0;

              /* Check type[1:2] must be 32-bit or 64-bit */
              addr_type = (reg_value >> BAR_MDT_SHIFT) & BAR_MDT_MASK;
              if ((addr_type != BITS_32) && (addr_type != BITS_64))
              {
                  val_print(AVS_PRINT_ERR, "\n       BDF 0x%x ", bdf);
                  val_print(AVS_PRINT_ERR, " Addr Type: 0x%x", addr_type);
                  test_fails++;
                  continue;
              }

              /* if BAR is 64 bit move index to next BAR */
              if (addr_type == BITS_64)
                  bar_index++;

              /* Check BAR must be MMIO */
              if (reg_value & BAR_MIT_MASK)
              {
                 val_print(AVS_PRINT_ERR, "\n       BDF 0x%x Not MMIO", 0);
                 test_fails++;
              }
           }
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
p034_entry(uint32_t num_pe)
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
