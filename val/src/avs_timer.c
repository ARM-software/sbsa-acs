/** @file
 * Copyright (c) 2016-2023, Arm Limited or its affiliates. All rights reserved.
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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_pe.h"
#include "include/sbsa_avs_timer_support.h"
#include "include/sbsa_avs_timer.h"
#include "include/sbsa_avs_common.h"
#include "include/sbsa_avs_mmu.h"

TIMER_INFO_TABLE  *g_timer_info_table;

/**
  @brief   This API executes all the timer tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_timer_create_info_table()
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_timer_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = AVS_STATUS_SKIP, i;
  uint32_t skip_module;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == AVS_TIMER_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "      USER Override - Skipping all Timer tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  skip_module = val_check_skip_module(AVS_TIMER_TEST_NUM_BASE);
  if (skip_module) {
      val_print(AVS_PRINT_TEST, "\n USER Override - Skipping all Timer tests \n", 0);
      return AVS_STATUS_SKIP;
  }

  val_print_test_start("Timer");

  return status;
}

/**
  @brief   This API is the single entry point to return all Timer related information
           1. Caller       -  Test Suite
           2. Prerequisite -  val_timer_create_info_table
  @param   info_type  - Type of the information to be returned

  @return  64-bit data pertaining to the requested input type
**/

uint64_t
val_timer_get_info(TIMER_INFO_e info_type, uint64_t instance)
{

  uint32_t block_num, block_index;

  if (g_timer_info_table == NULL)
      return 0;

  switch (info_type) {
      case TIMER_INFO_CNTFREQ:
          return ArmArchTimerReadReg(CntFrq);
      case TIMER_INFO_PHY_EL1_INTID:
          return g_timer_info_table->header.ns_el1_timer_gsiv;
      case TIMER_INFO_VIR_EL1_INTID:
          return g_timer_info_table->header.virtual_timer_gsiv;
      case TIMER_INFO_PHY_EL2_INTID:
          return g_timer_info_table->header.el2_timer_gsiv;
      case TIMER_INFO_VIR_EL2_INTID:
          return g_timer_info_table->header.el2_virt_timer_gsiv;
      case TIMER_INFO_NUM_PLATFORM_TIMERS:
          return g_timer_info_table->header.num_platform_timer;
      case TIMER_INFO_IS_PLATFORM_TIMER_SECURE:
          val_platform_timer_get_entry_index (instance, &block_num, &block_index);
          if (block_num != 0xFFFF)
              return ((g_timer_info_table->gt_info[block_num].flags[block_index] >> 16) & 1);
      case TIMER_INFO_SYS_CNTL_BASE:
          val_platform_timer_get_entry_index (instance, &block_num, &block_index);
          if (block_num != 0xFFFF)
              return g_timer_info_table->gt_info[block_num].block_cntl_base;
      case TIMER_INFO_SYS_CNT_BASE_N:
          val_platform_timer_get_entry_index (instance, &block_num, &block_index);
          if (block_num != 0xFFFF)
              return g_timer_info_table->gt_info[block_num].GtCntBase[block_index];
      case TIMER_INFO_FRAME_NUM:
          val_platform_timer_get_entry_index (instance, &block_num, &block_index);
          if (block_num != 0xFFFF)
              return g_timer_info_table->gt_info[block_num].frame_num[block_index];
      case TIMER_INFO_SYS_INTID:
          val_platform_timer_get_entry_index (instance, &block_num, &block_index);
          if (block_num != 0xFFFF)
            return g_timer_info_table->gt_info[block_num].gsiv[block_index];
      case TIMER_INFO_PHY_EL1_FLAGS:
          return g_timer_info_table->header.ns_el1_timer_flag;
      case TIMER_INFO_VIR_EL1_FLAGS:
          return g_timer_info_table->header.virtual_timer_flag;
      case TIMER_INFO_PHY_EL2_FLAGS:
          return g_timer_info_table->header.el2_timer_flag;
      case TIMER_INFO_SYS_TIMER_STATUS:
          return g_timer_info_table->header.sys_timer_status;
    default:
      return 0;
  }
}

void
val_platform_timer_get_entry_index(uint64_t instance, uint32_t *block, uint32_t *index)
{
  if(instance > g_timer_info_table->header.num_platform_timer){
      *block = 0xFFFF;
      return;
  }

  *block = 0;
  *index = instance;
  while (instance > g_timer_info_table->gt_info[*block].timer_count){
      instance = instance - g_timer_info_table->gt_info[*block].timer_count;
      *index   = instance;
      *block   = *block + 1;
  }
}

/**
  @brief   This API enables the Architecture timer whose register is given as the input parameter.
           1. Caller       -  VAL
           2. Prerequisite -  None
  @param   reg  - system register of the ELx Arch timer.

  @return  None
**/
void
ArmGenericTimerEnableTimer (
  ARM_ARCH_TIMER_REGS reg
 )
{
  uint64_t timer_ctrl_reg;

  timer_ctrl_reg = ArmArchTimerReadReg (reg);
  timer_ctrl_reg &= (~ARM_ARCH_TIMER_IMASK);
  timer_ctrl_reg |= ARM_ARCH_TIMER_ENABLE;
  ArmArchTimerWriteReg (reg, &timer_ctrl_reg);
}

/**
  @brief   This API disables the Architecture timer whose register is given as the input parameter.
           1. Caller       -  VAL
           2. Prerequisite -  None
  @param   reg  - system register of the ELx Arch timer.

  @return  None
**/
void
ArmGenericTimerDisableTimer (
  ARM_ARCH_TIMER_REGS reg
 )
{
  uint64_t timer_ctrl_reg;

  timer_ctrl_reg = ArmArchTimerReadReg (reg);
  timer_ctrl_reg |= ARM_ARCH_TIMER_IMASK;
  timer_ctrl_reg &= ~ARM_ARCH_TIMER_ENABLE;
  ArmArchTimerWriteReg (reg, &timer_ctrl_reg);
}

/**
  @brief   This API programs the el1 phy timer with the input timeout value.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   timeout - clock ticks after which an interrupt is generated.

  @return  None
**/
void
val_timer_set_phy_el1(uint64_t timeout)
{

  if (timeout != 0) {
    ArmGenericTimerDisableTimer(CntpCtl);
    ArmArchTimerWriteReg(CntpTval, &timeout);
    ArmGenericTimerEnableTimer(CntpCtl);
  } else {
    ArmGenericTimerDisableTimer(CntpCtl);
 }
}

/**
  @brief   This API programs the el1 Virtual timer with the input timeout value.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   timeout - clock ticks after which an interrupt is generated.

  @return  None
**/
void
val_timer_set_vir_el1(uint64_t timeout)
{

  if (timeout != 0) {
    ArmGenericTimerDisableTimer(CntvCtl);
    ArmArchTimerWriteReg(CntvTval, &timeout);
    ArmGenericTimerEnableTimer(CntvCtl);
  } else {
    ArmGenericTimerDisableTimer(CntvCtl);
 }

}

/**
  @brief   This API programs the el2 phy timer with the input timeout value.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   timeout - clock ticks after which an interrupt is generated.

  @return  None
**/
void
val_timer_set_phy_el2(uint64_t timeout)
{

  if (timeout != 0) {
    ArmGenericTimerDisableTimer(CnthpCtl);
    ArmArchTimerWriteReg(CnthpTval, &timeout);
    ArmGenericTimerEnableTimer(CnthpCtl);
  } else {
    ArmGenericTimerDisableTimer(CnthpCtl);
 }
}

/**
  @brief   This API programs the el2 Virt timer with the input timeout value.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   timeout - clock ticks after which an interrupt is generated.

  @return  None
**/
void
val_timer_set_vir_el2(uint64_t timeout)
{

  if (timeout != 0) {
    ArmGenericTimerDisableTimer(CnthvCtl);
    ArmArchTimerWriteReg(CnthvTval, &timeout);
    ArmGenericTimerEnableTimer(CnthvCtl);
  } else {
    ArmGenericTimerDisableTimer(CnthvCtl);
 }

}

/**
  @brief   This API will call PAL layer to fill in the Timer information
           into the g_timer_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   timer_info_table  pre-allocated memory pointer for timer_info
  @return  Error if Input param is NULL
**/
void
val_timer_create_info_table(uint64_t *timer_info_table)
{

  uint64_t timer_num;
  uint64_t gt_entry;
  uint64_t timer_entry;

  if (timer_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "Input for Create Info table cannot be NULL \n", 0);
      return;
  }

  g_timer_info_table = (TIMER_INFO_TABLE *)timer_info_table;

  pal_timer_create_info_table(g_timer_info_table);

  /* UEFI or other EL1 software may have enabled the EL1 physical/virtual timer.
     Disable the timers to prevent interrupts at un-expected times */

  val_timer_set_phy_el1(0);
  val_timer_set_vir_el1(0);

  val_print(AVS_PRINT_TEST, " TIMER_INFO: Number of system timers  : %4d \n",
                                                  g_timer_info_table->header.num_platform_timer);

  timer_num = val_timer_get_info(TIMER_INFO_NUM_PLATFORM_TIMERS, 0);

  while (timer_num) {
      --timer_num;

      if (val_timer_get_info(TIMER_INFO_IS_PLATFORM_TIMER_SECURE, timer_num))
          continue;    //Skip Secure Timer

      gt_entry = val_timer_get_info(TIMER_INFO_SYS_CNTL_BASE, timer_num);
      timer_entry = val_timer_get_info(TIMER_INFO_SYS_CNT_BASE_N, timer_num);

      val_print(AVS_PRINT_DEBUG, "   Add entry %lx entry in memmap", gt_entry);
      if (val_mmu_update_entry(gt_entry, 0x10000))
          val_print(AVS_PRINT_WARN, "\n   Adding %lx entry failed", gt_entry);

      val_print(AVS_PRINT_DEBUG, "\n   Add entry %lx entry in memmap", timer_entry);
      if (val_mmu_update_entry(timer_entry, 0x10000))
          val_print(AVS_PRINT_WARN, "\n   Adding %lx entry failed", timer_entry);
  }
}

/**
  @brief  Free the memory allocated for the Timer Info table
**/
void
val_timer_free_info_table()
{
  pal_mem_free((void *)g_timer_info_table);
}

/**
  @brief  This API will program and start the counter
**/
void
val_timer_set_system_timer(addr_t cnt_base_n, uint32_t timeout)
{
  /* Start the System timer */
  val_mmio_write(cnt_base_n + 0x28, timeout);

  /* enable System timer */
  val_mmio_write(cnt_base_n + 0x2C, 1);

}

/**
  @brief  This API will stop the counter
**/
void
val_timer_disable_system_timer(addr_t cnt_base_n)
{

  /* stop System timer */
  val_mmio_write(cnt_base_n + 0x2C, 0);
}

/**
  @brief  This API will read CNTACR (from CNTCTLBase) to determine whether
          access permission from NS state is permitted
**/
uint32_t
val_timer_skip_if_cntbase_access_not_allowed(uint64_t index)
{
  uint64_t cnt_ctl_base;
  uint32_t data;
  uint32_t frame_num = 0;

  cnt_ctl_base = val_timer_get_info(TIMER_INFO_SYS_CNTL_BASE, index);
  frame_num = val_timer_get_info(TIMER_INFO_FRAME_NUM, index);

  if(cnt_ctl_base){
      data = val_mmio_read(cnt_ctl_base + 0x40 + frame_num * 4);
      if((data & 0x1) == 0x1)
          return 0;
      else{
          data |= 0x1;
          val_mmio_write(cnt_ctl_base + 0x40 + frame_num *4, data);
          data = val_mmio_read(cnt_ctl_base + 0x40 + frame_num * 4);
          if((data & 0x1) == 1)
              return 0;
          else
              return AVS_STATUS_SKIP;
      }
  }
  else
      return AVS_STATUS_SKIP;

}
