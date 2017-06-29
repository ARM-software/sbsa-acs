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
#include "val/include/sbsa_avs_secure.h"

#define TEST_NUM   (AVS_SECURE_TEST_NUM_BASE + 1)
#define TEST_DESC  "Check NS Watchdog WS1 interrupt   "

static
void
isr()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_print(AVS_PRINT_DEBUG, "\n       Received WS0 interrupt    ", 0);

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}


static
void
payload()
{
  uint32_t int_id_ws0, int_id_ws1, ns_wdg = 0;
  uint64_t wd_num = val_wd_get_info(0, WD_INFO_COUNT);
  uint32_t timeout = 2, timeout_intr;
  uint32_t timer_expire_ticks = 1000;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  SBSA_SMC_t  smc;


  if (wd_num == 0) {
      val_print(AVS_PRINT_WARN, "\n       No Watchdogs reported          %d  ", wd_num);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  do
  {
      wd_num--;         //array index starts from 0, so subtract 1 from count

      if (val_wd_get_info(wd_num, WD_INFO_ISSECURE))
          continue;    //Skip Secure watchdog

      ns_wdg++;
      timeout_intr = TIMEOUT_LARGE;
      val_set_status(index, RESULT_PENDING(g_sbsa_level, TEST_NUM));     // Set the initial result to pending

      int_id_ws0 = val_wd_get_info(wd_num, WD_INFO_GSIV);
      int_id_ws1 = val_wd_get_info(wd_num+1, WD_INFO_GSIV);  // ACPI table for WS1 may need to be populated
      val_print(AVS_PRINT_DEBUG, "\n       WS0 Interrupt id  %d        ", int_id_ws0);
      val_print(AVS_PRINT_DEBUG, "\n       WS1 Interrupt id  %d        ", int_id_ws1);

      val_gic_install_isr(int_id_ws0, isr);
      val_gic_install_isr(int_id_ws1, isr); // ISR doesn't matter here,
                                            // because interrupt is routed to EL3

      val_wd_set_ws0(wd_num, timer_expire_ticks);

      while (!(IS_TEST_PASS(val_get_status(index))) && (--timeout_intr));

      if(timeout_intr == 0){
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          val_print(AVS_PRINT_WARN, "\n       WS0 Interrupt was not generated", 0);
          return;
      }

      smc.test_index = SBSA_SECURE_TEST_NSWD_WS1,
      smc.test_arg01 = int_id_ws1;
      val_secure_call_smc(&smc);

      switch (val_secure_get_result(&smc, timeout))
      {
          case AVS_STATUS_PASS:
              val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 02));
              break;
          case AVS_STATUS_FAIL:
              val_print(AVS_PRINT_ERR, "\n       WS1 Interrupt not received", 0);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
              break;
          default:
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
      }

      // Stop watchdog and signal end of interrupt to gic, it should be done after receiving both
      // WS0 and WS1 interrupts
      val_wd_set_ws0(wd_num, 0);
      val_gic_end_of_interrupt(int_id_ws0);

  }while(wd_num);

  if(!ns_wdg) {
      val_print(AVS_PRINT_WARN, "\n       No non-secure Watchdogs reported", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
      return;
  }

}

uint32_t
s001_entry(uint32_t num_pe)
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
