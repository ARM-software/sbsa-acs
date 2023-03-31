/** @file
 * Copyright (c) 2023 Arm Limited or its affiliates. All rights reserved.
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


#include "../include/pal_common_support.h"
#include "include/pal_pcie_enum.h"

#ifdef ENABLE_OOB
/* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

#include  <Library/ShellCEntryLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellLib.h>
#include  <Library/PrintLib.h>
#include  <Library/BaseMemoryLib.h>
#include <Protocol/Cpu.h>
#endif

/**
  @brief   Allocate DMAable memory: (Aligned to 4K by default)

  @param   buffer - Pointer to return the buffer address
  @param   length - Number of bytes to allocate
  @param   dev    - Pointer to the device structure
  @param   flag   - Allocation flags

  @return  DMA address of memory allocated
**/
uint64_t
pal_dma_mem_alloc(void **buffer, uint32_t length, void *dev, uint32_t flag)
{
#ifdef ENABLE_OOB
  VOID *Mem = NULL;
  VOID **Aligned_Ptr = NULL;
  UINT32 alignment = MEM_ALIGN_4K;

  UINT64 Mask = ~(UINT64)(alignment - 1);

  /* Allocate memory with extra bytes, so we can return an aligned address*/
  Mem = (VOID *)pal_mem_alloc(length + alignment);

  if( Mem == NULL)
    return 0;

  /* Add the alignment to allocated memory address and align it to target alignment*/
  Aligned_Ptr = (VOID **)(((UINT64) Mem + alignment - 1) & Mask);

  /* Using a double pointer to store the address of allocated
     memory location so that it can be used to free the memory later*/
  Aligned_Ptr[-1] = Mem;

  *buffer = Aligned_Ptr;
#else
  *buffer = (void *)memalign(MEM_ALIGN_4K, length);
#endif

  return 0;
}

/**
  @brief   Free the memory allocated by pal_dma_mem_alloc.

  @param  buffer: memory mapped to the DMA that is tobe freed
  @param  mem_dma: DMA address with respect to device
  @param  length: size of the memory
  @param  port: ATA port structure
  @param  flags: Allocation flags
**/
void pal_dma_mem_free(void *buffer, uint64_t mem_dma, unsigned int length, void *port, unsigned int flags)
{
#ifdef ENABLE_OOB
    free(((VOID **)buffer)[-1]);
#else
    free(buffer);
#endif

    return;
}

/**
  @brief  Abstracts the functionality of performing a DMA
          operation from the device to DDR memory

  @param  dma_target_buf is the target physical addressing
          the memory where the DMA data is to bewritten.
  @return 0 on success.
          IMPLEMENTATION DEFINED on error.
**/
unsigned int
pal_dma_start_from_device(void *dma_target_buf, unsigned int length,
                          void *host, void *dev)
{

   return 0;
}

/**
  @brief  Abstracts the functionality of performing a DMA
          operation to the device from the DDR memory

  @param  dma_source_buf: physical address in the memory
          where the DMA data is read from and has to be
          written to the device.
  @return 0 on success.
          IMPLEMENTATION DEFINED on error.
**/
unsigned int
pal_dma_start_to_device(void *dma_source_buf, unsigned int length,
                         void *host, void *target, unsigned int timeout)
{
   return 0;
}

/**
  @brief   Get the DMA address used by the queried DMA controller port

  @param   port : DMA port information
  @param   dma_address : Pointer where the DMA address needs to be returned
  @param   dma_len : Length of the DMA address mapping

  @return  None
**/
void
pal_dma_scsi_get_dma_addr(void *port, void *dma_addr, unsigned int *dma_len)
{

    /* *dma_addr = dma_address;
     * *dma_len  = dma_length;
    */
}

/**
  @brief   Get atributes of DMA memory

  @param   buf   - Pointer to return the buffer
  @param   attr  - Variable to return the attributes
  @param   sh    - Inner sharable domain or not

  @return  0 on SUCCESS or 1 for FAIL
**/
int
pal_dma_mem_get_attrs(void *buf, uint32_t *attr, uint32_t *sh)
{

  /* Pointer to return: the attributes (attr)
   * and shareable domain (sh)
  */

  return 1;
}
