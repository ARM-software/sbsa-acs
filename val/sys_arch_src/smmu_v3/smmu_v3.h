/*@file
 * Copyright (c) 2020, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SMMU_V3_H__
#define __SMMU_V3_H__

#include "../include/sbsa_avs_val.h"
#include "../include/sbsa_avs_memory.h"
#include "../include/sbsa_avs_iovirt.h"
#include "../include/sbsa_avs_pgt.h"
#include "smmu_reg.h"

static uint64_t inline max(uint64_t x, uint64_t y)
{
    return x > y ? x : y;
}

#define CMDQ_OP_CFGI_STE 0x3
#define CMDQ_OP_CFGI_ALL 0x4
#define CMDQ_OP_TLBI_EL2_ALL 0x20
#define CMDQ_OP_TLBI_NSNH_ALL 0x30
#define CMDQ_OP_CMD_SYNC 0x46

typedef struct {
    uint32_t prod;
    uint32_t cons;
    uint32_t log2nent;
} smmu_queue_t;

typedef struct {
    smmu_queue_t queue;
    void    *base_ptr;
    uint8_t *base;
    uint64_t base_phys;
    uint64_t queue_base;
    uint64_t entry_size;
    uint32_t *prod_reg;
    uint32_t *cons_reg;
} smmu_cmd_queue_t;

typedef struct {
    uint8_t  span;
    void     *l2ptr;
    uint64_t *l2desc64;
    uint64_t l2desc_phys;
} smmu_strtab_l1_desc_t;

typedef struct {
    uint16_t vmid;
    uint64_t vttbr;
    uint64_t vtcr;
} smmu_stage2_config_t;

typedef struct {
    uint16_t    asid;
    uint64_t    ttbr;
    uint64_t    tcr;
    uint64_t    mair;
} smmu_cdtab_ctx_desc_t;

typedef struct {
    void     *l2ptr;
    uint64_t *l2desc64;
    uint64_t l2desc_phys;
} smmu_cdtab_l1_ctx_desc_t;

typedef struct {
    void                           *cdtab_ptr;
    uint64_t                       *cdtab64;
    uint64_t                       cdtab_phys;
    smmu_cdtab_l1_ctx_desc_t       *l1_desc;
    unsigned int                   l1_ent_count;
} smmu_cdtab_config_t;

typedef struct {
    smmu_cdtab_config_t             cdcfg;
    smmu_cdtab_ctx_desc_t        cd;
    uint8_t                         s1fmt;
    uint8_t                         s1cdmax;
} smmu_stage1_config_t;

typedef struct {
    void     *strtab_ptr;
    uint64_t *strtab64;
    uint64_t strtab_phys;
    smmu_strtab_l1_desc_t *l1_desc;
    uint32_t l1_ent_count;
    uint64_t strtab_base;
    uint32_t strtab_base_cfg;
} smmu_strtab_config_t;

typedef struct {
    uint64_t base;
    uint64_t ias;
    uint64_t oas;
    uint32_t ssid_bits;
    uint32_t sid_bits;
    smmu_cmd_queue_t cmdq;
    smmu_strtab_config_t strtab_cfg;
    union {
        struct {
           uint32_t st_level_2lvl:1;
           uint32_t cd2l:1;
           uint32_t hyp:1;
           uint32_t s1p:1;
           uint32_t s2p:1;
        };
        uint32_t bitmap;
    } supported;
} smmu_dev_t;

typedef enum {
    SMMU_STAGE_S1 = 0,
    SMMU_STAGE_S2,
    SMMU_STAGE_BYPASS
} smmu_stage_t;

typedef struct {
#define MAX_PAGE_TABLES_PER_MASTER 8
    smmu_dev_t *smmu;
    smmu_stage_t stage;
    smmu_stage1_config_t stage1_config;
    smmu_stage2_config_t stage2_config;
    uint32_t sid;
    uint32_t ssid;
    uint32_t ssid_bits;
} smmu_master_t;

#endif /*__SMMU_V3_H__ */
