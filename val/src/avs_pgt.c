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

#include "include/sbsa_avs_pgt.h"
#include "include/sbsa_avs_memory.h"
#include "include/sbsa_avs_mmu.h"

#define get_min(a, b) ((a) < (b))?(a):(b)

#define PGT_DEBUG_LEVEL AVS_PRINT_INFO

static uint32_t page_size;
static uint32_t bits_per_level;
static uint64_t pgt_addr_mask;
uint64_t is_values_init = 0;
uint64_t offset = 0;

typedef struct
{
    uint64_t *tt_base;
    uint64_t input_base;
    uint64_t input_top;
    uint64_t output_base;
    uint32_t level;
    uint32_t size_log2;
    uint32_t nbits;
} tt_descriptor_t;

typedef struct {
    uint32_t l0_index;
    uint32_t l1_index;
    uint32_t l2_index;
    uint32_t l3_index;
    uint64_t size_used;
} acs_pgt_t;

static acs_pgt_t acs_pgt_info;

void setup_acs_pgt_values(void)
{
    acs_pgt_info.l0_index = 0;
    acs_pgt_info.l1_index = 0;
    acs_pgt_info.l2_index = 0;
    acs_pgt_info.l3_index = 0;
    acs_pgt_info.size_used = 0;
}


static
uint32_t get_pgt_index(uint32_t level)
{
    switch (level)
    {
        case(PGT_LEVEL_0):
            return acs_pgt_info.l0_index;
        case(PGT_LEVEL_1):
            return acs_pgt_info.l1_index;
        case(PGT_LEVEL_2):
            return acs_pgt_info.l2_index;
        case(PGT_LEVEL_3):
            return acs_pgt_info.l3_index;
        default:
            return 0;
    }
}

static
void increment_pgt_index(uint32_t level, uint32_t max_index)
{
    switch (level)
    {
        case(PGT_LEVEL_1):
            if (acs_pgt_info.l1_index == (max_index - 1)) {
                acs_pgt_info.l1_index = 0;
                acs_pgt_info.l0_index++;
            }
            break;

        case(PGT_LEVEL_2):
            if (acs_pgt_info.l2_index == (max_index - 1)) {
                acs_pgt_info.l2_index = 0;
                acs_pgt_info.l1_index++;
            }
            break;

        case(PGT_LEVEL_3):
            if (acs_pgt_info.l3_index == (max_index - 1)) {
                acs_pgt_info.l3_index = 0;
                acs_pgt_info.l2_index++;
            } else
                acs_pgt_info.l3_index++;
            break;
        default:
            break;
    }
}

static
uint32_t get_entries_per_level(uint32_t page_size)
{
    switch (page_size)
    {
        case(PAGE_SIZE_4K):   //4kb granule
            return MAX_ENTRIES_4K;
        case(PAGE_SIZE_16K):  //16kb granule
            return MAX_ENTRIES_16K;
        case(PAGE_SIZE_64K):  //64kb granule
            return MAX_ENTRIES_64K;
        default:
            val_print(AVS_PRINT_ERR, "\n       %llx granularity not supported.", page_size);
            return 0;
    }
}

static
uint64_t get_block_size(uint32_t level)
{
    uint32_t entries = get_entries_per_level(page_size);

    switch (level)
    {
        case(PGT_LEVEL_0):  // For L0 table translation
            if (page_size == PAGE_SIZE_4K)
                return (uint64_t)(page_size * entries * entries * entries);
            else if (page_size == PAGE_SIZE_16K)
                return (uint64_t)(page_size * entries * entries * 2); // only 2 lookup tables in L0
            else {
                val_print(AVS_PRINT_ERR, "\n       L0 tables not supported for page size %llx",
                                                                                        page_size);
                return 0;
            }

        case(PGT_LEVEL_1):  // For L1 table translation
            if (page_size == PAGE_SIZE_4K || page_size == PAGE_SIZE_16K)
                return (uint64_t)(page_size * entries * entries);
            else
                return (uint64_t)(page_size * entries * 64); // 64 Lookup tables in L1 (64KB Gran)

        case(PGT_LEVEL_2):  // For L2 table translation
            return (uint64_t)(page_size * entries);
        case(PGT_LEVEL_3):  // For L3 table translation
            return (uint64_t)(page_size);
        default:
            return 0;
    }
}

uint32_t fill_translation_table(tt_descriptor_t tt_desc, memory_region_descriptor_t *mem_desc)
{
    uint64_t block_size = 0x1ull << tt_desc.size_log2;
    uint64_t input_address, output_address, filled_tables, table_index, max_allowed_mem;
    uint64_t *tt_base_next_level, *table_desc;
    tt_descriptor_t tt_desc_next_level;

    val_print(PGT_DEBUG_LEVEL, "\n      tt_desc.level: %d     ", tt_desc.level);
    val_print(PGT_DEBUG_LEVEL, "\n      tt_desc.input_base: 0x%llx     ", tt_desc.input_base);
    val_print(PGT_DEBUG_LEVEL, "\n      tt_desc.input_top: 0x%llx     ", tt_desc.input_top);
    val_print(PGT_DEBUG_LEVEL, "\n      tt_desc.output_base: 0x%llx     ", tt_desc.output_base);
    val_print(PGT_DEBUG_LEVEL, "\n      tt_desc.size_log2: %d     ", tt_desc.size_log2);
    val_print(PGT_DEBUG_LEVEL, "\n      tt_desc.nbits: %d     ", tt_desc.nbits);

    if (!is_values_init) {
        setup_acs_pgt_values();
        is_values_init = 1;
    }

    for (input_address = tt_desc.input_base, output_address = tt_desc.output_base;
         input_address < tt_desc.input_top;
         input_address += (block_size - offset), output_address += (block_size - offset))
    {
        table_index = input_address >> tt_desc.size_log2 & ((0x1ull << tt_desc.nbits) - 1);
        table_desc = &tt_desc.tt_base[table_index];

        val_print(PGT_DEBUG_LEVEL, "\n      table_index = %d     ", table_index);

        if (tt_desc.level == 3)
        {
            //Create level 3 page descriptor entry
            *table_desc = PGT_ENTRY_PAGE_MASK | PGT_ENTRY_VALID_MASK;
            *table_desc |= (output_address & ~(uint64_t)(page_size - 1));
            *table_desc |= mem_desc->attributes;
            val_print(PGT_DEBUG_LEVEL, "\n      page_descriptor = 0x%llx     ", *table_desc);
            /* Keep a count of number of L3 tables filled. If the number exceedes the limit, move
               to next L2 table and continue.  */
            increment_pgt_index(tt_desc.level, get_entries_per_level(page_size));
            offset = 0;
            continue;
        }

        //Are input and output addresses eligible for being described via block descriptor?
        if ((input_address & (block_size - 1)) == 0 &&
             (output_address & (block_size - 1)) == 0 &&
             tt_desc.input_top >= (input_address + block_size - 1)) {
            //Create a block descriptor entry
            *table_desc = PGT_ENTRY_BLOCK_MASK | PGT_ENTRY_VALID_MASK;
            *table_desc |= (output_address & ~(block_size - 1));
            *table_desc |= mem_desc->attributes;
            val_print(PGT_DEBUG_LEVEL, "\n      block_descriptor = 0x%llx     ", *table_desc);
            increment_pgt_index(tt_desc.level, get_entries_per_level(page_size));
            offset = 0;
            continue;
        }
        /*
        If there's no descriptor populated at current index of this page_table, or
        If there's a block descriptor, allocate new page, else use the already populated address.
        Block descriptor info will be overwritten in case its there.
        */
        if (*table_desc == 0 || IS_PGT_ENTRY_BLOCK(*table_desc))
        {
            tt_base_next_level = val_memory_alloc_pages(1);
            if (tt_base_next_level == NULL)
            {
                val_print(AVS_PRINT_ERR, "\n      fill_translation_table: page allocation failed     ", 0);
                return AVS_STATUS_ERR;
            }
            val_memory_set(tt_base_next_level, page_size, 0);
        }
        else
            tt_base_next_level = val_memory_phys_to_virt(*table_desc & pgt_addr_mask);

        tt_desc_next_level.tt_base      = tt_base_next_level;
        tt_desc_next_level.input_base   = input_address;
        filled_tables                   = get_pgt_index(tt_desc.level + 1);
        offset                          = filled_tables * get_block_size(tt_desc.level + 1);

        val_print(PGT_DEBUG_LEVEL, "       filled_tables in next level = 0x%llx", filled_tables);
        val_print(PGT_DEBUG_LEVEL, "       offset = 0x%llx", offset);

        // Calculate the maximum allowed mem addr that can be mapped for the L0/L1/L2 table.
        // This prevents overwriting page tables.
        max_allowed_mem                = input_address + block_size - offset - 1;
        tt_desc_next_level.input_top   = get_min(tt_desc.input_top, max_allowed_mem);
        tt_desc_next_level.output_base = output_address;
        tt_desc_next_level.level       = tt_desc.level + 1;
        tt_desc_next_level.size_log2   = tt_desc.size_log2 - bits_per_level;
        tt_desc_next_level.nbits       = bits_per_level;
        increment_pgt_index(tt_desc.level, get_entries_per_level(page_size));

        if (fill_translation_table(tt_desc_next_level, mem_desc))
        {
            val_memory_free_pages(tt_base_next_level, 1);
            return AVS_STATUS_ERR;
        }

        *table_desc = PGT_ENTRY_TABLE_MASK | PGT_ENTRY_VALID_MASK;
        *table_desc |= (uint64_t)val_memory_virt_to_phys(tt_base_next_level) &
                       ~(uint64_t)(page_size - 1);
        val_print(PGT_DEBUG_LEVEL, "\n      table_descriptor = 0x%llx     ", *table_desc);
    }
    return 0;
}

uint32_t log2_page_size(uint64_t size)
{
    int bit = 0;
    while (size != 0)
    {
        if (size & 1)
            return bit;
        size >>= 1;
        ++bit;
    }
    return 0;
}

/**
  @brief Create stage 1 or stage 2 page table, with given memory addresses and attributes
         Note: This API updates existing translation table if pgt_desc->pgt_base is not NULL
               else it created new table and updated pgt_desc->pgt_base with the address.
  @param mem_desc - Array of memory addresses and attributes needed for page table creation.
  @param pgt_desc - Data structure for output page table base and input translation attributes.
  @return status
**/
uint32_t val_pgt_create(memory_region_descriptor_t *mem_desc, pgt_descriptor_t *pgt_desc)
{
    uint64_t *tt_base;
    tt_descriptor_t tt_desc;
    uint32_t num_pgt_levels, page_size_log2;
    memory_region_descriptor_t *mem_desc_iter;

    page_size = val_memory_page_size();
    page_size_log2 = log2_page_size(page_size);
    bits_per_level = page_size_log2 - 3;
    num_pgt_levels = (pgt_desc->ias - page_size_log2 + bits_per_level - 1)/bits_per_level;
    num_pgt_levels = (num_pgt_levels > 4)?4:num_pgt_levels;
    val_print(PGT_DEBUG_LEVEL, "\n      val_pgt_create: nbits_per_level = %d    ", bits_per_level);
    val_print(PGT_DEBUG_LEVEL, "\n      val_pgt_create: page_size_log2 = %d     ", page_size_log2);

    /* check whether input page descriptor has base addr of translation table
       to use. If the pgt_base member is NULL allocate a page to create a new
       table, else update existing translation table */
    if (pgt_desc->pgt_base == (uint64_t) NULL) {
        tt_base = (uint64_t *) val_memory_alloc_pages(1);
        if (tt_base == NULL) {
            val_print(AVS_PRINT_ERR, "\n      val_pgt_create: page allocation failed     ", 0);
            return AVS_STATUS_ERR;
        }
        val_memory_set(tt_base, page_size, 0);
    }
    else
        tt_base = (uint64_t *) pgt_desc->pgt_base;

    tt_desc.tt_base = tt_base;
    pgt_addr_mask = ((0x1ull << (pgt_desc->ias - page_size_log2)) - 1) << page_size_log2;

    for (mem_desc_iter = mem_desc; mem_desc_iter->length != 0; ++mem_desc_iter)
    {
        val_print(PGT_DEBUG_LEVEL, "val_pgt_create:i/p addr = 0x%llx ", mem_desc->virtual_address);
        val_print(PGT_DEBUG_LEVEL, "val_pgt_create:o/p addr = 0x%llx ", mem_desc->physical_address);
        val_print(PGT_DEBUG_LEVEL, "val_pgt_create: length = 0x%llx\n ", mem_desc->length);
        if ((mem_desc->virtual_address & (uint64_t)(page_size - 1)) != 0 ||
            (mem_desc->physical_address & (uint64_t)(page_size - 1)) != 0)
            {
                val_print(AVS_PRINT_ERR, "\n      val_pgt_create: address alignment error     ", 0);
                return AVS_STATUS_ERR;
            }

        if (mem_desc->physical_address >= (0x1ull << pgt_desc->oas))
        {
            val_print(AVS_PRINT_ERR, "\n      val_pgt_create: output address size error     ", 0);
            return AVS_STATUS_ERR;
        }

        if (mem_desc->virtual_address >= (0x1ull << pgt_desc->ias))
        {
            val_print(AVS_PRINT_WARN, "\n      val_pgt_create: input address size error, truncating to %d-bits     ", pgt_desc->ias);
            mem_desc->virtual_address &= ((0x1ull << pgt_desc->ias) - 1);
        }

        tt_desc.input_base = mem_desc->virtual_address & ((0x1ull << pgt_desc->ias) - 1);
        tt_desc.input_top = tt_desc.input_base + mem_desc->length - 1;
        tt_desc.output_base = mem_desc->physical_address & ((0x1ull << pgt_desc->oas) - 1);
        tt_desc.level = 4 - num_pgt_levels;
        tt_desc.size_log2 = (num_pgt_levels - 1) * bits_per_level + page_size_log2;
        tt_desc.nbits = pgt_desc->ias - tt_desc.size_log2;

        if (fill_translation_table(tt_desc, mem_desc))
        {
            val_memory_free_pages(tt_base, 1);
            return AVS_STATUS_ERR;
        }
    }

    pgt_desc->pgt_base = (uint64_t)val_memory_virt_to_phys(tt_base);

    return 0;
}

/**
  @brief Get attributes of a page corresponding to a given virtual address.
  @param pgt_desc - page table base and translation attributes.
  @param virtual_address - virtual address of memory region base whose attribute is to be read.
  @param attributes - output attributes
  @return status
**/
uint64_t val_pgt_get_attributes(pgt_descriptor_t pgt_desc, uint64_t virtual_address, uint64_t *attributes)
{
    uint32_t ias, index, num_pgt_levels, this_level;
    uint32_t bits_at_this_level, bits_remaining;
    uint64_t val64, tt_base_phys, *tt_base_virt;
    uint32_t page_size_log2 = pgt_desc.tcr.tg_size_log2;

    if (attributes == NULL)
        return AVS_STATUS_ERR;

    if (!pgt_desc.pgt_base)
        return AVS_STATUS_ERR;

    ias = 64 - pgt_desc.tcr.tsz;

    bits_per_level = page_size_log2 - 3;
    num_pgt_levels = (ias - page_size_log2 + bits_per_level - 1)/bits_per_level;
    this_level = 4 - num_pgt_levels;
    bits_remaining = (num_pgt_levels - 1) * bits_per_level + page_size_log2;
    bits_at_this_level = ias - bits_remaining;
    tt_base_phys = pgt_desc.pgt_base;

    while (1) {
        index = (virtual_address >> bits_remaining) & ((0x1u << bits_at_this_level) - 1);
        tt_base_virt = (uint64_t*)val_memory_phys_to_virt(tt_base_phys);
        val64 = tt_base_virt[index];

        val_print(PGT_DEBUG_LEVEL, "\n       val_pgt_get_attr: this_level = %d ", this_level);
        val_print(PGT_DEBUG_LEVEL, "\n       val_pgt_get_attr: index = %d ", index);
        val_print(PGT_DEBUG_LEVEL,
                 "\n       val_pgt_get_attr: bits_remaining = %d", bits_remaining);
        val_print(PGT_DEBUG_LEVEL,
                 "\n       val_pgt_get_attr: tt_base_virt %llx", (uint64_t)tt_base_virt);
        val_print(PGT_DEBUG_LEVEL, "\n       val_pgt_get_attr: val64 = %llx ", val64);
        if (this_level == 3)
        {
            if (!IS_PGT_ENTRY_PAGE(val64))
                return AVS_STATUS_ERR;
            *attributes = PGT_DESC_ATTRIBUTES(val64);
            return 0;
        }
        if (IS_PGT_ENTRY_BLOCK(val64)) {
            *attributes = PGT_DESC_ATTRIBUTES(val64);
            return 0;
        }
        tt_base_phys = val64 & (((0x1ull << (ias - page_size_log2)) - 1) << page_size_log2);
        ++this_level;
        bits_remaining -= bits_at_this_level;
        bits_at_this_level = bits_per_level;
    }
}

static void free_translation_table(uint64_t *tt_base, uint32_t bits_at_this_level, uint32_t this_level)
{
    uint32_t index;
    uint64_t *tt_base_next_virt;

    if (this_level == 3)
        return;
    for (index = 0; index < (0x1ul << bits_at_this_level); ++index)
    {
        if (tt_base[index] != 0)
        {
            if (IS_PGT_ENTRY_BLOCK(tt_base[index]))
                continue;
            tt_base_next_virt = val_memory_phys_to_virt((tt_base[index] & pgt_addr_mask));
            if (tt_base_next_virt == NULL)
                continue;
            free_translation_table(tt_base_next_virt, bits_per_level, this_level+1);
            val_print(PGT_DEBUG_LEVEL, "\n      free_translation_table: tt_base_next_virt = %llx     ", (uint64_t)tt_base_next_virt);
            val_memory_free_pages(tt_base_next_virt, 1);
        }
    }
}

/**
  @brief Free all page tables in the page table hierarchy starting from the base page table.
  @param pgt_desc - page table base and translation attributes.
  @return void
**/
void val_pgt_destroy(pgt_descriptor_t pgt_desc)
{
    uint32_t page_size_log2, num_pgt_levels;
    uint64_t *pgt_base_virt = val_memory_phys_to_virt(pgt_desc.pgt_base);

    if (!pgt_desc.pgt_base)
        return;

    val_print(PGT_DEBUG_LEVEL, "\n      val_pgt_destroy: pgt_base = %llx     ", pgt_desc.pgt_base);
    page_size = val_memory_page_size();
    page_size_log2 = log2_page_size(page_size);
    bits_per_level =  page_size_log2 - 3;
    pgt_addr_mask = ((0x1ull << (pgt_desc.ias - page_size_log2)) - 1) << page_size_log2;
    num_pgt_levels = (pgt_desc.ias - page_size_log2 + bits_per_level - 1)/bits_per_level;

    free_translation_table(pgt_base_virt,
                           pgt_desc.ias - ((num_pgt_levels - 1) * bits_per_level + page_size_log2),
                           4 - num_pgt_levels);
    val_memory_free_pages(pgt_base_virt, 1);
}
