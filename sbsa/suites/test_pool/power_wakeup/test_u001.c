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


static
void
isr_failsafe()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  val_timer_set_phy_el1(0);
  val_print(AVS_PRINT_INFO, "\n       Received Failsafe interrupt      ", 0);
  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
}

static
void
isr1()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  val_timer_set_phy_el1(0);
  val_print(AVS_PRINT_INFO, "\n       Received EL1 PHY interrupt       ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
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
}

static
void
isr4()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
//  uint64_t wd_num = val_get_wd_info(0, WD_INFO_COUNT);

  /* We received our interrupt, we don't know which WD instance generated 
     the interrupt, so just ahead and disable all */
//  while(wd_num)
//    val_set_wd_ws0(wd_num, 0);

  val_wd_set_ws0(0, 0);
  val_print(AVS_PRINT_INFO, "\n       Received WS0 interrupt           ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM4, 01));
}


void
wakeup_set_failsafe()
{
  uint32_t intid;
  uint64_t timer_expire_val = 900000;

  intid = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID);
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
  uint32_t intid;
  uint64_t timer_expire_val = 100000;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));

  intid = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID);
  val_gic_install_isr(intid, isr1);
  val_timer_set_phy_el1(timer_expire_val);
  val_power_enter_semantic(SBSA_POWER_SEM_B);
  return;
}

static
void
payload2()
{
  uint32_t intid;
  uint64_t timer_expire_val = 100000;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM2, 01));
  intid = val_timer_get_info(TIMER_INFO_VIR_EL1_INTID);
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
  uint32_t intid;
  uint64_t timer_expire_val = 100000;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM3, 01));
  intid = val_timer_get_info(TIMER_INFO_PHY_EL2_INTID);
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
  uint32_t intid;
  uint32_t status;
  uint64_t timer_expire_val = 100000;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM4, 01));

  if (val_wd_get_info(0, WD_INFO_COUNT)) { 
      intid = val_wd_get_info(0, WD_INFO_GSIV);
      status = val_gic_install_isr(intid, isr4);
      if (status == 0) {
          wakeup_set_failsafe();
          val_wd_set_ws0(0, timer_expire_val);
          val_power_enter_semantic(SBSA_POWER_SEM_B);
          wakeup_clear_failsafe();
      } else {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM4, 01));
      }
  } else {
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM4, 01));
  }

}

uint32_t
u001_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  val_print(AVS_PRINT_TEST, TEST_DESC, 0);

  status = val_initialize_test(TEST_NUM, TEST_DESC1, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload1, 0);
  status = val_check_for_error(TEST_NUM, num_pe);

  status = val_initialize_test(TEST_NUM2, TEST_DESC2, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM2, num_pe, payload2, 0);
  status |= val_check_for_error(TEST_NUM2, num_pe);

  status = val_initialize_test(TEST_NUM3, TEST_DESC3, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM3, num_pe, payload3, 0);
  status |= val_check_for_error(TEST_NUM3, num_pe);

  status = val_initialize_test(TEST_NUM4, TEST_DESC4, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM4, num_pe, payload4, 0);
  status |= val_check_for_error(TEST_NUM4, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
