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

#ifndef __SBSA_AVS_PGT_H__
#define __SBSA_AVS_PGT_H__

#define PGT_STAGE1 1
#define PGT_STAGE2 2

#define PGT_ENTRY_TABLE_MASK (0x1 << 1)
#define PGT_ENTRY_VALID_MASK  0x1ULL
#define PGT_ENTRY_PAGE_MASK  (0x1 << 1)
#define PGT_ENTRY_BLOCK_MASK (0x0 << 1)

#define PGT_ENTRY_TYPE_MASK   0x3ULL
#define PGT_ENTRY_TYPE_TABLE  0x3
#define PGT_ENTRY_TYPE_BLOCK  0x1
#define PGT_ENTRY_TYPE_PAGE   0x3

#define IS_PGT_ENTRY_PAGE(val)  ((val & PGT_ENTRY_TYPE_MASK) == PGT_ENTRY_TYPE_PAGE)
#define IS_PGT_ENTRY_BLOCK(val) ((val & PGT_ENTRY_TYPE_MASK) == PGT_ENTRY_TYPE_BLOCK)
#define IS_PGT_ENTRY_TABLE(val) ((val & PGT_ENTRY_TYPE_MASK) == PGT_ENTRY_TYPE_TABLE)
#define IS_PGT_ENTRY_INVALID(val) !(val & PGT_ENTRY_VALID_MASK)

#define PGT_DESC_SIZE 8
#define PGT_DESC_ATTR_UPPER_MASK ((0x1ull << 12) - 1) << 52
#define PGT_DESC_ATTR_LOWER_MASK ((0x1ull << 10) - 1) << 2
#define PGT_DESC_ATTRIBUTES_MASK (PGT_DESC_ATTR_UPPER_MASK | PGT_DESC_ATTR_LOWER_MASK)
#define PGT_DESC_ATTRIBUTES(val) (val & PGT_DESC_ATTRIBUTES_MASK)

#define PGT_STAGE1_AP_RO (0x3ull << 6)
#define PGT_STAGE1_AP_RW (0x1ull << 6)
#define PGT_STAGE2_AP_RO (0x1ull << 6)
#define PGT_STAGE2_AP_RW (0x3ull << 6)

#define PGT_LEVEL_MAX 4

uint32_t val_pgt_create(memory_region_descriptor_t *mem_desc, pgt_descriptor_t *pgt_desc);
void val_pgt_destroy(pgt_descriptor_t pgt_desc);
uint64_t val_pgt_get_attributes(pgt_descriptor_t pgt_desc, uint64_t virtual_address, uint64_t *attributes);

#endif
