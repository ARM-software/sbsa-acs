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
#include "val/common/include/acs_timer.h"

#define TEST_NUM   (ACS_TIMER_TEST_NUM_BASE + 1)
#define TEST_RULE  "S_L8TI_01"
#define TEST_DESC  "Check Minimum Counter Frequency 50MHz "

static
void
payload()
{

  uint64_t counter_freq, print_freq;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t print_mhz = 0;

  if (g_sbsa_level < 8) {
      val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  counter_freq = val_timer_get_info(TIMER_INFO_CNTFREQ, 0);

  if (counter_freq != 0) {
    print_freq = counter_freq/1000;
    if (print_freq > 1000) {
      print_freq = print_freq/1000;
      print_mhz = 1;
    }
  }

  if (print_mhz)
    val_print(ACS_PRINT_ERR, "\n       Counter frequency is %ld MHz", print_freq);
  else
    val_print(ACS_PRINT_ERR, "\n       Counter frequency is %ld KHz", print_freq);

  if (counter_freq >= 50*1000*1000) {
      val_set_status(index, RESULT_PASS(TEST_NUM, 1));
      return;
  }

  val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
}

uint32_t
t001_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);
  return status;
}
