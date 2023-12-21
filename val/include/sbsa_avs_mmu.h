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

extern uint64_t tt_l0_base[];

#endif /* __SBSA_AVS_MMU_H__ */
