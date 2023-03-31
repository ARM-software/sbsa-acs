/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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


#include "include/pal_pcie_enum.h"
#include "include/pal_common_support.h"

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
  @brief  Sends a formatted string to the output console

  @param  string  An ASCII string
  @param  data    data for the formatted output

  @return None
**/
void
pal_print(char *string, uint64_t data)
{

#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
 */
    AsciiPrint(string, data);
#endif

}

/**
  @brief   Creates a buffer with length equal to size within the
           address range (mem_base, mem_base + mem_size)

  @param   mem_base    - Base address of the memory range
  @param   mem_size    - Size of the memory range of interest
  @param   size        - Buffer size to be created

  @return  Buffer address if SUCCESSFUL, else NULL
**/
void *
pal_mem_alloc_at_address (
  uint64_t mem_base,
  uint64_t Size
  )
{
#ifdef ENABLE_OOB
  EFI_STATUS Status;
  EFI_PHYSICAL_ADDRESS PageBase;

  PageBase = mem_base;
  Status = gBS->AllocatePages (AllocateAddress,
                               EfiBootServicesData,
                               EFI_SIZE_TO_PAGES(Size),
                               &PageBase);
  if (EFI_ERROR(Status))
  {
    print(AVS_PRINT_ERR, " Allocate Pages failed %x \n", Status);
    return NULL;
  }

  return (VOID*)(UINTN)PageBase;
#endif
}

/**
  @brief  Free the memory allocated by UEFI Framework APIs
  @param  Buffer the base address of the memory range to be freed

  @return None
**/
void
pal_mem_free_at_address(uint64_t mem_base,
  uint64_t Size
)
{
#ifdef ENABLE_OOB
  gBS->FreePages(mem_base, EFI_SIZE_TO_PAGES(Size));
#endif
}

/**
  @brief  Allocates memory of the requested size.

  @param  Bdf:  BDF of the requesting PCIe device
  @param  Size: size of the memory region to be allocated
  @param  Pa:   physical address of the allocated memory
**/
void *
pal_mem_alloc_cacheable(uint32_t Bdf, uint32_t Size, void **Pa)
{

#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  EFI_PHYSICAL_ADDRESS      Address;
  EFI_CPU_ARCH_PROTOCOL     *Cpu;
  EFI_STATUS                Status;

  Status = gBS->AllocatePages (AllocateAnyPages,
                               EfiBootServicesData,
                               EFI_SIZE_TO_PAGES(Size),
                               &Address);
  if (EFI_ERROR(Status)) {
    print(AVS_PRINT_ERR, "Allocate Pool failed %x \n", Status);
    return NULL;
  }

  /* Check Whether Cpu architectural protocol is installed */
  Status = gBS->LocateProtocol ( &gEfiCpuArchProtocolGuid, NULL, (VOID **)&Cpu);
  if (EFI_ERROR(Status)) {
    print(AVS_PRINT_ERR, "Could not get Cpu Arch Protocol %x \n", Status);
    return NULL;
  }

  /* Set Memory Attributes */
  Status = Cpu->SetMemoryAttributes (Cpu,
                                     Address,
                                     Size,
                                     EFI_MEMORY_WB);
  if (EFI_ERROR (Status)) {
    print(AVS_PRINT_ERR, "Could not Set Memory Attribute %x \n", Status);
    return NULL;
  }

  *Pa = (VOID *)Address;
  return (VOID *)Address;
#endif

  return 0;
}

/**
  @brief  Frees the memory allocated

  @param  Bdf:  BDF of the requesting PCIe device
  @param  Size: size of the memory region to be freed
  @param  Va:   virtual address of the memory to be freed
  @param  Pa:   physical address of the memory to be freed
**/
void
pal_mem_free_cacheable(uint32_t Bdf, uint32_t Size, void *Va, void *Pa)
{

#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  gBS->FreePages((EFI_PHYSICAL_ADDRESS)(UINTN)Va, EFI_SIZE_TO_PAGES(Size));
#endif

}

/**
  @brief  Returns the physical address of the input virtual address.

  @param Va virtual address of the memory to be converted

  Returns the physical address.
**/
void *
pal_mem_virt_to_phys(void *Va)
{
  /* Place holder function. Need to be
   * implemented if needed in later releases
   */
  return Va;
}

/**
  @brief  Returns the virtual address of the input physical address.

  @param Pa physical address of the memory to be converted

  Returns the virtual address.
**/
void *
pal_mem_phys_to_virt (
  uint64_t Pa
  )
{
  /* Place holder function*/
  return (void*)Pa;
}

/**
  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return 1 - Success, 0 -Failure

**/
uint64_t
pal_time_delay_ms(uint64_t MicroSeconds)
{
  /**Need to implement**/
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

  gBS->Stall(MicroSeconds);
#endif
  return 0;
}

/**
  @brief  page size being used in current translation regime.

  @return page size being used
**/
uint32_t
pal_mem_page_size()
{
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

    return EFI_PAGE_SIZE;
#endif
    return 0;
}

/**
  @brief  allocates contiguous numpages of size
          returned by pal_mem_page_size()

  @return Start address of base page
**/
void *
pal_mem_alloc_pages (uint32_t NumPages)
{
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

  EFI_STATUS Status;
  EFI_PHYSICAL_ADDRESS PageBase;

  Status = gBS->AllocatePages (AllocateAnyPages,
                               EfiBootServicesData,
                               NumPages,
                               &PageBase);
  if (EFI_ERROR(Status))
  {
    print(AVS_PRINT_ERR, "Allocate Pages failed %x \n", Status);
    return NULL;
  }

  return (VOID*)(UINTN)PageBase;
#endif
  return 0;

}

/**
  @brief  frees continguous numpages starting from page
          at address PageBase

**/
void
pal_mem_free_pages(void *PageBase, uint32_t NumPages)
{
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

  gBS->FreePages((EFI_PHYSICAL_ADDRESS)(UINTN)PageBase, NumPages);
#endif
}

/**
  @brief  Allocates memory with the given alignement.

  @param  Alignment   Specifies the alignment.
  @param  Size        Requested memory allocation size.

  @return Pointer to the allocated memory with requested alignment.
**/
void
*pal_aligned_alloc( uint32_t alignment, uint32_t size )
{
#ifdef ENABLE_OOB
  VOID *Mem = NULL;
  VOID **Aligned_Ptr = NULL;

  /* Generate mask for the Alignment parameter*/
  UINT64 Mask = ~(UINT64)(alignment - 1);

  /* Allocate memory with extra bytes, so we can return an aligned address*/
  Mem = (VOID *)pal_mem_alloc(size + alignment);

  if( Mem == NULL)
    return 0;

  /* Add the alignment to allocated memory address and align it to target alignment*/
  Aligned_Ptr = (VOID **)(((UINT64) Mem + alignment - 1) & Mask);

  /* Using a double pointer to store the address of allocated
     memory location so that it can be used to free the memory later*/
  Aligned_Ptr[-1] = Mem;

  return Aligned_Ptr;

#else
  return (void *)memalign(alignment, size);

#endif
}

/**
  @brief  Free the Aligned memory allocated by UEFI Framework APIs

  @param  Buffer        the base address of the aligned memory range

  @return None
*/

void
pal_mem_free_aligned (void *Buffer)
{
#ifdef ENABLE_OOB
    free(((VOID **)Buffer)[-1]);
    return;
#else
    free(Buffer);
    return;
#endif
}

