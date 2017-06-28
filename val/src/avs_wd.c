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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_timer_support.h"
#include "include/sbsa_avs_wd.h"
#include "include/sbsa_avs_common.h"


WD_INFO_TABLE  *g_wd_info_table;

/**
  @brief   This API executes all the Watchdog tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_wd_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_wd_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status, i;

  for (i=0 ; i<MAX_TEST_SKIP_NUM ; i++){
      if (g_skip_test_num[i] == AVS_WD_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "      USER Override - Skipping all Watchdog tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }


  status = w001_entry(num_pe);
  if (status != AVS_STATUS_PASS) {
    val_print(AVS_PRINT_WARN, "\n      Watchdog not present. Skip remaining tests.. \n", 0);
    return AVS_STATUS_SKIP;
  }
  status |= w002_entry(num_pe);

  if (status != 0)
    val_print(AVS_PRINT_TEST, "\n      ***One or more tests have failed... *** \n", 0);
  else
    val_print(AVS_PRINT_TEST, "\n      All Watchdog tests passed!! \n", 0);

  return status;
}

/**
  @brief   This API is a single point of entry to retrieve
           information stored in the WD Info table
           1. Caller       -  Test Suite
           2. Prerequisite -  val_wd_create_info_table
  @param   type   the type of information being requested
  @return  64-bit data
**/
uint64_t
val_wd_get_info(uint32_t index, WD_INFO_TYPE_e info_type)
{

  if (g_wd_info_table == NULL)
      return 0;

  switch (info_type) {
    case WD_INFO_COUNT:
      return g_wd_info_table->header.num_wd;
    case WD_INFO_CTRL_BASE:
      return g_wd_info_table->wd_info[index].wd_ctrl_base;
    case WD_INFO_REFRESH_BASE:
      return g_wd_info_table->wd_info[index].wd_refresh_base;
    case WD_INFO_GSIV:
      return g_wd_info_table->wd_info[index].wd_gsiv;
    case WD_INFO_ISSECURE:
      return ((g_wd_info_table->wd_info[index].wd_flags >> 2) & 1);
    default:
      return 0;
  }
}

/**
  @brief   This API will call PAL layer to fill in the Watchdog information
           into the address pointed by g_wd_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   wd_info_table  pre-allocated memory pointer for smmu_info
  @return  Error if Input param is NULL
**/
void
val_wd_create_info_table(uint64_t *wd_info_table)
{

  if (wd_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "Input for Create Info table cannot be NULL \n", 0);
      return;
  }

  g_wd_info_table = (WD_INFO_TABLE *)wd_info_table;

  pal_wd_create_info_table(g_wd_info_table);

  val_print(AVS_PRINT_TEST, " WATCHDOG_INFO: Number of Watchdogs   : %4d \n", val_wd_get_info(0, WD_INFO_COUNT));
}

/**
  @brief  Free the memory allocated for the Watchdog information table
**/
void
val_wd_free_info_table()
{
  pal_mem_free((void *)g_wd_info_table);
}

/**
  @brief   This API Enables watchdog by writing to Control Base register
  @param   index   - identifies the watchdog instance to enable
  @return  None
 **/
void
val_wd_enable(uint32_t index)
{

  val_mmio_write((g_wd_info_table->wd_info[index].wd_ctrl_base + 0), 1);

}

/**
  @brief   This API disables watchdog by writing to Control Base register
  @param   index   - identifies the watchdog instance to enable
  @return  None
 **/
void
val_wd_disable(uint32_t index)
{

  val_mmio_write((g_wd_info_table->wd_info[index].wd_ctrl_base + 0), 0);

}

/**
  @brief   This API arms the watchdog by writing to Control Base register
  @param   index   - identifies the watchdog instance to program
  @param   timeout - ticks to generation of ws0 interrupt
  @return  None
 **/
void
val_wd_set_ws0(uint32_t index, uint32_t timeout)
{

  if (timeout == 0) {
      val_wd_disable(index);
      return;
  }

  val_mmio_write((g_wd_info_table->wd_info[index].wd_ctrl_base + 8), timeout);
  val_wd_enable(index);

}

