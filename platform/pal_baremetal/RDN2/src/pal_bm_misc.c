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


#include "pal_pcie_enum.h"
#include "pal_common_support.h"
#include "platform_image_def.h"
#include "platform_override_fvp.h"

#define __ADDR_ALIGN_MASK(a, mask)    (((a) + (mask)) & ~(mask))
#define ADDR_ALIGN(a, b)              __ADDR_ALIGN_MASK(a, (typeof(a))(b) - 1)

void *mem_alloc(size_t alignment, size_t size);
void mem_free(void *ptr);

typedef struct {
    uint64_t base;
    uint64_t size;
} val_host_alloc_region_ts;

static uint64_t heap_base;
static uint64_t heap_top;
static uint64_t heap_init_done = 0;


/**
  @brief  Sends a formatted string to the output console

  @param  string  An ASCII string
  @param  data    data for the formatted output

  @return None
**/
void
pal_print(char *string, uint64_t data)
{
  (void) string;
  (void) data;
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
  (void) mem_base;
  (void) Size;
  return (void*) NULL;
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
  (void) mem_base;
  (void) Size;
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
  (void) Bdf;
  *Pa = mem_alloc(MEM_ALIGN_4K, Size);
  return (void *) *Pa;
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
 (void) Bdf;
 (void) Size;
 (void) Pa;
 (void) Va;
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
  (void) MicroSeconds;
  return 0;
}

/**
  @brief  page size being used in current translation regime.

  @return page size being used
**/
uint32_t
pal_mem_page_size()
{
    return PLATFORM_PAGE_SIZE;
}

/**
  @brief  allocates contiguous numpages of size
          returned by pal_mem_page_size()

  @return Start address of base page
**/
void *
pal_mem_alloc_pages (uint32_t NumPages)
{
  return (void *)mem_alloc(MEM_ALIGN_4K, NumPages * PLATFORM_PAGE_SIZE);
}

/**
  @brief  frees continguous numpages starting from page
          at address PageBase

**/
void
pal_mem_free_pages(void *PageBase, uint32_t NumPages)
{
  (void) PageBase;
  (void) NumPages;
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
  return (void *)mem_alloc(alignment, size);
}

/**
  @brief  Free the Aligned memory allocated by UEFI Framework APIs

  @param  Buffer        the base address of the aligned memory range

  @return None
*/

void
pal_mem_free_aligned(void *Buffer)
{
    mem_free(Buffer);
    return;
}

/* Functions implemented below are used to allocate memory from heap. Baremetal implementation
   of memory allocation.
*/

static int is_power_of_2(uint32_t n)
{
    return n && !(n & (n - 1));
}

/**
 * @brief Allocates contiguous memory of requested size(no_of_bytes) and alignment.
 * @param alignment - alignment for the address. It must be in power of 2.
 * @param Size - Size of the region. It must not be zero.
 * @return - Returns allocated memory base address if allocation is successful.
 *           Otherwise returns NULL.
 **/
void *heap_alloc(size_t alignment, size_t size)
{
    uint64_t addr;

    addr = ADDR_ALIGN(heap_base, alignment);
    size += addr - heap_base;

    if ((heap_top - heap_base) < size)
    {
       return NULL;
    }

    heap_base += size;

    return (void *)addr;
}

/**
 * @brief  Initialisation of allocation data structure
 * @param  void
 * @return Void
 **/
void mem_alloc_init(void)
{
    heap_base = PLATFORM_HEAP_REGION_BASE;
    heap_top = PLATFORM_HEAP_REGION_BASE + PLATFORM_HEAP_REGION_SIZE;
    heap_init_done = 1;
}

/**
 * @brief Allocates contiguous memory of requested size(no_of_bytes) and alignment.
 * @param alignment - alignment for the address. It must be in power of 2.
 * @param Size - Size of the region. It must not be zero.
 * @return - Returns allocated memory base address if allocation is successful.
 *           Otherwise returns NULL.
 **/
void *mem_alloc(size_t alignment, size_t size)
{
  void *addr = NULL;

  if(heap_init_done != 1)
    mem_alloc_init();

  if (size <= 0)
  {
    return NULL;
  }

  if (!is_power_of_2((uint32_t)alignment))
  {
    return NULL;
  }

  size += alignment - 1;
  addr = heap_alloc(alignment, size);

  return addr;
}

/**
 * TODO: Free the memory for given memory address
 * Currently acs code is initialisazing from base for every test,
 * the regions data structure is internal and below code only setting to zero
 * not actually freeing memory.
 * If require can revisit in future.
 **/
void mem_free(void *ptr)
{
  if (!ptr)
    return;

  return;
}
