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

#include "val/include/sbsa_avs_wd.h"

#define TEST_NUM   (AVS_WD_TEST_NUM_BASE + 2)
#define TEST_DESC  "Check Watchdog WS0 interrupt      "

static uint32_t int_id;
static uint64_t wd_num;

static
void
isr()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  val_wd_set_ws0(wd_num, 0);
  val_print(AVS_PRINT_DEBUG, "\n       Received WS0 interrupt            ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  val_gic_end_of_interrupt(int_id);
}


static
void
payload()
{

  uint32_t timeout, ns_wdg = 0;
  uint64_t timer_expire_ticks = 100;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  wd_num = val_wd_get_info(0, WD_INFO_COUNT);


  if (wd_num == 0) {
      val_print(AVS_PRINT_WARN, "\n       No Watchdogs reported          %d  ", wd_num);
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  do {
      wd_num--;         //array index starts from 0, so subtract 1 from count

      if (val_wd_get_info(wd_num, WD_INFO_ISSECURE))
          continue;    //Skip Secure watchdog

      ns_wdg++;
      timeout = TIMEOUT_LARGE;
      val_set_status(index, RESULT_PENDING(g_sbsa_level, TEST_NUM));     // Set the initial result to pending

      int_id       = val_wd_get_info(wd_num, WD_INFO_GSIV);
      val_print(AVS_PRINT_DEBUG, "\n       WS0 Interrupt id  %d        ", int_id);

      val_gic_install_isr(int_id, isr);

      val_wd_set_ws0(wd_num, timer_expire_ticks);

      while ((--timeout > 0) && (IS_RESULT_PENDING(val_get_status(index))));

      if (timeout == 0) {
          val_print(AVS_PRINT_ERR, "\n       WS0 Interrupt not received on %d   ", int_id);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
          return;
      }

  } while(wd_num);

  if(!ns_wdg) {
      val_print(AVS_PRINT_WARN, "\n       No non-secure Watchdogs reported", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
      return;
  }

}

uint32_t
w002_entry(uint32_t num_pe)
{

  uint32_t error_flag = 0;
  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);

  val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  error_flag = val_check_for_error(TEST_NUM, num_pe);

  if (!error_flag)
      status = AVS_STATUS_PASS;

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));
  return status;

}
