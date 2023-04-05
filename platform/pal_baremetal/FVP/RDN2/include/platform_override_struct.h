/** @file
 * Copyright (c) 2020-2023, Arm Limited or its affiliates. All rights reserved.
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
  uint64_t smmu_base;
} PLATFORM_OVERRIDE_IOVIRT_PMCG_INFO_BLOCK;

#define MAX_NAMED_COMP_LENGTH 256
typedef struct {
        uint32_t its_count;
        uint32_t identifiers[1];     /* GIC ITS identifier arrary */
}PLATFORM_OVERRIDE_IORT_ITS_GROUP;

typedef struct {
  uint64_t smmu_base;               /* SMMU base to which component is attached, else NULL */
  uint32_t memory_properties;       /* Cache Coherency Attribute */
  char name[MAX_NAMED_COMP_LENGTH]; /* Device object name */
} PLATFORM_OVERRIDE_NC_INFO_BLOCK;

typedef union {
  PLATFORM_OVERRIDE_IOVIRT_RC_INFO_BLOCK rc;
  uint32_t its_count;
} PLATFORM_OVERRIDE_NODE_DATA;

typedef union {
  PLATFORM_OVERRIDE_SMMU_INFO_BLOCK smmu[IOVIRT_SMMUV3_COUNT];
} PLATFORM_OVERRIDE_SMMU_NODE_DATA;

typedef struct {
  PLATFORM_OVERRIDE_IOVIRT_PMCG_INFO_BLOCK pmcg[IOVIRT_PMCG_COUNT];
} PLATFORM_OVERRIDE_PMCG_NODE_DATA;

typedef struct {
  PLATFORM_OVERRIDE_NC_INFO_BLOCK named[IOVIRT_NAMED_COMPONENT_COUNT];
} PLATFORM_OVERRIDE_NAMED_NODE_DATA;

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


typedef struct {
  uint32_t flags;             /* Cache flags */
  uint32_t offset;            /* Cache PPTT structure offset */
  uint32_t next_level_index;  /* Index of next level cache entry in CACHE_INFO_TABLE */
  uint32_t size;              /* Size of the cache in bytes */
  uint32_t cache_id;          /* Unique, non-zero identifier for this cache */
  uint32_t is_private;        /* Field indicate whether cache is private */
  uint8_t  cache_type;        /* Cache type */
} PLATFORM_OVERRIDE_CACHE_INFO_ENTRY;

typedef struct {
  uint32_t num_of_cache;                             /* Total of number of cache info entries */
  PLATFORM_OVERRIDE_CACHE_INFO_ENTRY cache_info[];  /* Array of cache info entries */
} PLATFORM_OVERRIDE_CACHE_INFO_TABLE;

typedef struct {
  uint32_t cache_id[MAX_L1_CACHE_RES];
} PLATFORM_OVERRIDE_PPTT_INFO;

typedef struct {
  PLATFORM_OVERRIDE_PPTT_INFO pptt_info[PLATFORM_OVERRIDE_PE_CNT];
} PLATFORM_OVERRIDE_PPTT_INFO_TABLE;

typedef struct {
  uint32_t   prox_domain;      /* Proximity domain*/
  uint32_t   proc_uid;         /* ACPI Processor UID */
  uint32_t   flags;            /* Flags*/
  uint32_t   clk_domain;       /* Clock Domain*/
} PLATFORM_OVERRIDE_SRAT_GICC_AFF_ENTRY;

typedef struct {
  uint32_t   prox_domain;     /* Proximity domain */
  uint32_t   flags;           /* flags */
  uint64_t   addr_base;       /* mem range address base */
  uint64_t   addr_len;        /* mem range address len */
} PLATFORM_OVERRIDE_SRAT_MEM_AFF_ENTRY;

typedef struct {
  PLATFORM_OVERRIDE_SRAT_MEM_AFF_ENTRY mem_aff[PLATFORM_OVERRIDE_MEM_AFF_CNT];
  PLATFORM_OVERRIDE_SRAT_GICC_AFF_ENTRY gicc_aff[PLATFORM_OVERRIDE_GICC_AFF_CNT];
} PLATFORM_OVERRIDE_SRAT_NODE_INFO_TABLE;

typedef struct {
  uint32_t mem_prox_domain;             /* Proximity domain of the memory region*/
  uint64_t max_write_bw;                    /* Maximum write bandwidth */
  uint64_t max_read_bw;                     /* Maximum read bandwidth */
} PLATFORM_OVERRIDE_HMAT_MEM_ENTRY;

typedef struct {
  PLATFORM_OVERRIDE_HMAT_MEM_ENTRY bw_mem_info[PLATFORM_OVERRIDE_HMAT_MEM_ENTRIES];
} PLATFORM_OVERRIDE_HMAT_MEM_TABLE;

typedef struct {
  uint32_t type;
  uint32_t data_type;             /* Proximity domain of the memory region*/
  uint32_t flags;                    /* Maximum write bandwidth */
  uint64_t entry_base_unit;                     /* Maximum read bandwidth */
} PLATFORM_OVERRIDE_HMAT_BW_ENTRY;

typedef struct {
  uint32_t num_of_prox_domain;      /* Number of Memory Proximity Domains */
  PLATFORM_OVERRIDE_HMAT_BW_ENTRY bw_info[];            /* Array of bandwidth info based on proximity domain */
} PLATFORM_OVERRIDE_HMAT_INFO_TABLE;

typedef struct {
  uint8_t  type;                /* The component that this PMU block is associated with */
  uint64_t primary_instance;    /* Primary node instance, specific to the PMU type */
  uint32_t secondary_instance;  /* Secondary node instance, specific to the PMU type */
  uint8_t  dual_page_extension; /* Support of the dual-page mode */
  uint64_t base0;               /* Base address of Page 0 of the PMU */
  uint64_t base1;               /* Base address of Page 1 of the PMU,
                                     valid only if dual_page_extension is 1 */
} PLATFORM_OVERRIDE_PMU_INFO_BLOCK;

typedef struct {
  uint32_t  pmu_count; /* Total number of PMU info blocks*/
  /* PMU info blocks for each PMU nodes*/
  PLATFORM_OVERRIDE_PMU_INFO_BLOCK  pmu_info[PLATFORM_OVERRIDE_PMU_NODE_CNT];
} PLATFORM_OVERRIDE_PMU_INFO_TABLE;

/* RAS Information */

typedef struct {
  uint32_t  processor_id;
  uint32_t  resource_type;
  uint32_t  flags;
  uint64_t  affinity;
  uint64_t  res_specific_data;  /* Resource Specific Data */
} PLATFORM_OVERRIDE_RAS_NODE_PE_DATA;

typedef struct {
  uint32_t  proximity_domain;
} PLATFORM_OVERRIDE_RAS_NODE_MC_DATA;

typedef struct {
  uint32_t  intf_type;           /* Interface Type */
  uint32_t  flags;
  uint64_t  base_addr;
  uint32_t  start_rec_index;     /* Start Record Index */
  uint32_t  num_err_rec;
  uint64_t  err_rec_implement;
  uint64_t  err_status_reporting;
  uint64_t  addressing_mode;
} PLATFORM_OVERRIDE_RAS_NODE_INTERFACE;

typedef struct {
  uint32_t  type;
  uint32_t  flag;
  uint32_t  gsiv;
  uint32_t  its_grp_id;
} PLATFORM_OVERRIDE_RAS_NODE_INTERRUPT;

typedef struct {
  PLATFORM_OVERRIDE_RAS_NODE_INTERRUPT intr_info[RAS_MAX_NUM_NODES][RAS_MAX_INTR_TYPE];
} PLATFORM_OVERRIDE_RAS_NODE_INTERRUPT_INFO;

typedef struct {
  PLATFORM_OVERRIDE_RAS_NODE_INTERFACE intf_info[RAS_MAX_NUM_NODES];
} PLATFORM_OVERRIDE_RAS_NODE_INTERFACE_INFO;

typedef union {
  PLATFORM_OVERRIDE_RAS_NODE_PE_DATA pe;
  PLATFORM_OVERRIDE_RAS_NODE_MC_DATA mc;
} PLATFORM_OVERRIDE_RAS_NODE_DATA;

typedef struct {
  PLATFORM_OVERRIDE_RAS_NODE_DATA node_data[RAS_MAX_NUM_NODES];
} PLATFORM_OVERRIDE_RAS_NODE_DATA_INFO;

typedef struct {
  uint32_t  type;
  uint32_t  proximity_domain;      /* Proximity domain of the memory */
  uint32_t  patrol_scrub_support;  /* Patrol srub support flag */
} PLATFORM_OVERRIDE_RAS2_BLOCK;

typedef struct {
  uint32_t num_all_block;        /* Number of RAS2 feature blocks */
  uint32_t num_of_mem_block;     /* Number of memory feature blocks */
  PLATFORM_OVERRIDE_RAS2_BLOCK blocks[RAS2_MAX_NUM_BLOCKS];
} PLATFORM_OVERRIDE_RAS2_INFO_TABLE;

typedef struct {
    uint8_t    ris_index;
    uint8_t    locator_type;  /* Identifies location of this resource */
    uint64_t   descriptor1;   /* Primary acpi description of location */
    uint32_t   descriptor2;   /* Secondary acpi description of location */
} PLATFORM_OVERRIDE_MPAM_RESOURCE_NODE;

typedef struct {
    uint64_t    msc_base_addr; /* base addr of mem-map MSC reg */
    uint32_t    msc_addr_len;  /*  MSC mem map size */
    uint32_t    max_nrdy;      /* max time in microseconds that MSC not ready
                                after config change */
    uint32_t    rsrc_count;    /* number of resource nodes */
    /* Details of resource node*/
    PLATFORM_OVERRIDE_MPAM_RESOURCE_NODE rsrc_node[MPAM_MAX_RSRC_NODE];
} PLATFORM_OVERRIDE_MPAM_MSC_NODE;

typedef struct {
    uint32_t          msc_count;  /* Number of MSC node */
    PLATFORM_OVERRIDE_MPAM_MSC_NODE   msc_node[MPAM_MAX_MSC_NODE]; /* Details of MSC node */
} PLATFORM_OVERRIDE_MPAM_INFO_TABLE;

