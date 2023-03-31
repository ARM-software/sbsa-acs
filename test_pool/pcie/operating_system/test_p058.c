/** @file
 * Copyright (c) 2021-2023, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 58)
#define TEST_RULE  "RE_BAR_1"
#define TEST_DESC  "Read and write to RCiEP BAR reg   "

#define TEST_DATA_1  0xDEADDAED
#define TEST_DATA_2  0xABABABAB

static
void
payload(void)
{
  uint32_t bdf, offset;
  uint32_t bar_value;
  uint32_t bar_value_1;
  uint32_t bar_reg_value;
  uint32_t base_lower;
  uint32_t base_upper;
  uint32_t dp_type;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t fail_cnt;
  uint32_t test_skip = 1;
  pcie_device_bdf_table *bdf_tbl_ptr;
  uint64_t bar_orig;
  uint64_t bar_new;
  uint64_t bar_upper_bits;

  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  fail_cnt = 0;
  tbl_index = 0;

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);
      if (dp_type == RCiEP) {
          /* If test runs for atleast an endpoint */
          test_skip = 0;
          offset = BAR0_OFFSET;

           while (offset <= BAR_TYPE_1_MAX_OFFSET) {
              val_pcie_read_cfg(bdf, offset, &bar_value);
              val_print(AVS_PRINT_DEBUG, "\n       BDF - 0x%x ", bdf);
              val_print(AVS_PRINT_DEBUG, "BAR offset0x%x value", offset);
              val_print(AVS_PRINT_DEBUG, " is 0x%x     ", bar_value);
              bar_orig = 0;
              bar_new = 0;

              if (bar_value == 0)
              {
                  /** This BAR is not implemented **/
                 offset = offset + 4;
                 continue;
              }

              if (BAR_REG(bar_value) == BAR_64_BIT)
              {
                  val_print(AVS_PRINT_INFO,
                           "\n       The BAR supports 64-bit address capability", 0);
                  val_pcie_read_cfg(bdf, offset+4, &bar_value_1);
                  base_upper = bar_value_1;

                 /* BAR supports 64-bit address therefore, write all 1's
                  * to BARn and BARn+1 and identify the size requested
                  */
                 val_pcie_read_cfg(bdf, offset, &bar_reg_value);
                 bar_value = bar_reg_value;
                 base_lower = bar_reg_value;
                 val_pcie_read_cfg(bdf, offset + 4, &bar_reg_value);
                 bar_upper_bits = bar_reg_value;
                 base_upper = bar_reg_value;
                 bar_orig = (bar_upper_bits << 32) | bar_value;
                 val_pcie_write_cfg(bdf, offset, TEST_DATA_1);
                 val_pcie_write_cfg(bdf, offset + 4, TEST_DATA_2);
                 val_pcie_read_cfg(bdf, offset, &bar_reg_value);
                 bar_value = bar_reg_value;
                 val_pcie_read_cfg(bdf, offset + 4, &bar_reg_value);
                 bar_upper_bits = bar_reg_value;
                 bar_new = (bar_upper_bits << 32) | bar_value;
                if (bar_orig == bar_new) {
                    val_print(AVS_PRINT_DEBUG, "\n       Value read from BAR 0x%llx", bar_new);
                    val_print(AVS_PRINT_ERR,
                              "\n       Read write to BAR reg not supported bdf %x", bdf);
                    fail_cnt++;
                }

                 /* Restore the original BAR value */
                 val_pcie_write_cfg(bdf, offset + 4, base_upper);
                 val_pcie_write_cfg(bdf, offset, base_lower);
                 offset = offset+8;
              }

             else {
                 val_print(AVS_PRINT_INFO,
                           "\n       The BAR supports 32-bit address capability", 0);

                 /* BAR supports 32-bit address. Write all 1's
                  * to BARn and identify the size requested
                  */
                 val_pcie_read_cfg(bdf, offset, &bar_reg_value);
                 bar_orig = bar_reg_value;
                 base_lower = bar_reg_value;
                 val_pcie_write_cfg(bdf, offset, TEST_DATA_1);
                 val_pcie_read_cfg(bdf, offset, &bar_reg_value);
                 bar_new = bar_reg_value;
                 if (bar_orig == bar_new) {
                    val_print(AVS_PRINT_DEBUG,
                              "\n       Value written into BAR 0x%x", TEST_DATA_1);
                    val_print(AVS_PRINT_DEBUG, " Value read from BAR 0x%x", bar_new);
                    val_print(AVS_PRINT_ERR,
                              "\n       Read write to BAR reg not supported bdf %x", bdf);
                    fail_cnt++;
                 }

                 /* Restore the original BAR value */
                 val_pcie_write_cfg(bdf, offset, base_lower);
                 offset = offset+4;
              }
          }
      }


  if (test_skip == 1) {
      val_print(AVS_PRINT_DEBUG, "\n       No RCiEP type device found. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 1));
  }
  else if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 1));
      }
}

uint32_t
p058_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from the PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
