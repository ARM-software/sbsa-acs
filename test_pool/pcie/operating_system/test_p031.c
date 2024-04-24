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

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 31)
#define TEST_DESC  "Check Type0/1 BIST Register rule      "
#define TEST_RULE  "RE_REG_1, IE_REG_1, IE_REG_3"

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t dp_type;
  uint32_t test_fails;
  uint32_t test_skip = 1;
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

      /* Read 32-bits from Cache Line Size register offset */
      val_pcie_read_cfg(bdf, TYPE01_CLSR, &reg_value);

      /* Extract BIST register value */
      reg_value = VAL_EXTRACT_BITS(reg_value, BIST_REG_START, BIST_REG_END);

      /* If test runs for atleast an endpoint */
      test_skip = 0;

      /*
       * If BIST Capable bit[7] is clear Completion Code[0:3] and Start Bist[6]
       * must be hardwired to 0b
       */
      if (((reg_value & BIST_BC_MASK) == 0x00) &&
         (((reg_value & BIST_CC_MASK) != 0x00) || ((reg_value & BIST_SB_MASK) != 0x00)))
      {
          val_print(ACS_PRINT_ERR, "\n       BDF - 0x%x", bdf);
          val_print(ACS_PRINT_ERR, " BIST Reg Value : %d", reg_value);
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
p031_entry(uint32_t num_pe)
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
