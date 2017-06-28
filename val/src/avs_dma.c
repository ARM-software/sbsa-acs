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
#include "include/sbsa_avs_dma.h"

DMA_INFO_TABLE  *g_dma_info_table;

/**
  @brief  Allocate memory which is to be used for DMA

  @param  None

  @result None
**/
addr_t
val_dma_mem_alloc(void **buffer, uint32_t size, uint32_t dev_index, uint32_t flags)
{

  void *ap = NULL;

  ap = (void *)val_dma_get_info(DMA_PORT_INFO, dev_index);
  return pal_dma_mem_alloc(buffer, size, ap, flags);

}

void
val_dma_free_info_table(void)
{
  pal_mem_free((void *)g_dma_info_table);
}

uint32_t
val_dma_start_from_device(void *buffer, uint32_t length, uint32_t ctrl_index)
{
  void *host, *target;

  host = (void *)val_dma_get_info(DMA_HOST_INFO, ctrl_index);
  target = (void *)val_dma_get_info(DMA_TARGET_INFO, ctrl_index);

  pal_dma_start_from_device(buffer, length, host, target);

  return 0;
}

/**
  @brief  Perform DMA operation from Memory to Device
          1. Caller       - Test Suite
          2. Prerequisite - val_dma_create_info_table
  @param  buffer     - input buffer with data
  @param  length     - size of the data transfer
  @param  ctrl_index _ the device which is the target of the DMA transfer

  @return None
**/
uint32_t
val_dma_start_to_device(void *buffer, uint32_t length, uint32_t ctrl_index)
{
  void *host, *target;

  host = (void *)val_dma_get_info(DMA_HOST_INFO, ctrl_index);
  target = (void *)val_dma_get_info(DMA_TARGET_INFO, ctrl_index);

  pal_dma_start_to_device(buffer, length, host, target, 10000);

  return 0;
}


/* Pre-requisite - val_peripheral_create_info_table */
void
val_dma_create_info_table(uint64_t *dma_info_ptr)
{

  g_dma_info_table = (DMA_INFO_TABLE *)dma_info_ptr;

  pal_dma_create_info_table(g_dma_info_table);

  val_print(AVS_PRINT_TEST, " DMA_INFO: Number of DMA CTRL in PCIe :    %x \n", val_dma_get_info(DMA_NUM_CTRL, 0));
}


/**
  @brief  Single entry point to return all DMA Controller related information.
          1. Caller       - Test Suite
          2. Prerequisite - val_dma_create_info_table
  @param  info_type - Type of DMA controller whose index needs to be returned
  @param  instance  - id (0' based) for which the info has to be returned

  @return  64-bit data of dma info matching type and instance
**/
uint64_t
val_dma_get_info(DMA_INFO_e type, uint32_t index)
{

  if (g_dma_info_table == NULL)
  {
      val_print(AVS_PRINT_ERR, "GET_DMA_INFO: DMA info table is not created \n", 0);
      return 0;
  }
  if (index > g_dma_info_table->num_dma_ctrls)
  {
      val_print(AVS_PRINT_ERR, "GET_DMA_INFO: Index (%d) is greater than num of DMA \n", index);
      return 0;
  }

  switch (type)
  {
      case DMA_NUM_CTRL:
          return g_dma_info_table->num_dma_ctrls;

      case DMA_HOST_INFO:
          return (uint64_t)g_dma_info_table->info[index].host;

      case DMA_PORT_INFO:
          return (uint64_t)g_dma_info_table->info[index].port;

      case DMA_TARGET_INFO:
          return (uint64_t)g_dma_info_table->info[index].target;

      case DMA_HOST_COHERENT:
          return ((uint64_t)g_dma_info_table->info[index].flags & (DMA_COHERENT_MASK));

      case DMA_HOST_IOMMU_ATTACHED:
          return ((uint64_t)g_dma_info_table->info[index].flags & (IOMMU_ATTACHED_MASK));

      case DMA_HOST_PCI:
          return ((uint64_t)g_dma_info_table->info[index].flags & (PCI_EP_MASK));

      default:
          val_print(AVS_PRINT_ERR, "This DMA info option not supported %d \n", type);
          break;
  }

  return 0;
}

/**
  @brief  Hook to get the DMA address used by the controller pointed by the index
          1. Caller       - Test Suite
          2. Prerequisite - val_dma_create_info_table
  @param  ctrl_index - Index DMA controller whose index needs to be returned
  @param  dma_addr   - DMA device visible Address returned from the device driver
  @param  cpu_addr   - The CPU visible Address returned from the device driver

  @return None
**/

void
val_dma_device_get_dma_addr(uint32_t ctrl_index, void *dma_addr, uint32_t *cpu_len)
{
  void *ap = NULL;

  ap = (void *)val_dma_get_info(DMA_PORT_INFO, ctrl_index);
  pal_dma_scsi_get_dma_addr(ap, dma_addr, cpu_len);

}

int
val_dma_mem_get_attrs(void *buf, uint32_t *attr, uint32_t *sh)
{
  return pal_dma_mem_get_attrs(buf, attr, sh);
}
