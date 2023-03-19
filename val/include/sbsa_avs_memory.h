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

#ifndef __SBSA_AVS_MEMORY_H__
#define __SBSA_AVS_MEMORY_H__

#define MEM_MAP_SUCCESS  0x0
#define MEM_MAP_NO_MEM   0x1
#define MEM_MAP_FAILURE  0x2
#define MEM_SIZE_64KB    65536

addr_t val_memory_ioremap(void *addr, uint32_t size, uint64_t attr);

void val_memory_unmap(void *ptr);
void *val_memory_alloc(uint32_t size);
void *val_memory_calloc(uint32_t num, uint32_t size);
void *val_memory_alloc_cacheable(uint32_t bdf, uint32_t size, void **pa);
void val_memory_free(void *addr);
int val_memory_compare(void *src, void *dest, uint32_t len);
void val_memory_set(void *buf, uint32_t size, uint8_t value);
void val_memory_free_cacheable(uint32_t bdf, uint32_t size, void *va, void *pa);
void *val_memory_virt_to_phys(void *va);
void *val_memory_phys_to_virt(uint64_t pa);
uint32_t val_memory_page_size(void);
void *val_memory_alloc_pages(uint32_t num_pages);
void val_memory_free_pages(void *page_base, uint32_t num_pages);
void *val_aligned_alloc( uint32_t alignment, uint32_t size );
void val_memory_free_aligned(void *addr);
uint32_t val_memory_check_for_persistent_mem(void);

uint32_t m001_entry(uint32_t num_pe);

#endif
