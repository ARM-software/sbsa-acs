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
  uint32_t status = AVS_STATUS_PASS, i;
  uint32_t num_smmu;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == AVS_SMMU_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "      USER Override - Skipping all SMMU tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(AVS_SMMU_TEST_NUM_BASE);
  if (status) {
      val_print(AVS_PRINT_TEST, "\n USER Override - Skipping all SMMU tests \n", 0);
      return AVS_STATUS_SKIP;
  }

  num_smmu = val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0);
  if (num_smmu == 0) {
    val_print(AVS_PRINT_WARN, "\n     No SMMU Controller Found, Skipping SMMU tests...\n", 0);
    return AVS_STATUS_SKIP;
  }

  val_print_test_start("SMMU");
  g_curr_module = 1 << SMMU_MODULE;

#ifndef TARGET_LINUX
  if (g_sbsa_level > 3) {
      status = i001_entry(num_pe) ;
  }

  if (g_sbsa_level > 4) {
      status |= i002_entry(num_pe);
      status |= i003_entry(num_pe);
      status |= i004_entry(num_pe);
      status |= i005_entry(num_pe);
  }

  if (g_sbsa_level > 5) {
      status |= i006_entry(num_pe);
      status |= i007_entry(num_pe);
      status |= i008_entry(num_pe);
      status |= i009_entry(num_pe);
      status |= i010_entry(num_pe);
      status |= i011_entry(num_pe);
      status |= i012_entry(num_pe);
      status |= i013_entry(num_pe);
  }

  if (g_sbsa_level > 6) {
     status |= i014_entry(num_pe);
     status |= i015_entry(num_pe);
  }
#endif
#if defined(TARGET_LINUX) || defined(TARGET_EMULATION)
  if (level > 6)
     status |= i016_entry(num_pe);
#endif
  val_print_test_end(status, "SMMU");

  return status;
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
  val_print(AVS_PRINT_DEBUG, "Input dma addr = %llx \n", dma_addr);

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
val_smmu_max_pasids(uint32_t smmu_index)
{
  uint64_t smmu_base;

  smmu_base = val_iovirt_get_smmu_info(SMMU_CTRL_BASE, smmu_index);
  return pal_smmu_max_pasids(smmu_base);
}

uint32_t
val_smmu_create_pasid_entry(uint32_t smmu_index, uint32_t pasid)
{
  uint64_t smmu_base;

  smmu_base = val_smmu_get_info(SMMU_CTRL_BASE, smmu_index);
  return pal_smmu_create_pasid_entry(smmu_base, pasid);
}

uint64_t
val_smmu_pa2iova(uint32_t smmu_index, uint64_t pa)
{
  uint64_t smmu_base;

  smmu_base = val_smmu_get_info(SMMU_CTRL_BASE, smmu_index);
  return pal_smmu_pa2iova(smmu_base, pa);
}