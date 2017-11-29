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

#include "val/include/sbsa_avs_wakeup.h"

#define TEST_DESC  "       TEST Wakeup from Power Semantic B  \n"

#define TEST_NUM   (AVS_WAKEUP_TEST_NUM_BASE + 1)
#define TEST_DESC1 "Wake from EL0 PHY Timer Interrupt "
#define TEST_NUM2  (AVS_WAKEUP_TEST_NUM_BASE + 2)
#define TEST_DESC2 "Wake from EL0 VIRT Timer Interrupt"
#define TEST_NUM3  (AVS_WAKEUP_TEST_NUM_BASE + 3)
#define TEST_DESC3 "Wake from EL2 PHY Timer Interrupt "
#define TEST_NUM4  (AVS_WAKEUP_TEST_NUM_BASE + 4)
#define TEST_DESC4 "Wake from Watchdog WS0 Interrupt  "
#define TEST_NUM5  (AVS_WAKEUP_TEST_NUM_BASE + 5)
#define TEST_DESC5 "Wake from System Timer Interrupt  "

static uint32_t intid;
uint64_t timer_num;

static
void
isr_failsafe()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  val_timer_set_phy_el1(0);
  val_print(AVS_PRINT_ERR, "\n       Received Failsafe interrupt      ", 0);
  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
  intid = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID, 0);
  val_gic_end_of_interrupt(intid);
}

static
void
isr1()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  val_timer_set_phy_el1(0);
  val_print(AVS_PRINT_INFO, "\n       Received EL1 PHY interrupt       ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  intid = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID, 0);
  val_gic_end_of_interrupt(intid);
}


static
void
isr2()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  /* We received our interrupt, so disable timer from generating further interrupts */
  val_timer_set_vir_el1(0);
  val_print(AVS_PRINT_INFO, "\n       Received EL1 VIRT interrupt      ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM2, 01));
  intid = val_timer_get_info(TIMER_INFO_VIR_EL1_INTID, 0);
  val_gic_end_of_interrupt(intid);
}

static
void
isr3()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  /* We received our interrupt, so disable timer from generating further interrupts */
  val_timer_set_phy_el2(0);
  val_print(AVS_PRINT_INFO, "\n       Received EL2 Physical interrupt  ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM3, 01));
  intid = val_timer_get_info(TIMER_INFO_PHY_EL2_INTID, 0);
  val_gic_end_of_interrupt(intid);
}

static
void
isr4()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  val_wd_set_ws0(timer_num, 0);
  val_print(AVS_PRINT_INFO, "\n       Received WS0 interrupt           ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM4, 01));
  intid = val_wd_get_info(timer_num, WD_INFO_GSIV);
  val_gic_end_of_interrupt(intid);
}

static
void
isr5()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t cnt_base_n = val_timer_get_info(TIMER_INFO_SYS_CNT_BASE_N, timer_num);
  val_timer_disable_system_timer((addr_t)cnt_base_n);
  val_print(AVS_PRINT_INFO, "\n       Received Sys timer interrupt   ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM5, 01));
  intid = val_timer_get_info(TIMER_INFO_SYS_INTID, timer_num);
  val_gic_end_of_interrupt(intid);
}


void
wakeup_set_failsafe()
{
  uint64_t timer_expire_val = TIMEOUT_LARGE;

  intid = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID, 0);
  val_gic_install_isr(intid, isr_failsafe);
  val_timer_set_phy_el1(timer_expire_val);
}

void
wakeup_clear_failsafe()
{
  val_timer_set_phy_el1(0);

}

static
void
payload1()
{
  uint64_t timer_expire_val = TIMEOUT_SMALL;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));

  intid = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID, 0);
  val_gic_install_isr(intid, isr1);
  val_timer_set_phy_el1(timer_expire_val);
  val_power_enter_semantic(SBSA_POWER_SEM_B);
  return;
}

static
void
payload2()
{
  uint64_t timer_expire_val = TIMEOUT_SMALL;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM2, 01));
  intid = val_timer_get_info(TIMER_INFO_VIR_EL1_INTID, 0);
  val_gic_install_isr(intid, isr2);
  wakeup_set_failsafe();
  val_timer_set_vir_el1(timer_expire_val);
  val_power_enter_semantic(SBSA_POWER_SEM_B);
  wakeup_clear_failsafe();
  return;
}

static
void
payload3()
{
  uint64_t timer_expire_val = TIMEOUT_SMALL;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM3, 01));
  intid = val_timer_get_info(TIMER_INFO_PHY_EL2_INTID, 0);
  val_gic_install_isr(intid, isr3);
  wakeup_set_failsafe();
  val_timer_set_phy_el2(timer_expire_val);
  val_power_enter_semantic(SBSA_POWER_SEM_B);
  wakeup_clear_failsafe();
  return;
}

static
void
payload4()
{
  uint32_t status, ns_wdg = 0;
  uint64_t timer_expire_val = TIMEOUT_SMALL;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  timer_num = val_wd_get_info(0, WD_INFO_COUNT);
  if(!timer_num){
      val_print(AVS_PRINT_WARN, "\n       No watchdog implemented   ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM4, 01));
      return;
  }

  while(timer_num--) {
      if(val_wd_get_info(timer_num, WD_INFO_ISSECURE))
          continue;

      ns_wdg++;
      intid = val_wd_get_info(timer_num, WD_INFO_GSIV);
      status = val_gic_install_isr(intid, isr4);
      if (status == 0) {
          wakeup_set_failsafe();
          val_wd_set_ws0(timer_num, timer_expire_val);
          val_power_enter_semantic(SBSA_POWER_SEM_B);
          wakeup_clear_failsafe();
      } else {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM4, 01));
      }
  }

  if(!ns_wdg){
      val_print(AVS_PRINT_WARN, "\n       No non-secure watchdog implemented   ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM4, 02));
      return;
  }

}

static
void
payload5()
{
  uint64_t cnt_base_n;
  uint64_t timer_expire_val = TIMEOUT_SMALL;
  uint32_t status, ns_timer = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  timer_num = val_timer_get_info(TIMER_INFO_NUM_PLATFORM_TIMERS, 0);
  if(!timer_num){
      val_print(AVS_PRINT_WARN, "\n       No system timers implemented      ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM5, 01));
      return;
  }

  while(timer_num--) {

      if(val_timer_get_info(TIMER_INFO_IS_PLATFORM_TIMER_SECURE, timer_num))
          continue;

      ns_timer++;
      status = val_timer_get_info(TIMER_INFO_SYS_TIMER_STATUS, 0);
      if(status != AVS_STATUS_PASS){
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM5, 02));
          return;
      }

      //Read CNTACR to determine whether access permission from NS state is permitted
      status = val_timer_skip_if_cntbase_access_not_allowed(timer_num);
      if(status == AVS_STATUS_SKIP){
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM5, 03));
          return;
      }

      cnt_base_n = val_timer_get_info(TIMER_INFO_SYS_CNT_BASE_N, timer_num);
      if(cnt_base_n == 0){
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM5, 04));
          return;
      }

      intid = val_timer_get_info(TIMER_INFO_SYS_INTID, timer_num);
      status = val_gic_install_isr(intid, isr5);

      if(status == 0) {
          wakeup_set_failsafe();
          /* enable System timer */
          val_timer_set_system_timer((addr_t)cnt_base_n, timer_expire_val);
          val_power_enter_semantic(SBSA_POWER_SEM_B);
          wakeup_clear_failsafe();
      } else{
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM5, 01));
          return;
      }
  }

  if(!ns_timer){
      val_print(AVS_PRINT_WARN, "\n       No non-secure systimer implemented   ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM4, 03));
      return;
  }
}

uint32_t
u001_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL, status_test = AVS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  val_print(AVS_PRINT_TEST, TEST_DESC, 0);

  status_test = val_initialize_test(TEST_NUM, TEST_DESC1, num_pe, g_sbsa_level);
  if (status_test != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload1, 0);
  status = val_check_for_error(TEST_NUM, num_pe);

  status_test = val_initialize_test(TEST_NUM2, TEST_DESC2, num_pe, g_sbsa_level);
  if (status_test != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM2, num_pe, payload2, 0);
  status |= val_check_for_error(TEST_NUM2, num_pe);

  status_test = val_initialize_test(TEST_NUM3, TEST_DESC3, num_pe, g_sbsa_level);
  if (status_test != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM3, num_pe, payload3, 0);
  status |= val_check_for_error(TEST_NUM3, num_pe);

  status_test = val_initialize_test(TEST_NUM4, TEST_DESC4, num_pe, g_sbsa_level);
  if (status_test != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM4, num_pe, payload4, 0);
  status |= val_check_for_error(TEST_NUM4, num_pe);

  status_test = val_initialize_test(TEST_NUM5, TEST_DESC5, num_pe, g_sbsa_level);
  if (status_test != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM5, num_pe, payload5, 0);
  status |= val_check_for_error(TEST_NUM5, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
