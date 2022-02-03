/** @file
 * Copyright (c) 2020-2022, Arm Limited or its affiliates. All rights reserved.
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

#include <stdio.h>
#include <stdint.h>
#include "platform_override_fvp.h"

typedef struct {
  uint32_t gic_version;
  uint32_t num_gicc;
  uint32_t num_gicd;
  uint32_t num_gicrd;
  uint32_t num_gicits;
  uint32_t num_gich;
  uint32_t num_msiframes;
  uint32_t gicc_type;
  uint32_t gicd_type;
  uint32_t gicrd_type;
  uint32_t gicrd_length;
  uint32_t gicits_type;
  uint64_t gicc_base[PLATFORM_OVERRIDE_GICC_COUNT];
  uint64_t gicd_base[PLATFORM_OVERRIDE_GICD_COUNT];
  uint64_t gicrd_base[PLATFORM_OVERRIDE_GICRD_COUNT];
  uint64_t gicits_base[PLATFORM_OVERRIDE_GICITS_COUNT];
  uint64_t gicits_id[PLATFORM_OVERRIDE_GICITS_COUNT];
  uint64_t gich_base[PLATFORM_OVERRIDE_GICH_COUNT];
  uint64_t gicmsiframe_base[PLATFORM_OVERRIDE_GICMSIFRAME_COUNT];
  uint64_t gicmsiframe_id[PLATFORM_OVERRIDE_GICMSIFRAME_COUNT];
  uint32_t gicmsiframe_flags[PLATFORM_OVERRIDE_GICMSIFRAME_COUNT];
  uint32_t gicmsiframe_spi_count[PLATFORM_OVERRIDE_GICMSIFRAME_COUNT];
  uint32_t gicmsiframe_spi_base[PLATFORM_OVERRIDE_GICMSIFRAME_COUNT];
} PLATFORM_OVERRIDE_GIC_INFO_TABLE;

typedef struct {
  uint32_t s_el1_timer_flags;
  uint32_t ns_el1_timer_flags;
  uint32_t el2_timer_flags;
  uint32_t s_el1_timer_gsiv;
  uint32_t ns_el1_timer_gsiv;
  uint32_t el2_timer_gsiv;
  uint32_t virtual_timer_flags;
  uint32_t virtual_timer_gsiv;
  uint32_t el2_virt_timer_gsiv;
  uint32_t num_platform_timer;
} PLATFORM_OVERRIDE_TIMER_INFO_HDR;

typedef struct {
  uint32_t type;
  uint32_t timer_count;
  uint64_t block_cntl_base;
  uint64_t GtCntBase[PLATFORM_OVERRIDE_TIMER_COUNT];
  uint64_t GtCntEl0Base[PLATFORM_OVERRIDE_TIMER_COUNT];
  uint32_t gsiv[PLATFORM_OVERRIDE_TIMER_COUNT];
  uint32_t virt_gsiv[PLATFORM_OVERRIDE_TIMER_COUNT];
  uint32_t flags[PLATFORM_OVERRIDE_TIMER_COUNT];
} PLATFORM_OVERRIDE_TIMER_INFO_GTBLOCK;

typedef struct {
  PLATFORM_OVERRIDE_TIMER_INFO_HDR     header;
  PLATFORM_OVERRIDE_TIMER_INFO_GTBLOCK gt_info;
} PLATFORM_OVERRIDE_TIMER_INFO_TABLE;

typedef struct {
  uint32_t arch_major_rev;  ///< Version 1 or 2 or 3
  uint64_t base;              ///< SMMU Controller base address
  uint64_t context_interrupt_offset;
  uint32_t context_interrupt_count;
} PLATFORM_OVERRIDE_SMMU_INFO_BLOCK;

typedef struct {
  uint32_t segment;
  uint32_t ats_attr;
  uint32_t cca;             //Cache Coherency Attribute
  uint64_t smmu_base;
} PLATFORM_OVERRIDE_IOVIRT_RC_INFO_BLOCK;

typedef struct {
  uint64_t base;
  uint32_t overflow_gsiv;
  uint32_t node_ref;
} PLATFORM_OVERRIDE_IOVIRT_PMCG_INFO_BLOCK;

#define MAX_NAMED_COMP_LENGTH 256

typedef struct {
  char name[MAX_NAMED_COMP_LENGTH];
  PLATFORM_OVERRIDE_IOVIRT_RC_INFO_BLOCK rc;
  PLATFORM_OVERRIDE_IOVIRT_PMCG_INFO_BLOCK pmcg;
  uint32_t its_count;
  PLATFORM_OVERRIDE_SMMU_INFO_BLOCK smmu;
} PLATFORM_OVERRIDE_NODE_DATA;

typedef struct {
  uint32_t input_base[IOVIRT_MAX_NUM_MAP];
  uint32_t id_count[IOVIRT_MAX_NUM_MAP];
  uint32_t output_base[IOVIRT_MAX_NUM_MAP];
  uint32_t output_ref[IOVIRT_MAX_NUM_MAP];
} PLATFORM_OVERRIDE_NODE_DATA_MAP;

typedef struct {
  uint64_t Address;
  uint32_t node_count;
  uint32_t type[IORT_NODE_COUNT];
  uint32_t num_map[IORT_NODE_COUNT];
  PLATFORM_OVERRIDE_NODE_DATA_MAP map[IORT_NODE_COUNT];
} PLATFORM_OVERRIDE_IOVIRT_INFO_TABLE;

struct ecam_reg_data {
    uint32_t offset;    //Offset into 4096 bytes ecam config reg space
    uint32_t attribute;
    uint32_t value;
};

struct exerciser_data_cfg_space {
    struct ecam_reg_data reg[TEST_REG_COUNT];
};

typedef enum {
    MMIO_PREFETCHABLE = 0x0,
    MMIO_NON_PREFETCHABLE = 0x1
} BAR_MEM_TYPE;

struct exerciser_data_bar_space {
    void *base_addr;
    BAR_MEM_TYPE type;
};

typedef enum {
  MMIO = 0,
  IO = 1
} BAR_MEM_INDICATOR_TYPE;

typedef enum {
  BITS_32 = 0,
  BITS_64 = 2
} BAR_MEM_DECODE_TYPE;

typedef union exerciser_data {
    struct exerciser_data_cfg_space cfg_space;
    struct exerciser_data_bar_space bar_space;
} exerciser_data_t;

typedef enum {
    EXERCISER_DATA_CFG_SPACE = 0x1,
    EXERCISER_DATA_BAR0_SPACE = 0x2,
    EXERCISER_DATA_MMIO_SPACE = 0x3,
} EXERCISER_DATA_TYPE;

typedef enum {
    ACCESS_TYPE_RD = 0x0,
    ACCESS_TYPE_RW = 0x1
} ECAM_REG_ATTRIBUTE;

typedef enum {
    TYPE0 = 0x0,
    TYPE1 = 0x1,
} EXERCISER_CFG_HEADER_TYPE;


typedef enum {
    CFG_READ   = 0x0,
    CFG_WRITE  = 0x1,
} EXERCISER_CFG_TXN_ATTR;


typedef enum {
    TXN_REQ_ID     = 0x0,
    TXN_ADDR_TYPE  = 0x1,
    TXN_REQ_ID_VALID    = 0x2,
} EXERCISER_TXN_ATTR;

typedef enum {
    AT_UNTRANSLATED = 0x0,
    AT_TRANS_REQ    = 0x1,
    AT_TRANSLATED   = 0x2,
    AT_RESERVED     = 0x3
} EXERCISER_TXN_ADDR_TYPE;


typedef enum {
    DEVICE_nGnRnE = 0x0,
    DEVICE_nGnRE  = 0x1,
    DEVICE_nGRE   = 0x2,
    DEVICE_GRE    = 0x3
} ARM_DEVICE_MEM;

typedef enum {
    NORMAL_NC = 0x4,
    NORMAL_WT = 0x5
} ARM_NORMAL_MEM;

typedef struct {
    uint64_t phy_addr;
    uint64_t virt_addr;
    uint64_t size;
    uint64_t type;
} MEMORY_INFO;

typedef struct {
    uint32_t count;
    MEMORY_INFO info[];
} PLATFORM_OVERRIDE_MEMORY_INFO_TABLE;
