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
#include "include/sbsa_avs_common.h"

#include "include/sbsa_avs_smmu.h"

SMMU_INFO_TABLE *g_smmu_info_table;

uint64_t
pal_get_iort_ptr(void);

/**
  @brief  This API reads 32-bit data from a register of an SMMU controller
          specifed by index
  @param offset   32-bit register offset
  @param index    when multiple SMMU controllers are present in the system.
                  '0' based index to uniquely identify them

  @return  32-bit data value
**/
uint32_t
val_smmu_read_cfg(uint32_t offset, uint32_t index)
{

  uint64_t ctrl_base = val_smmu_get_info(SMMU_CTRL_BASE, index);

  if (ctrl_base == 0)
      return 0;

  return val_mmio_read(ctrl_base + offset);
}

/**
  @brief   This API executes all the SMMU tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_smmu_create_info_table()
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_smmu_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status;

  if (val_smmu_get_info(SMMU_NUM_CTRL, 0) == 0) {
      val_print(AVS_PRINT_WARN, "      SMMU controller not present. Skipping tests..\n", 0);
      return AVS_STATUS_SKIP;
  }

  if (g_skip_test_num == AVS_SMMU_TEST_NUM_BASE) {
      val_print(AVS_PRINT_TEST, "      USER Override - Skipping all SMMU tests \n", 0);
      return AVS_STATUS_SKIP;
  }


  status = i001_entry(num_pe);
  status |= i002_entry(num_pe);
  status |= i003_entry(num_pe);

  if (status != AVS_STATUS_PASS) {
      val_print(AVS_PRINT_WARN, "\n     *** Skipping remaining IO-VIRT tests *** \n", 0);
      return status;
  }

  status |= i004_entry(num_pe);

  if (status != AVS_STATUS_PASS) {
      val_print(AVS_PRINT_ERR, "\n      One or more SMMU tests have failed...  \n", status);
  }

  return status;
}

/**
  @brief   This API will call PAL layer to fill in the SMMU information
           into the g_smmu_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   smmu_info_table  pre-allocated memory pointer for smmu_info
  @return  Error if Input param is NULL
**/
void
val_smmu_create_info_table(uint64_t *smmu_info_table)
{

  if (smmu_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "\n   Input for Create Info table cannot be NULL \n", 0);
      return;
  }

  g_smmu_info_table = (SMMU_INFO_TABLE *)smmu_info_table;

  pal_smmu_create_info_table(g_smmu_info_table);
  
  val_print(AVS_PRINT_TEST, " SMMU_INFO: Number of SMMU CTRL       :    %x \n", val_smmu_get_info(SMMU_NUM_CTRL, 0));
}

void
val_smmu_free_info_table()
{
  pal_mem_free((void *)g_smmu_info_table);
}

/**
  @brief   This API is a single point of entry to retrieve 
           information stored in the SMMU Info table
           1. Caller       -  Test Suite
           2. Prerequisite -  val_smmu_create_info_table
  @param   type   the type of information being requested
  @return  64-bit data
**/
uint64_t
val_smmu_get_info(SMMU_INFO_e type, uint32_t index)
{

  if (g_smmu_info_table == NULL)
  {
      val_print(AVS_PRINT_ERR, "GET_SMMU_INFO: SMMU info table is not created \n", 0);
      return 0;
  }
  if (index > g_smmu_info_table->smmu_num_ctrl)
  {
      val_print(AVS_PRINT_ERR, "GET_SMMU_INFO: Index (%d) is greater than num of SMMU \n", index);
      return 0;
  }

  switch (type)
  {
      case SMMU_NUM_CTRL:
          return g_smmu_info_table->smmu_num_ctrl;

      case SMMU_CTRL_ARCH_MAJOR_REV:
          return g_smmu_info_table->smmu_block[index].arch_major_rev;

      case SMMU_CTRL_BASE:
          return g_smmu_info_table->smmu_block[index].base;

      default:
          val_print(AVS_PRINT_ERR, "This SMMU info option not supported %d \n", type);
          break;
  }

  return 0;
}

