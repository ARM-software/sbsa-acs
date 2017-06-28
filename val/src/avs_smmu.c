/** @file
 * Copyright (c) 2017, ARM Limited or its affiliates. All rights reserved.

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
#include "include/sbsa_avs_iovirt.h"

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

#ifndef TARGET_LINUX

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
  uint32_t status, i;

  if (val_smmu_get_info(SMMU_NUM_CTRL, 0) == 0) {
      val_print(AVS_PRINT_WARN, "      SMMU controller not present. Skipping tests..\n", 0);
      return AVS_STATUS_SKIP;
  }

  for (i=0 ; i<MAX_TEST_SKIP_NUM ; i++){
      if (g_skip_test_num[i] == AVS_SMMU_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "      USER Override - Skipping all SMMU tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  status = i001_entry(num_pe);
  status |= i002_entry(num_pe);
  status |= i003_entry(num_pe);

  if (status != AVS_STATUS_PASS) {
      val_print(AVS_PRINT_WARN, "\n     *** Skipping remaining IO-VIRT tests *** \n", 0);
      return status;
  }

  status |= i004_entry(num_pe);
  status |= i005_entry(num_pe);
  status |= i006_entry(num_pe);

  if (status != AVS_STATUS_PASS) {
      val_print(AVS_PRINT_ERR, "\n      One or more SMMU tests have failed...  \n", status);
  }

  return status;
}
#endif


uint64_t
val_smmu_get_info(SMMU_INFO_e type, uint32_t index)
{
  return val_iovirt_get_smmu_info(type, index);
}


uint32_t
val_smmu_start_monitor_dev(uint32_t ctrl_index)
{
  void *ap = NULL;

  ap = (void *)val_dma_get_info(DMA_PORT_INFO, ctrl_index);
  if (ap == NULL) {
      val_print(AVS_PRINT_ERR, "Invalid Controller index %d \n", ctrl_index);
      return AVS_STATUS_ERR;
  }

  pal_smmu_device_start_monitor_iova(ap);

  return 0;
}

uint32_t
val_smmu_stop_monitor_dev(uint32_t ctrl_index)
{
  void *ap = NULL;

  ap = (void *)val_dma_get_info(DMA_PORT_INFO, ctrl_index);
  if (ap == NULL) {
      val_print(AVS_PRINT_ERR, "Invalid Controller index %d \n", ctrl_index);
      return AVS_STATUS_ERR;
  }

  pal_smmu_device_stop_monitor_iova(ap);

  return 0;
}


/**
  @brief   Check if input address is within the IOVA translation range for the device
           1. Caller       -  Test suite
           2. Prerequisite -  val_smmu_create_info_table()
  @param   ctrl_index - The device whose IO Translation range needs to be checked
  @param   dma_addr   - The input address to be checked
  @return  Success if the input address is found in the range
**/
uint32_t
val_smmu_check_device_iova(uint32_t ctrl_index, addr_t dma_addr)
{
  void *ap = NULL;
  uint32_t status;

  ap = (void *)val_dma_get_info(DMA_PORT_INFO, ctrl_index);
  if (ap == NULL) {
      val_print(AVS_PRINT_ERR, "Invalid Controller index %d \n", ctrl_index);
      return AVS_STATUS_ERR;
  }
  val_print(AVS_PRINT_DEBUG, "Input dma addr = %lx \n", dma_addr);

  status = pal_smmu_check_device_iova(ap, dma_addr);

  return status;
}


uint64_t
val_smmu_ops(SMMU_OPS_e ops, uint32_t smmu_index, void *param1, void *param2)
{

  switch(ops)
  {
      case SMMU_START_MONITOR_DEV:
          return val_smmu_start_monitor_dev(*(uint32_t *)param1);

      case SMMU_STOP_MONITOR_DEV:
          return val_smmu_stop_monitor_dev(*(uint32_t *)param1);

      case SMMU_CHECK_DEVICE_IOVA:
          return val_smmu_check_device_iova(*(uint32_t *)param1, *(addr_t *)param2);
          break;

      default:
          break;
  }
//  pal_smmu_ops(ops, index, param1, param2);
  return 0;

}

uint32_t
val_smmu_max_pasids(uint64_t smmu_base)
{
  return pal_smmu_max_pasids(smmu_base);
}
