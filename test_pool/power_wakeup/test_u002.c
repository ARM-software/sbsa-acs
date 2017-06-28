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

#include "val/include/sbsa_avs_wakeup.h"
#include "val/include/sbsa_std_smc.h"

#define TEST_NUM   (AVS_WAKEUP_TEST_NUM_BASE + 6)
#define TEST_DESC  "Test No-Wake from Power Semantic F"

static uint32_t intid, wakeup_event, cnt_base_n=0;
static uint64_t timer_num, wd_num;

#define WATCHDOG_SEMF  0x1
#define SYSTIMER_SEMF  0x2

static
void
isr()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  //val_print(AVS_PRINT_INFO, "\n       Received interrupt            ", 0);
  if(wakeup_event == SYSTIMER_SEMF)
      val_timer_disable_system_timer((addr_t)cnt_base_n);
  else if(wakeup_event == WATCHDOG_SEMF)
      val_wd_set_ws0(wd_num, 0);

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  val_gic_end_of_interrupt(intid);
}

uint32_t
wakeup_event_for_semantic_f()
{
  uint32_t ns_timer = 0, ns_wdg = 0;
  wd_num = val_wd_get_info(0, WD_INFO_COUNT);

  if(wd_num == 0){
      timer_num = val_timer_get_info(TIMER_INFO_NUM_PLATFORM_TIMERS, 0);
      if(timer_num == 0)
          return 0;
      else{
          while(timer_num--) {
              if(val_timer_get_info(TIMER_INFO_IS_PLATFORM_TIMER_SECURE, timer_num))
                  continue;
              else{
                  ns_timer++;
                  break;
              }
          }
          if(ns_timer == 0)
              return 0;
          intid = val_timer_get_info(TIMER_INFO_SYS_INTID, timer_num);
          return SYSTIMER_SEMF;
      }
  }
  else{
      while(wd_num--) {
          if(val_wd_get_info(wd_num, WD_INFO_ISSECURE))
              continue;
          else{
              ns_wdg++;
              break;
          }
      }
      if(ns_wdg == 0)
          return 0;
      intid = val_wd_get_info(wd_num, WD_INFO_GSIV);
      return WATCHDOG_SEMF;
  }

}

static
void
payload_target_pe()
{
  uint64_t data1, data2;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  //val_print(AVS_PRINT_INFO, "\n       Print from target PE       ", 0);
  val_get_test_data(index, &data1, &data2);
  val_pe_reg_write(VBAR_EL2, data2);

  val_gic_cpuif_init();
  val_suspend_pe(0,0,0);
  // Set the status to indicate that target PE has resumed execution from sleep mode
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

static
void
payload_dummy(void)
{

}

static
void
payload()
{
  uint64_t timeout = TIMEOUT_SMALL;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t target_pe;
  uint64_t timer_expire_ticks = TIMEOUT_SMALL;

  // Step1: Choose the index of the target PE
  if ((index + 1) >= val_pe_get_num())
      target_pe = index-1;
  else
      target_pe = index+1;

  val_set_status(target_pe, RESULT_PENDING(g_sbsa_level, TEST_NUM));

  // Step2: Get the wakeup event, which is either watchdog signal or system timer
  //        if none of these are present in a platform, skip the test
  wakeup_event = wakeup_event_for_semantic_f();
  if(wakeup_event == 0){
      val_print(AVS_PRINT_WARN, "\n       No Watchdogs and system timers present", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  // Step3: Route the interrupt to target PE and install ISR
  val_gic_route_interrupt_to_pe(intid, val_pe_get_mpid_index(target_pe));
  val_gic_install_isr(intid, isr);

  // Step4: val_execute_on_pe will call payload on target PE and target PE will do following:
  //        1. program VBAR of target PE with the same vale as main PE
  //        2. initialize gic cpu interface for target PE
  //        3. place itself in sleep mode and expect the wakeup_event to wake it up
  //        4. after wake-up it will update the status, which main PE will rely on
  val_execute_on_pe(target_pe, payload_target_pe, val_pe_reg_read(VBAR_EL2));

  // Step5: Program timer/watchdog, which on expiry will generate an interrupt
  //        and wake target PE
  if(wakeup_event == SYSTIMER_SEMF){
      cnt_base_n = val_timer_get_info(TIMER_INFO_SYS_CNT_BASE_N, timer_num);
      val_timer_set_system_timer((addr_t)cnt_base_n, timer_expire_ticks);
  }
  else if(wakeup_event == WATCHDOG_SEMF){
      val_wd_set_ws0(wd_num, timer_expire_ticks);
  }

  // Step6: Wait for target PE to update the status, if a timeout occurs that would mean that
  //        target PE was not able to wakeup
  while((IS_TEST_PASS(val_get_status(target_pe))) && (--timeout));

  if(timeout == 0)
      val_print(AVS_PRINT_INFO, "\n       Target PE was not able to wakeup succesfully from sleep \n       due to watchdog/sytimer interrupt", 0);

  // Step7: Clear the pending/active interrupt if any
  if(1 == val_gic_get_interrupt_state(intid)){
      val_print(AVS_PRINT_INFO, "\n       Pending interrupt was seen for the 1st interrupt", 0);
      if(wakeup_event == SYSTIMER_SEMF)
          val_timer_disable_system_timer((addr_t)cnt_base_n);
      else if(wakeup_event == WATCHDOG_SEMF)
          val_wd_set_ws0(wd_num, 0);

      val_gic_clear_interrupt(intid);
  }
  val_gic_end_of_interrupt(intid);     // trigger END of interrupt for above interrupt

  // Step8: Wait for target PE to switch itself off, if it still doesn't switch off timeout
  //        value should be increased
  timeout = TIMEOUT_SMALL;
  while(--timeout);

  // Step9: Generate timer interrupt again, when target PE is off and make sure it doesn't wakeup
  val_gic_route_interrupt_to_pe(intid, val_pe_get_mpid_index(target_pe));
  val_gic_install_isr(intid, isr);
  timer_expire_ticks = TIMEOUT_SMALL;

  if(wakeup_event == SYSTIMER_SEMF)
      val_timer_set_system_timer((addr_t)cnt_base_n, timer_expire_ticks);
  else if(wakeup_event == WATCHDOG_SEMF)
      val_wd_set_ws0(wd_num, timer_expire_ticks);

  val_print(AVS_PRINT_INFO, "\n       Interrupt generating sequence triggered", 0);

  // Step10: wait for interrupt to become active or pendind for a timeout duration
  timeout = TIMEOUT_MEDIUM;
  while ((--timeout) && (0 == val_gic_get_interrupt_state(intid)));

  if(timeout == 0)
      val_print(AVS_PRINT_INFO, "\n       No pending interrupt was seen for the 2nd interrupt", 0);
  else{
      val_print(AVS_PRINT_INFO, "\n       Pending interrupt was seen for the 2nd interrupt", 0);
      if(wakeup_event == SYSTIMER_SEMF)
          val_timer_disable_system_timer((addr_t)cnt_base_n);
      else if(wakeup_event == WATCHDOG_SEMF)
          val_wd_set_ws0(wd_num, 0);

      val_gic_clear_interrupt(intid);
      val_gic_end_of_interrupt(intid);     // trigger END of interrupt for above interrupt
  }

  // Step11: If event triggered woke up the target PE when it was off, then making PSCI call
  //         to switch it ON again would throw an error response, based on which the test is
  //         passed ot failed.
  val_execute_on_pe(target_pe, payload_dummy, 0);

  if(IS_TEST_FAIL(val_get_status(target_pe)) || IS_RESULT_PENDING(val_get_status(target_pe)))
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
  else
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

  return;
}


uint32_t
u002_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL, status_test = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor, which will start and trigger interrupt to
               // target PE.

  status_test = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status_test != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
