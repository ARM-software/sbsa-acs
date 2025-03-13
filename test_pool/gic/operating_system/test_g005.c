/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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

#include "val/sbsa/include/sbsa_acs_gic.h"

#define TEST_NUM   (ACS_GIC_TEST_NUM_BASE + 5)
#define TEST_RULE  "S_L5GI_01"
#define TEST_DESC  "Check Non GIC Interrupts              "

static
void
payload(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_non_gic = 0;

  if (g_sbsa_level < 5) {
      val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  num_non_gic = val_get_num_nongic_ctrl();

  val_print(ACS_PRINT_DEBUG, "\n       Non GIC Interrupt count: %d", num_non_gic);
  if (num_non_gic > 0) {
      val_print(ACS_PRINT_ERR, "\n       Non GIC Interrupt found", 0);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
      return;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 01));
  return;
}

uint32_t
g005_entry(uint32_t num_pe)
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
