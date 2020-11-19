/** @file
 * Copyright (c) 2020, Arm Limited or its affiliates. All rights reserved.
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

#include "include/pal_common_support.h"
#include "include/pal_pcie_enum.h"

#define SMMU_V3_IDR1 0x4
#define SMMU_V3_IDR1_PASID_SHIFT 6
#define SMMU_V3_IDR1_PASID_MASK  0x1f

/**
  @brief   This API prepares the smmu page tables to support input PasId
  @param   SmmuBase - Physical addr of the SMMU for which PasId support is needed
  @param   PasId    - Process Address Space identifier
  @return  zero for success, one for failure
**/
uint32_t
pal_smmu_create_pasid_entry (uint64_t SmmuBase, uint32_t PasId)
{
    return 1;
}

/**
  @brief   This API globally disables the SMMU based on input base address
  @param   SmmuBase - Physical addr of the SMMU that needs to be globally disabled
  @return  zero for success, one for failure
**/
uint32_t
pal_smmu_disable (uint64_t SmmuBase)
{
  return 0;
}


/**
  @brief   This API converts physical address to IO virtual address
  @param   SmmuBase - Physical addr of the SMMU for pa to iova conversion
  @param   Pa       - Physical address to use in conversion
  @return  zero for success, one for failure
*/
uint64_t
pal_smmu_pa2iova (uint64_t SmmuBase, uint64_t Pa)
{
  return 0;
}

uint32_t
pal_smmu_max_pasids(uint64_t smmu_base)
{
  uint32_t reg = pal_mmio_read(smmu_base + SMMU_V3_IDR1);
  uint32_t pasid_bits = (reg >> SMMU_V3_IDR1_PASID_SHIFT) & SMMU_V3_IDR1_PASID_MASK;
  if(pasid_bits == 0)
     return 0;

  return (1 << pasid_bits);
}

/**
  @brief   Check if input address is within the IOVA translation range for the device
  @param   port - Pointer to the DMA port
  @param   dma_addr   - The input address to be checked
  @return  Success if the input address is found in the range
**/
uint32_t pal_smmu_check_device_iova(void *port, uint64_t dma_addr)
{

  return 0;
}

/**
  @brief   Start monitoring an IO virtual address coming from DMA port
  @param   port - Pointer to the DMA port
  @return  None
**/
void pal_smmu_device_start_monitor_iova(void *port)
{
  return;
}

/**
  @brief   Stop monitoring an IO virtual address coming from DMA port
  @param   port - Pointer to the DMA port
  @return  None
**/
void pal_smmu_device_stop_monitor_iova(void *port)
{
  return;
}
