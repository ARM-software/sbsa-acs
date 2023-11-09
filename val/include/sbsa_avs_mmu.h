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

#ifndef __SBSA_AVS_MMU_H__
#define __SBSA_AVS_MMU_H__

/* upper and lower mem attribute shift definitions */
#define MEM_ATTR_INDX_SHIFT 2
#define MEM_ATTR_AP_SHIFT   6
#define MEM_ATTR_SH_SHIFT   8
#define MEM_ATTR_AF_SHIFT   10

#define MAX_MMAP_REGION_COUNT 15
#define TCR_TG0     0

#define PGT_IPS     0x2ull

#define ATTR_NORMAL_NONCACHEABLE (0x0ull << 2)
#define ATTR_NORMAL_WB_WA_RA      (0x1ull << 2)
#define ATTR_DEVICE               (0x2ull << 2)
#define ATTR_NORMAL_WB            (0x1ull << 3)

/* Stage 1 Inner and Outer Cacheability attribute encoding without TEX remap */
#define ATTR_S1_NONCACHEABLE  (0x0ull << 2)
#define ATTR_S1_WB_WA_RA       (0x1ull << 2)
#define ATTR_S1_WT_RA          (0x2ull << 2)
#define ATTR_S1_WB_RA          (0x3ull << 2)

/* Stage 2 MemAttr[1:0] encoding for Normal memory */
#define ATTR_S2_INNER_NONCACHEABLE   (0x1ull << 2)
#define ATTR_S2_INNER_WT_CACHEABLE   (0x2ull << 2)
#define ATTR_S2_INNER_WB_CACHEABLE   (0x3ull << 2)

#define ATTR_NS   (0x1ull << 5)
#define ATTR_S    (0x0ull << 5)

#define ATTR_STAGE1_AP_RW (0x1ull << 6)
#define ATTR_STAGE2_AP_RW (0x3ull << 6)
#define ATTR_STAGE2_MASK  (0x3ull << 6 | 0x1ull << 4)
#define ATTR_STAGE2_MASK_RO  (0x1ull << 6 | 0x1ull << 4)

#define ATTR_NON_SHARED     (0x0ull << 8)
#define ATTR_OUTER_SHARED   (0x2ull << 8)
#define ATTR_INNER_SHARED   (0x3ull << 8)

#define ATTR_AF   (0x1ull << 10)
#define ATTR_nG   (0x1ull << 11)

#define ATTR_UXN    (0x1ull << 54)
#define ATTR_PXN    (0x1ull << 53)

#define ATTR_PRIV_RW        (0x0ull << 6)
#define ATTR_PRIV_RO        (0x2ull << 6)
#define ATTR_USER_RW        (0x1ull << 6)
#define ATTR_USER_RO        (0x3ull << 6)

#define ATTR_CODE           (ATTR_S1_WB_WA_RA | ATTR_USER_RO | \
                              ATTR_AF | ATTR_INNER_SHARED)
#define ATTR_RO_DATA        (ATTR_S1_WB_WA_RA | ATTR_USER_RO | \
                              ATTR_UXN | ATTR_PXN | ATTR_AF | \
                              ATTR_INNER_SHARED)
#define ATTR_RW_DATA        (ATTR_S1_WB_WA_RA | \
                              ATTR_USER_RW | ATTR_UXN | ATTR_PXN | ATTR_AF \
                              | ATTR_INNER_SHARED)
#define ATTR_DEVICE_RW      (ATTR_DEVICE | ATTR_USER_RW | ATTR_UXN | \
                              ATTR_PXN | ATTR_AF | ATTR_INNER_SHARED)

#define ATTR_RW_DATA_NC      (ATTR_S1_NONCACHEABLE | \
                              ATTR_USER_RW | ATTR_UXN | ATTR_PXN | ATTR_AF \
                              | ATTR_INNER_SHARED)
/* memory type MAIR register index definitions*/
#define ATTR_DEVICE_nGnRnE (0x0ULL << MEM_ATTR_INDX_SHIFT)

uint32_t val_mmu_check_for_entry(uint64_t base_addr);
uint32_t val_mmu_add_entry(uint64_t base_addr, uint64_t size);
uint32_t val_mmu_update_entry(uint64_t address, uint32_t size);

extern void val_mair_write(uint64_t value, uint64_t el_num);
extern void val_tcr_write(uint64_t value, uint64_t el_num);
extern void val_ttbr0_write(uint64_t value, uint64_t el_num);
extern void val_sctlr_write(uint64_t value, uint64_t el_num);
extern uint64_t val_sctlr_read(uint64_t el_num);
extern uint64_t val_read_current_el(void);
extern void EnableMMU(void);

extern uint64_t tt_l0_base[];
extern uint64_t tt_l1_base[];
extern uint64_t tt_l2_base_1[];
extern uint64_t tt_l2_base_2[];
extern uint64_t tt_l2_base_3[];
extern uint64_t tt_l2_base_4[];
extern uint64_t tt_l2_base_5[];
extern uint64_t tt_l2_base_6[];
extern uint64_t tt_l3_base_1[];
extern uint64_t tt_l3_base_2[];
extern uint64_t tt_l3_base_3[];
extern uint64_t tt_l3_base_4[];
extern uint64_t tt_l3_base_5[];
extern uint64_t tt_l3_base_6[];
extern uint64_t tt_l3_base_7[];
extern uint64_t tt_l3_base_8[];
extern uint64_t tt_l3_base_9[];
extern uint64_t tt_l3_base_10[];

#endif /* __SBSA_AVS_MMU_H__ */
