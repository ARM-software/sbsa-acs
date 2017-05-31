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
#include "val/include/val_interface.h"

#include "val/include/sbsa_avs_timer.h"
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM   (AVS_TIMER_TEST_NUM_BASE + 6)
#define TEST_DESC  "SYS Timer if PE Timer not ON      "

static
void
payload()
{

  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  if (val_timer_get_info(TIMER_INFO_NUM_PLATFORM_TIMERS, 0) == 0) {
      val_print(AVS_PRINT_INFO, "\n Physical EL1 timer flag = %x", val_timer_get_info(TIMER_INFO_PHY_EL1_FLAGS, 0));
      val_print(AVS_PRINT_INFO, "\n Physical EL2 timer flag = %x", val_timer_get_info(TIMER_INFO_PHY_EL2_FLAGS, 0));
      val_print(AVS_PRINT_INFO, "\n Virtual EL1 timer flag  = %x", val_timer_get_info(TIMER_INFO_VIR_EL1_FLAGS, 0));

      if((val_timer_get_info(TIMER_INFO_PHY_EL1_FLAGS, 0) & SBSA_TIMER_FLAG_ALWAYS_ON) &&
        (val_timer_get_info(TIMER_INFO_PHY_EL2_FLAGS, 0) & SBSA_TIMER_FLAG_ALWAYS_ON) &&
        (val_timer_get_info(TIMER_INFO_VIR_EL1_FLAGS, 0) & SBSA_TIMER_FLAG_ALWAYS_ON)) {
          val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
      } else {
          val_print(AVS_PRINT_ERR, "\n       PE Timers are not always-on.       ", 0);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
      }
  } else {
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 02));
  }

}

uint32_t
t006_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));
  return status;

}
