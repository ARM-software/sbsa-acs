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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_common.h"
#include "include/sbsa_avs_mmu.h"
#include "include/sbsa_avs_pgt.h"
#include "include/sbsa_avs_pe.h"
#include "include/sbsa_avs_memory.h"
#include "include/val_interface.h"

static uint32_t log2_func(uint64_t size);

/**
  @brief   This API will check whether inputted base address is already
           mapped in the translation table or not.

  @param   addr - address to be checked.

  @return  0 - if mmu entry is present, 1 otherwise.
**/
uint32_t
val_mmu_check_for_entry(uint64_t addr)
{
  PE_TCR_BF tcr;
  uint64_t ttbr, ttable_entry;
  uint64_t tt_base_phys, *tt_base_virt;
  uint32_t ias, index, num_pgt_levels, this_level;
  uint32_t page_size_log2, bits_per_level, page_size;
  uint32_t bits_at_this_level, bits_remaining;

  /* Get translation attributes from TCR and translation table base from TTBR
     TTBR0 is used since we are accessing lower address region */
  if (val_pe_reg_read_tcr(0 /*for TTBR0*/, &tcr)) {
      val_print(AVS_PRINT_ERR, "\n   Failed to fetch TCR", 0);
      return 1;
  }

  if (val_pe_reg_read_ttbr(0 /*TTBR0*/, &ttbr)) {
      val_print(AVS_PRINT_ERR, "\n   Failed to fetch TTBR0", 0);
      return 1;
  }

    /* obtain page size */
    page_size = val_memory_page_size();
    /* number of bits required to index byte location inside the page */
    page_size_log2 = log2_func(page_size);
    /* calculate input addr size */
    ias = 64 - tcr.tsz;
    /* calculate bits resolved per level for all granule sizes */
    bits_per_level = page_size_log2 - 3;
    /* calculate max num of translation levels possible */
    num_pgt_levels = (ias - page_size_log2 + bits_per_level - 1)/bits_per_level;
    /* start translation from lowest level */
    this_level = PGT_LEVEL_MAX - num_pgt_levels;
    /* bits to translate after first translation */
    bits_remaining = (num_pgt_levels - 1) * bits_per_level + page_size_log2;
    /* bits translated at current level */
    bits_at_this_level = ias - bits_remaining;
    /* address to first translation table comes from TTBR0 register */
    tt_base_phys = ttbr & AARCH64_TTBR_ADDR_MASK;

    while (this_level < PGT_LEVEL_MAX) {
        /* translation starts from most siginificant bits of VA */
        index = (addr >> bits_remaining) & ((0x1u << bits_at_this_level) - 1);
        tt_base_virt = (uint64_t *)val_memory_phys_to_virt(tt_base_phys);
        /* index the translation table and read the entry */
        ttable_entry = tt_base_virt[index];

        val_print(AVS_PRINT_INFO, "\n   Translation table level         = %d", this_level);
        val_print(AVS_PRINT_INFO, "\n   Table base address              = 0x%llx",
                  (uint64_t)tt_base_virt);
        val_print(AVS_PRINT_INFO, "\n   Table entry index               = %d", index);
        val_print(AVS_PRINT_INFO, "\n   Table entry                     = 0x%llx",
                  ttable_entry);
        val_print(AVS_PRINT_INFO, "\n   VA bits remaining to be resolve = %d", bits_remaining);

        /* check whether the table entry is invalid */
        if (IS_PGT_ENTRY_INVALID(ttable_entry)) {
            val_print(AVS_PRINT_DEBUG, "\n   VA not mapped in translation table", 0);
            return 1;
        }

        /* As per Arm ARM, entry of type "table descriptor" is only
           valid at translation level 0 */
        if (this_level == 0 && !IS_PGT_ENTRY_TABLE(ttable_entry)) {
            val_print(AVS_PRINT_DEBUG,
                      "\n   VA not mapped correctly in translation table", 0);
            return 1;
        }

        if (this_level == 3) {
            /* at level 3 table entry should be of type "page descriptor" with
               ttable_entry[1:0] bits = b11 */
            if (!IS_PGT_ENTRY_PAGE(ttable_entry)) {
                val_print(AVS_PRINT_DEBUG,
                          "\n   VA not mapped correctly in translation table", 0);
                return 1;
            }
            else {
                val_print(AVS_PRINT_DEBUG, "\n   VA translation successful", 0);
                return 0;
            }
        }

        /* check whether table walk hit a block descriptor entry
           Note : level 0 table entry can't describe a page or a block
                  level 3 can only have entry of type page descriptor
                  (Refer Arm ARM for more info) */
        if (IS_PGT_ENTRY_BLOCK(ttable_entry) && this_level != 0) {
            val_print(AVS_PRINT_DEBUG, "\n   VA translation successful", 0);
            return 0;
        }

        /* point to next translation table if table walk still not hit
           a page or a block descriptor entry */
        tt_base_phys = ttable_entry & (((0x1ull << (ias - page_size_log2)) - 1) << page_size_log2);

        /* update level and remaining VA bits to resolve */
        ++this_level;
        bits_remaining -= bits_at_this_level;
        bits_at_this_level = bits_per_level;
    }
    /* execution should don't reach here */
    return 1;
}

/**
  @brief   This API adds translation table entry for memory region specified.
           Note : This API should only be used to map memory of type Device.

  @param   base_addr - base address of the memory.
  @param   size      - size of the memory region.

  @return  0 - if SUCCESS, 1 otherwise.
**/
uint32_t
val_mmu_add_entry(uint64_t base_addr, uint64_t size)
{
  pgt_descriptor_t pgt_desc;
  memory_region_descriptor_t mem_desc;
  uint64_t ttbr;
  const uint32_t oas_bit_arr[7] = {32, 36, 40, 42, 44, 48, 52}; /* Physical address sizes */

  /* init descriptors */
  val_memory_set(&mem_desc, sizeof(mem_desc), 0);
  val_memory_set(&pgt_desc, sizeof(pgt_desc), 0);

  /* Get translation attributes from TCR and translation table base from TTBR */
  if (val_pe_reg_read_tcr(0 /*for TTBR0*/, &pgt_desc.tcr)) {
      val_print(AVS_PRINT_ERR, "\n   Failed to fetch TCR", 0);
      return 1;
  }
  if (val_pe_reg_read_ttbr(0 /*TTBR0*/, &ttbr)) {
      val_print(AVS_PRINT_ERR, "\n   Failed to fetch TTBR0", 0);
      return 1;
  }

  /* update pgt descriptor structure with translation table base from TTBR0 */
  pgt_desc.pgt_base = (ttbr & AARCH64_TTBR_ADDR_MASK);
  /* only stage one translation */
  pgt_desc.stage = PGT_STAGE1;

  /* realise OAS and IAS from TCR register */
  pgt_desc.oas = oas_bit_arr[pgt_desc.tcr.ps];
  pgt_desc.ias = 64 - pgt_desc.tcr.tsz;
  val_print(AVS_PRINT_DEBUG, "\n   Input addr size in bits (ias) = %d", pgt_desc.ias);
  val_print(AVS_PRINT_DEBUG, "\n   Output addr size in bits (oas) = %d\n", pgt_desc.oas);

  /* populate mem descriptor structure with addr region to be mapped and attributes */
  mem_desc.virtual_address = base_addr;
  mem_desc.physical_address = base_addr;
  mem_desc.length = size;
  mem_desc.attributes = ATTR_DEVICE_nGnRnE | (1ull << MEM_ATTR_AF_SHIFT);

  /* update translation table entry(s) for addr region defined by memory descriptor structure  */
  if (val_pgt_create(&mem_desc, &pgt_desc)) {
      val_print(AVS_PRINT_ERR, "   Failed to create MMU translation entry(s)\n", 0);
      return 1;
  }
  return 0;
}

/**
  @brief  This API adds translation table entry for memory region specified
          if the region is not  yet mapped.
          Note : This API should only be used to map memory of type Device.

  @param  None

  @return 0 if Success, 1 otherwise.
**/
uint32_t val_mmu_update_entry(uint64_t address, uint32_t size)
{

  /* If entry is already present return success */
  if (!val_mmu_check_for_entry(address)) {
      val_print(AVS_PRINT_DEBUG, "\n   Address is already mapped", 0);
      return 0;
  }

  /* Add the entry and return status */
  return val_mmu_add_entry(address, size);
}

/**
  @brief  This is local function is used to get log base 2 of input value.

  @param  value - input value

  @return result
**/
static uint32_t log2_func(uint64_t value)
{
    uint32_t bit = 0;

    while (value != 0)
    {
        if (value & 1)
            return bit;
        value >>= 1;
        ++bit;
    }
    return 0;
}

#ifdef TARGET_BM_BOOT
/**
 * @brief Setup page table for image regions and device regions
 * @param void
 * @return status
**/
uint32_t val_setup_mmu(void)
{
    memory_region_descriptor_t mem_desc_array[2], *mem_desc;
    memory_region_descriptor_t *mmap_region_list;
    pgt_descriptor_t pgt_desc;
    uint8_t i = 0;
    uint32_t map_count;

    // Memory map the image regions
    val_mmu_add_mmap();

    pgt_desc.ias = MMU_PGT_IAS;
    pgt_desc.oas = MMU_PGT_OAS;

    pgt_desc.pgt_base = (uint64_t) tt_l0_base;
    pgt_desc.stage = PGT_STAGE1;

    val_print(AVS_PRINT_DEBUG, "       mmu: ias=%d\n", pgt_desc.ias);
    val_print(AVS_PRINT_DEBUG, "       mmu: oas=%d\n", pgt_desc.oas);

    /* Map regions */

    val_memory_set(mem_desc_array, sizeof(mem_desc_array), 0);
    mem_desc = &mem_desc_array[0];
    mmap_region_list = (memory_region_descriptor_t *) val_mmu_get_mmap_list();
    map_count = val_mmu_get_mapping_count();

    while (i < map_count)
    {
        mem_desc->virtual_address = mmap_region_list[i].virtual_address;
        mem_desc->physical_address = mmap_region_list[i].physical_address;
        mem_desc->length = mmap_region_list[i].length;
        mem_desc->attributes = mmap_region_list[i].attributes;

        val_print(AVS_PRINT_DEBUG, "\n       Creating page table for region  : 0x%lx",
                                                                        mem_desc->virtual_address);
        val_print(AVS_PRINT_DEBUG, "- 0x%lx\n", (mem_desc->virtual_address + mem_desc->length) - 1);

        if (val_pgt_create(mem_desc, &pgt_desc))
        {
            return AVS_STATUS_ERR;
        }
        i++;
    }

    return AVS_STATUS_PASS;
}

/**
 * @brief Enable mmu through configuring mmu registers
 * @param void
 * @return status
**/
uint32_t val_enable_mmu(void)
{
    uint64_t tcr;
    uint32_t currentEL;
    currentEL = (val_read_current_el() & 0xc) >> 2;

    /*
     * Setup Memory Attribute Indirection Register
     * Attr0 = b01000100 = Normal, Inner/Outer Non-Cacheable
     * Attr1 = b11111111 = Normal, Inner/Outer WB/WA/RA
     * Attr2 = b00000000 = Device-nGnRnE
     */
    val_mair_write(0x00FF44, currentEL);

    /* Setup ttbr0 */
    val_ttbr0_write((uint64_t)tt_l0_base, currentEL);

    if (currentEL == 0x02)
    {
        tcr = ((1ull << 20)    |           /* TBI, top byte ignored. */
               (5ull << 16)    |           /* Physical Address Size - 48 Bits*/
               (TCR_TG0 << 14) |           /* TG0, granule size */
               (3ull << 12)    |           /* SH0, inner shareable. */
               (1ull << 10)    |           /* ORGN0, normal mem, WB RA WA Cacheable */
               (1ull << 8)     |           /* IRGN0, normal mem, WB RA WA Cacheable */
               (64 - MMU_PGT_IAS));        /* T0SZ, input address is 2^40 bytes. */
    }

    val_tcr_write(tcr, currentEL);

    val_print(AVS_PRINT_DEBUG, "       val_setup_mmu: TG0=0x%x\n", TCR_TG0);
    val_print(AVS_PRINT_DEBUG, "       val_setup_mmu: tcr=0x%lx\n", tcr);

    /* Enable MMU */
    val_sctlr_write((1 << 0) |  // M=1 Enable the stage 1 MMU
                    (1 << 2) |  // C=1 Enable data and unified caches
                    (1 << 12) | // I=1 Enable instruction caches
                    val_sctlr_read(currentEL),
                    currentEL);

    val_print(AVS_PRINT_DEBUG, "       val_enable_mmu: successful\n", 0);
    val_print(AVS_PRINT_DEBUG, "       System Control EL2 is %llx", val_sctlr_read(currentEL));

    return AVS_STATUS_PASS;
}
#endif  // TARGET_BM_BOOT
