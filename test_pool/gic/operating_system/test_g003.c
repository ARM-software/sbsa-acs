/** @file
 * Copyright (c) 2016-2018, 2021-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/common/include/acs_common.h"

#include "val/common/include/acs_pe.h"
#include "val/common/include/acs_gic.h"
#include "val/common/include/acs_gic_support.h"

#define TEST_NUM   (ACS_GIC_TEST_NUM_BASE + 3)
#define TEST_RULE  "S_L3PP_01"
#define TEST_DESC  "Check All PPI Interrupt IDs           "



static void payload(void)
{
  uint64_t data;
  uint32_t intid;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());


  if (g_sbsa_level < 3) {
      val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  /* Check non-secure physical timer Private Peripheral Interrupt (PPI) assignment */
  intid = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID, 0);
  if (intid != 30) {
      val_print(ACS_PRINT_ERR,
          "\n       EL0-Phy timer not mapped to PPI ID 30, INTID: %d   ", intid);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
      return;
  }

  /* Check non-secure virtual timer Private Peripheral Interrupt (PPI) assignment */
  intid = val_timer_get_info(TIMER_INFO_VIR_EL1_INTID, 0);
  if (intid != 27) {
      val_print(ACS_PRINT_ERR,
          "\n       EL0-Virtual timer not mapped to PPI ID 27, INTID: %d   ", intid);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
      return;
  }

  // Check Exception Level ie. EL1, EL2
  if (val_pe_reg_read(CurrentEL) == AARCH64_EL1) {
      val_print(ACS_PRINT_DEBUG, "\n       Skipping. Test accesses EL2"
                                    " Registers       ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 2));
      return;
  }

  /* This check run only when ARM v8.1 Virtualized Host Extensions are supported */
  data = val_pe_reg_read(ID_AA64MMFR1_EL1);

  /* Check for EL2 virtual timer interrupt, if PE supports 8.1 or greater.
   * ID_AA64MMFR1_EL1 VH, bits [11:8] must be 0x1 */
  if (!((data >> 8) & 0xF)) {
      val_print(ACS_PRINT_DEBUG, "\n       v8.1 VHE not supported on this PE ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 3));
      return;
  }

  /*Get EL2 virtual timer interrupt ID*/
  intid = val_timer_get_info(TIMER_INFO_VIR_EL2_INTID, 0);

  /*Recommended EL2 virtual timer interrupt ID is 28 as per SBSA*/
  if (intid != 28) {
      val_print(ACS_PRINT_ERR, "\n       NS EL2 virtual timer not mapped to PPI ID 28, id %d",
                                                                    intid);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 3));
      return;
  }

  /*Get EL2 physical timer interrupt ID*/
  intid = val_timer_get_info(TIMER_INFO_PHY_EL2_INTID, 0);

  /*Recommended EL2 physical timer interrupt ID is 26 as per SBSA*/
  if (intid != 26) {
      val_print(ACS_PRINT_DEBUG,
            "\n       NS EL2 physical timer not mapped to PPI id 26, INTID: %d ", intid);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 4));
      return;
   }

  /*Check GIC maintenance interrupt*/
  intid = val_pe_get_gmain_gsiv(index);

  /*Recommended GIC maintenance interrupt ID is 25 as per SBSA*/
  if (intid != 25) {
      val_print(ACS_PRINT_ERR,
                 "\n       GIC Maintenance interrupt not mapped to PPI ID 25, id %d", intid);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 5));
      return;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  return;
}


uint32_t g003_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This GIC test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
