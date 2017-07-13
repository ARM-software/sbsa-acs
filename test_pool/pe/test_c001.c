/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

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
#include "val/include/sbsa_avs_pe.h"
#include "val/include/val_interface.h"

#define TEST_NUM   AVS_PE_TEST_NUM_BASE  +  1
#define TEST_DESC  "Check for number of PE            "

static
void
payload()
{
  uint64_t num_of_pe = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  num_of_pe = val_pe_get_num();

  //
  //g_sbsa_level is set based on runtime input to the tool
  //
  if (g_sbsa_level < 2) {
      if (num_of_pe > MAX_NUM_PE_LEVEL0) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          val_print(AVS_PRINT_ERR, "Number of PE is %d \n", num_of_pe);
          return;
      }
  } else {
      if (num_of_pe > MAX_NUM_PE_LEVEL2) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          val_print(AVS_PRINT_ERR, "Number of PE is %d \n", num_of_pe);
          return;
      }

  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

  return;

}

uint32_t
c001_entry()
{

  uint32_t status = AVS_STATUS_FAIL;  //default value
  uint32_t num_pe = 1;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);

  /* This check is when user is forcing us to skip this test */
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}