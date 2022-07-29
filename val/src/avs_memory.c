/** @file
 * Copyright (c) 2016-2022, Arm Limited or its affiliates. All rights reserved.
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
#include "include/sbsa_avs_peripherals.h"
#include "include/sbsa_avs_common.h"


MEMORY_INFO_TABLE  *g_memory_info_table;

#ifndef TARGET_LINUX
/**
  @brief   This API will execute all Memory tests designated for a given compliance level
           1. Caller       -  Application layer.
           2. Prerequisite -  val_memory_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_memory_execute_tests(uint32_t level, uint32_t num_pe)
{

  uint32_t status;

  status = m001_entry(num_pe);

  val_print_test_end(status, "Memory");

  return status;
}

/**
  @brief  Free the memory allocated for the Memory Info table
**/
void
val_memory_free_info_table()
{
  pal_mem_free((void *)g_memory_info_table);
}

/**
  @brief   This function will call PAL layer to fill all relevant peripheral
           information into the g_peripheral_info_table pointer.
           1. Caller       - Application layer
           2. Prerequisite - Memory allocated and passed as argument.
  @param   memory_info_table - Address where the memory info table is created

  @return  None
**/

void
val_memory_create_info_table(uint64_t *memory_info_table)
{

  g_memory_info_table = (MEMORY_INFO_TABLE *)memory_info_table;

  pal_memory_create_info_table(g_memory_info_table);

}
#endif

/**
  @brief   Return the Index of the entry in the peripheral info table
           which matches the input type and the input instance number
           Instance number is '0' based
           1. Caller       -  VAL
           2. Prerequisite -
  @param   type     - type of memory being requested
  @param   instance - instance is '0' based and incremented to get different ranges
  @return  index
**/
uint32_t
val_memory_get_entry_index(uint32_t type, uint32_t instance)
{
  uint32_t  i = 0;

  while (g_memory_info_table->info[i].type != MEMORY_TYPE_LAST_ENTRY) {
      if (g_memory_info_table->info[i].type == type) {
          if (instance == 0)
             return i;
          else
             instance--;

      }
      i++;
  }
  return 0xFF;
}

/**
  @brief   Returns a random address whose attributes match the input
           memory type
           1. Caller       - Test Suite
           2. Prerequisite - val_memory_create_info_table
  @param   type     - type of memory being requested
  @param   instance - instance is '0' based and incremented to get different ranges
  @param   attr     - For cacheability etc. Not used for now.

  @return  64-bit address matching the input criteria
**/
addr_t
val_memory_get_addr(MEMORY_INFO_e mem_type, uint32_t instance, uint64_t *attr)
{
  uint32_t i;

  if (g_memory_info_table == NULL)
      return 0;

  switch(mem_type) {
      case MEM_TYPE_DEVICE:
          i = val_memory_get_entry_index(MEMORY_TYPE_DEVICE, instance);
          break;
      case MEM_TYPE_NORMAL:
          i = val_memory_get_entry_index(MEMORY_TYPE_NORMAL, instance);
          break;
      default:
          i = 0xFF;
          break;
  }
  if (i != 0xFF) {
      *attr = g_memory_info_table->info[i].flags;
      return g_memory_info_table->info[i].phy_addr;
  }
  return 0;
}

/**
  @brief   Returns the type and attributes of a given memory address
           1. Caller       - Test Suite
           2. Prerequisite - val_memory_create_info_table
  @param   addr     - Address whose type and attributes are being requested
  @param   attr     - For cacheability etc. Not used for now.

  @return  type of the memory address
**/
uint64_t
val_memory_get_info(addr_t addr, uint64_t *attr)
{

  uint32_t index = 0;

  while (g_memory_info_table->info[index].type != MEMORY_TYPE_LAST_ENTRY) {
      if ((addr >= g_memory_info_table->info[index].phy_addr) &&
        (addr < (g_memory_info_table->info[index].phy_addr +
        g_memory_info_table->info[index].size))) {
          *attr =  g_memory_info_table->info[index].flags;
          return g_memory_info_table->info[index].type;
       }
       index++;
      // val_print(AVS_PRINT_INFO," .", 0);
  }

  return MEM_TYPE_NOT_POPULATED;

}

addr_t
val_memory_ioremap(void *addr, uint32_t size, uint64_t attr)
{
  return (pal_memory_ioremap(addr, size, attr));
}

void
val_memory_unmap(void *ptr)
{
  pal_memory_unmap(ptr);
}

void *
val_memory_alloc(uint32_t size)
{
  return pal_mem_alloc(size);
}

void *
val_memory_calloc(uint32_t num, uint32_t size)
{
  return pal_mem_calloc(num, size);
}

void *
val_memory_alloc_cacheable(uint32_t bdf, uint32_t size, void **pa)
{
  return pal_mem_alloc_cacheable(bdf, size, pa);
}

void
val_memory_free(void *addr)
{
  pal_mem_free(addr);
}

int
val_memory_compare(void *src, void *dest, uint32_t len)
{
  return pal_mem_compare(src, dest, len);
}

void
val_memory_set(void *buf, uint32_t size, uint8_t value)
{
  pal_mem_set(buf, size, value);
}

void
val_memory_free_cacheable(uint32_t bdf, uint32_t size, void *va, void *pa)
{
  pal_mem_free_cacheable(bdf, size, va, pa);
}

void *
val_memory_virt_to_phys(void *va)
{
  return pal_mem_virt_to_phys(va);
}

void *
val_memory_phys_to_virt(uint64_t pa)
{
  return pal_mem_phys_to_virt(pa);
}

/**
  @brief  Return the address of unpopulated memory of requested
          instance from the GCD memory map.

  @param  addr      - Address of the unpopulated memory
          instance  - Instance of memory

  @return 0 - SUCCESS
          1 - No unpopulated memory present
          2 - FAILURE
**/
uint64_t
val_memory_get_unpopulated_addr(addr_t *addr, uint32_t instance)
{
  return pal_memory_get_unpopulated_addr(addr, instance);
}

uint32_t val_memory_page_size(void)
{
    return pal_mem_page_size();
}

void *
val_memory_alloc_pages(uint32_t num_pages)
{
    return pal_mem_alloc_pages(num_pages);
}

void
val_memory_free_pages(void *addr, uint32_t num_pages)
{
    pal_mem_free_pages(addr, num_pages);
}

/**
  @brief  Allocates memory with the given alignement.

  @param  Alignment   Specifies the alignment.
  @param  Size        Requested memory allocation size.

  @return Pointer to the allocated memory with requested alignment.
**/
void
*val_aligned_alloc( uint32_t alignment, uint32_t size )
{

  return pal_aligned_alloc(alignment, size);

}
