/** @file
 * Copyright (c) 2016-2024, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __PAL_UEFI_H__
#define __PAL_UEFI_H__

/* include ACPI specification headers */
#include "Include/Guid/Acpi.h"
#include <Protocol/AcpiTable.h>
#include "Include/IndustryStandard/Acpi.h"

extern VOID* g_sbsa_log_file_handle;
extern UINT32 g_print_level;
extern UINT32 g_print_mmio;
extern UINT32 g_curr_module;
extern UINT32 g_enable_module;
extern UINT32 g_pcie_p2p;
extern UINT32 g_pcie_cache_present;

#define AVS_PRINT_ERR   5      /* Only Errors. use this to de-clutter the terminal and focus only on specifics */
#define AVS_PRINT_WARN  4      /* Only warnings & errors. use this to de-clutter the terminal and focus only on specifics */
#define AVS_PRINT_TEST  3      /* Test description and result descriptions. THIS is DEFAULT */
#define AVS_PRINT_DEBUG 2      /* For Debug statements. contains register dumps etc */
#define AVS_PRINT_INFO  1      /* Print all statements. Do not use unless really needed */

#define PCIE_SUCCESS            0x00000000  /* Operation completed successfully */
#define PCIE_NO_MAPPING         0x10000001  /* A mapping to a Function does not exist */
#define PCIE_CAP_NOT_FOUND      0x10000010  /* The specified capability was not found */
#define PCIE_UNKNOWN_RESPONSE   0xFFFFFFFF  /* Function not found or UR response from completer */

#define NOT_IMPLEMENTED         0x4B1D  /* Feature or API by default unimplemented */
#define MEM_OFFSET_SMALL        0x10    /* Memory Offset from BAR base value that can be accesed*/

#define TYPE0_MAX_BARS  6
#define TYPE1_MAX_BARS  2

/* BAR registrer masks */
#define BAR_MIT_MASK    0x1
#define BAR_MDT_MASK    0x3
#define BAR_MT_MASK     0x1
#define BAR_BASE_MASK   0xfffffff

/* BAR register shifts */
#define BAR_MIT_SHIFT   0
#define BAR_MDT_SHIFT   1
#define BAR_MT_SHIFT    3
#define BAR_BASE_SHIFT  4

#define PLATFORM_TIMEOUT_MEDIUM 0x1000
#define PLATFORM_TIMEOUT_SMALL  0x10

typedef enum {
  MMIO = 0,
  IO = 1
} BAR_MEM_INDICATOR_TYPE;

typedef enum {
  BITS_32 = 0,
  BITS_64 = 2
} BAR_MEM_DECODE_TYPE;

typedef struct {
  UINT64   Arg0;
  UINT64   Arg1;
  UINT64   Arg2;
  UINT64   Arg3;
  UINT64   Arg4;
  UINT64   Arg5;
  UINT64   Arg6;
  UINT64   Arg7;
} ARM_SMC_ARGS;

#define sbsa_print(verbose, string, ...) if(verbose >= g_print_level) \
                                            Print(string, ##__VA_ARGS__)
UINT64 pal_get_acpi_table_ptr(UINT32 table_signature);

/**
  Conduits for service calls (SMC vs HVC).
**/
#define CONDUIT_SMC       0
#define CONDUIT_HVC       1
#define CONDUIT_UNKNOWN  -1
#define CONDUIT_NONE     -2

typedef struct {
  UINT32 num_of_pe;
}PE_INFO_HDR;

/**
  @brief  structure instance for PE entry
**/
#define DEFAULT_CACHE_IDX 0xFFFFFFFF
#define MAX_L1_CACHE_RES 2 /* Generally PE Level 1 have a data and a instruction cache */

typedef struct {
  UINT32   pe_num;                        /* PE Index */
  UINT32   attr;                          /* PE attributes */
  UINT64   mpidr;                         /* PE MPIDR */
  UINT32   pmu_gsiv;                      /* PMU Interrupt */
  UINT32   gmain_gsiv;                    /* GIC Maintenace Interrupt */
  UINT32   acpi_proc_uid;                 /* ACPI Processor UID */
  UINT32   level_1_res[MAX_L1_CACHE_RES]; /* index of level 1 cache(s) in cache_info_table */
}PE_INFO_ENTRY;

typedef struct {
  PE_INFO_HDR    header;
  PE_INFO_ENTRY  pe_info[];
}PE_INFO_TABLE;

VOID     pal_pe_data_cache_ops_by_va(UINT64 addr, UINT32 type);

#define CLEAN_AND_INVALIDATE  0x1
#define CLEAN                 0x2
#define INVALIDATE            0x3

typedef struct {
  UINT32   gic_version;
  UINT32   num_gicd;
  UINT32   num_gicrd;
  UINT32   num_its;
  UINT32   num_msi_frame;
  UINT32   num_gich;
}GIC_INFO_HDR;

typedef enum {
  ENTRY_TYPE_CPUIF = 0x1000,
  ENTRY_TYPE_GICD,
  ENTRY_TYPE_GICC_GICRD,
  ENTRY_TYPE_GICR_GICRD,
  ENTRY_TYPE_GICITS,
  ENTRY_TYPE_GIC_MSI_FRAME,
  ENTRY_TYPE_GICH
}GIC_INFO_TYPE_e;

/* Interrupt Trigger Type */
typedef enum {
  INTR_TRIGGER_INFO_LEVEL_LOW,
  INTR_TRIGGER_INFO_LEVEL_HIGH,
  INTR_TRIGGER_INFO_EDGE_FALLING,
  INTR_TRIGGER_INFO_EDGE_RISING
}INTR_TRIGGER_INFO_TYPE_e;

/**
  @brief  structure instance for GIC entry
**/
typedef struct {
  UINT32 type;
  UINT64 base;
  UINT32 entry_id;  /* This entry_id is used to tell component ID */
  UINT64 length;  /* This length is only used in case of Re-Distributor Range Address length */
  UINT32 flags;
  UINT32 spi_count;
  UINT32 spi_base;
}GIC_INFO_ENTRY;

/**
  @brief  GIC Information Table
**/
typedef struct {
  GIC_INFO_HDR   header;
  GIC_INFO_ENTRY gic_info[];  ///< Array of Information blocks - instantiated for each GIC type
}GIC_INFO_TABLE;

typedef struct {
  UINT32 s_el1_timer_flag;
  UINT32 ns_el1_timer_flag;
  UINT32 el2_timer_flag;
  UINT32 el2_virt_timer_flag;
  UINT32 s_el1_timer_gsiv;
  UINT32 ns_el1_timer_gsiv;
  UINT32 el2_timer_gsiv;
  UINT32 virtual_timer_flag;
  UINT32 virtual_timer_gsiv;
  UINT32 el2_virt_timer_gsiv;
  UINT32 num_platform_timer;
  UINT32 num_watchdog;
  UINT32 sys_timer_status;
}TIMER_INFO_HDR;

#define TIMER_TYPE_SYS_TIMER 0x2001

/**
  @brief  structure instance for TIMER entry
**/
typedef struct {
  UINT32 type;
  UINT32 timer_count;
  UINT64 block_cntl_base;
  UINT8  frame_num[8];
  UINT64 GtCntBase[8];
  UINT64 GtCntEl0Base[8];
  UINT32 gsiv[8];
  UINT32 virt_gsiv[8];
  UINT32 flags[8];
}TIMER_INFO_GTBLOCK;

typedef struct {
  TIMER_INFO_HDR     header;
  TIMER_INFO_GTBLOCK gt_info[];
}TIMER_INFO_TABLE;

/**
  @brief  Watchdog Info header - Summary of Watchdog subsytem
**/
typedef struct {
  UINT32 num_wd;  ///< number of Watchdogs present in the system
}WD_INFO_HDR;

/**
  @brief  structure instance for Watchdog entry
**/
typedef struct {
  UINT64 wd_ctrl_base;     ///< Watchdog Control Register Frame
  UINT64 wd_refresh_base;  ///< Watchdog Refresh Register Frame
  UINT32 wd_gsiv;          ///< Watchdog Interrupt ID
  UINT32 wd_flags;
}WD_INFO_BLOCK;

/**
  @brief Watchdog Info Table
**/
typedef struct {
  WD_INFO_HDR    header;
  WD_INFO_BLOCK  wd_info[];  ///< Array of Information blocks - instantiated for each WD Controller
}WD_INFO_TABLE;

/**
  @brief PCI Express Info Table
**/
typedef struct {
  UINT64   ecam_base;     ///< ECAM Base address
  UINT32   segment_num;   ///< Segment number of this ECAM
  UINT32   start_bus_num; ///< Start Bus number for this ecam space
  UINT32   end_bus_num;   ///< Last Bus number
}PCIE_INFO_BLOCK;

typedef struct {
  UINT32          num_entries;
  PCIE_INFO_BLOCK block[];
}PCIE_INFO_TABLE;

typedef enum {
  NON_PREFETCH_MEMORY = 0x0,
  PREFETCH_MEMORY = 0x1
} PCIE_MEM_TYPE_INFO_e;

VOID *pal_pci_bdf_to_dev(UINT32 bdf);
VOID pal_pci_read_config_byte(UINT32 bdf, UINT8 offset, UINT8 *data);

/**
  @brief  Instance of SMMU INFO block
**/
typedef struct {
  UINT32 arch_major_rev;  ///< Version 1 or 2 or 3
  UINT64 base;              ///< SMMU Controller base address
}SMMU_INFO_BLOCK;

typedef struct {
  UINT32 segment;
  UINT32 ats_attr;
  UINT32 cca;             //Cache Coherency Attribute
  UINT64 smmu_base;
}IOVIRT_RC_INFO_BLOCK;

typedef struct {
  UINT64 base;
  UINT32 overflow_gsiv;
  UINT32 node_ref;       /* offest to the IORT node in IORT ACPI table*/
  UINT64 smmu_base;      /* SMMU base to which component is attached, else NULL */
}IOVIRT_PMCG_INFO_BLOCK;

typedef enum {
  IOVIRT_NODE_ITS_GROUP = 0x00,
  IOVIRT_NODE_NAMED_COMPONENT = 0x01,
  IOVIRT_NODE_PCI_ROOT_COMPLEX = 0x02,
  IOVIRT_NODE_SMMU = 0x03,
  IOVIRT_NODE_SMMU_V3 = 0x04,
  IOVIRT_NODE_PMCG = 0x05
}IOVIRT_NODE_TYPE;

typedef enum {
  IOVIRT_FLAG_DEVID_OVERLAP_SHIFT,
  IOVIRT_FLAG_STRID_OVERLAP_SHIFT,
  IOVIRT_FLAG_SMMU_CTX_INT_SHIFT,
}IOVIRT_FLAG_SHIFT;

typedef struct {
  UINT32 input_base;
  UINT32 id_count;
  UINT32 output_base;
  UINT32 output_ref;     /* output ref captured here is offset to iovirt block in
                            IOVIRT info table not IORT ACPI table in memory */
}ID_MAP;

typedef union {
  UINT32 id[4];
  ID_MAP map;
}NODE_DATA_MAP;

#define MAX_NAMED_COMP_LENGTH 256

typedef struct {
  UINT64 smmu_base;                     /* SMMU base to which component is attached, else NULL */
  UINT32 cca;                           /* Cache Coherency Attribute */
  CHAR8 name[MAX_NAMED_COMP_LENGTH];    /* Device object name */
} IOVIRT_NAMED_COMP_INFO_BLOCK;

typedef union {
  IOVIRT_NAMED_COMP_INFO_BLOCK named_comp;
  IOVIRT_RC_INFO_BLOCK rc;
  IOVIRT_PMCG_INFO_BLOCK pmcg;
  UINT32 its_count;
  SMMU_INFO_BLOCK smmu;
}NODE_DATA;

typedef struct {
  UINT32 type;
  UINT32 num_data_map;
  NODE_DATA data;
  UINT32 flags;
  NODE_DATA_MAP data_map[];
}IOVIRT_BLOCK;

typedef struct {
  UINT32 num_blocks;
  UINT32 num_smmus;
  UINT32 num_pci_rcs;
  UINT32 num_named_components;
  UINT32 num_its_groups;
  UINT32 num_pmcgs;
  IOVIRT_BLOCK blocks[];
}IOVIRT_INFO_TABLE;

#define IOVIRT_NEXT_BLOCK(b) (IOVIRT_BLOCK *)((UINT8*)(&b->data_map[0]) + b->num_data_map * sizeof(NODE_DATA_MAP))
#define IOVIRT_CCA_MASK ~((UINT32)0)

/**
  @brief SMMU Info Table
**/
typedef struct {
  UINT32          smmu_num_ctrl;       ///< Number of SMMU Controllers in the system
  SMMU_INFO_BLOCK smmu_block[]; ///< Array of Information blocks - instantiated for each SMMU Controller
}SMMU_INFO_TABLE;

typedef struct {
  UINT32    num_usb;   ///< Number of USB  Controllers
  UINT32    num_sata;  ///< Number of SATA Controllers
  UINT32    num_uart;  ///< Number of UART Controllers
  UINT32    num_all;   ///< Number of all PCI Controllers
}PERIPHERAL_INFO_HDR;

typedef enum {
  PERIPHERAL_TYPE_USB = 0x2000,
  PERIPHERAL_TYPE_SATA,
  PERIPHERAL_TYPE_UART,
  PERIPHERAL_TYPE_OTHER,
  PERIPHERAL_TYPE_NONE
}PER_INFO_TYPE_e;

/**
  @brief  Instance of peripheral info
**/
typedef struct {
  PER_INFO_TYPE_e  type;  ///< PER_INFO_TYPE
  UINT32         bdf;   ///< Bus Device Function
  UINT64         base0; ///< Base Address of the controller
  UINT64         base1; ///< Base Address of the controller
  UINT32         irq;   ///< IRQ to install an ISR
  UINT32         flags;
  UINT32         msi;   ///< MSI Enabled
  UINT32         msix;  ///< MSIX Enabled
  UINT32         max_pasids;
}PERIPHERAL_INFO_BLOCK;

/**
  @brief Peripheral Info Structure
**/
typedef struct {
  PERIPHERAL_INFO_HDR     header;
  PERIPHERAL_INFO_BLOCK   info[]; ///< Array of Information blocks - instantiated for each peripheral
}PERIPHERAL_INFO_TABLE;

/**
  @brief MSI(X) controllers info structure
**/

typedef struct {
  UINT32         vector_upper_addr; ///< Bus Device Function
  UINT32         vector_lower_addr; ///< Base Address of the controller
  UINT32         vector_data;       ///< Base Address of the controller
  UINT32         vector_control;    ///< IRQ to install an ISR
  UINT64         vector_irq_base;   ///< Base IRQ for the vectors in the block
  UINT32         vector_n_irqs;     ///< Number of irq vectors in the block
  UINT32         vector_mapped_irq_base; ///< Mapped IRQ number base for this MSI
}PERIPHERAL_VECTOR_BLOCK;

typedef struct PERIPHERAL_VECTOR_LIST_STRUCT
{
  PERIPHERAL_VECTOR_BLOCK vector;
  struct PERIPHERAL_VECTOR_LIST_STRUCT *next;
}PERIPHERAL_VECTOR_LIST;

UINT32 pal_get_msi_vectors (UINT32 seg, UINT32 bus, UINT32 dev, UINT32 fn, PERIPHERAL_VECTOR_LIST **mvector);

#define LEGACY_PCI_IRQ_CNT 4  // Legacy PCI IRQ A, B, C. and D
#define MAX_IRQ_CNT 0xFFFF    // This value is arbitrary and may have to be adjusted

typedef struct {
  UINT32  irq_list[MAX_IRQ_CNT];
  UINT32  irq_count;
} PERIFERAL_IRQ_LIST;

typedef struct {
  PERIFERAL_IRQ_LIST  legacy_irq_map[LEGACY_PCI_IRQ_CNT];
} PERIPHERAL_IRQ_MAP;

UINT32 pal_pcie_get_root_port_bdf(UINT32 *seg, UINT32 *bus, UINT32 *dev, UINT32 *func);
UINT32 pal_pcie_max_pasid_bits(UINT32 bdf);

/* Memory INFO table */

#define MEM_MAP_SUCCESS  0x0
#define MEM_MAP_NO_MEM   0x1
#define MEM_MAP_FAILURE  0x2
#define MEM_INFO_TBL_MAX_ENTRY  500 /* Maximum entries to be added in Mem info table*/

typedef enum {
  MEMORY_TYPE_DEVICE = 0x1000,
  MEMORY_TYPE_NORMAL,
  MEMORY_TYPE_RESERVED,
  MEMORY_TYPE_NOT_POPULATED,
  MEMORY_TYPE_PERSISTENT,
  MEMORY_TYPE_LAST_ENTRY
}MEM_INFO_TYPE_e;


typedef struct {
  MEM_INFO_TYPE_e type;
  UINT64        phy_addr;
  UINT64        virt_addr;
  UINT64        size;
  UINT64        flags;  //To Indicate Cacheablility etc..
}MEM_INFO_BLOCK;


typedef struct {
  UINT64  dram_base;
  UINT64  dram_size;
  MEM_INFO_BLOCK  info[];
} MEMORY_INFO_TABLE;


VOID  pal_memory_create_info_table(MEMORY_INFO_TABLE *memoryInfoTable);

VOID    *pal_mem_alloc(UINT32 size);
VOID    *pal_mem_calloc(UINT32 num, UINT32 size);
VOID    *pal_mem_alloc_cacheable(UINT32 bdf, UINT32 size, VOID **pa);
VOID    pal_mem_free_cacheable(UINT32 bdf, UINT32 size, VOID *va, VOID *pa);
VOID    *pal_mem_virt_to_phys(VOID *va);
VOID    *pal_mem_phys_to_virt(UINT64 pa);
UINT64  pal_memory_get_unpopulated_addr(UINT64 *addr, UINT32 instance);

VOID    pal_mem_free(VOID *buffer);
UINT32  pal_pe_get_num();

/**
  @brief  Instance of system pmu info
**/
typedef struct {
    UINT8  type;                /* The component that this PMU block is associated with*/
    UINT64 primary_instance;    /* Primary node instance, specific to the PMU type*/
    UINT32 secondary_instance;  /* Secondary node instance, specific to the PMU type*/
    UINT8  dual_page_extension; /* Support of the dual-page mode*/
    UINT64 base0;               /* Base address of Page 0 of the PMU*/
    UINT64 base1;               /* Base address of Page 1 of the PMU,
                                   valid only if dual_page_extension is 1*/
} PMU_INFO_BLOCK;

typedef struct {
    UINT32  pmu_count;          /* Total number of PMU info blocks*/
    PMU_INFO_BLOCK  info[];     /* PMU info blocks for each PMU nodes*/
} PMU_INFO_TABLE;

/*
 * @brief Mpam Resource Node
 */
typedef struct {
    UINT8    ris_index;
    UINT8    locator_type;  /* Identifies location of this resource */
    UINT64   descriptor1;   /* Primary acpi description of location */
    UINT32   descriptor2;   /* Secondary acpi description of location */
} MPAM_RESOURCE_NODE;

/*
 * @brief Mpam MSC Node
 */
typedef struct {
    UINT64    msc_base_addr; /* base addr of mem-map MSC reg */
    UINT32    msc_addr_len;  /*  MSC mem map size */
    UINT32    max_nrdy;      /* max time in microseconds that MSC not ready
                                after config change */
    UINT32    rsrc_count;    /* number of resource nodes */
    MPAM_RESOURCE_NODE rsrc_node[]; /* Details of resource node */
} MPAM_MSC_NODE;

/*
 * @brief Mpam info table
 */

#define MPAM_NEXT_MSC(msc_entry) \
        (MPAM_MSC_NODE *)((UINT8 *)(&msc_entry->rsrc_node[0]) \
        + msc_entry->rsrc_count * sizeof(MPAM_RESOURCE_NODE))

typedef struct {
    UINT32          msc_count;  /* Number of MSC node */
    MPAM_MSC_NODE   msc_node[]; /* Details of MSC node */
} MPAM_INFO_TABLE;

/**
  @brief  SRAT node type
**/

typedef enum {
  SRAT_NODE_MEM_AFF  = 0x01,
  SRAT_NODE_GICC_AFF = 0x03
} SRAT_NODE_TYPE_e;

/**
  @brief  SRAT GICC Affinity Structure
**/

typedef struct {
  UINT32   prox_domain;      /* Proximity domain*/
  UINT32   proc_uid;         /* ACPI Processor UID */
  UINT32   flags;            /* Flags*/
  UINT32   clk_domain;       /* Clock Domain*/
} SRAT_GICC_AFF_ENTRY;

/**
  @brief  SRAT Memory Affinity Structure
**/

typedef struct {
  UINT32   prox_domain;     /* Proximity domain */
  UINT32   flags;           /* flags */
  UINT64   addr_base;       /* mem range address base */
  UINT64   addr_len;        /* mem range address len */
} SRAT_MEM_AFF_ENTRY;

typedef union {
  SRAT_MEM_AFF_ENTRY mem_aff;
  SRAT_GICC_AFF_ENTRY gicc_aff;
} SRAT_NODE_INFO;

typedef struct {
  UINT32 node_type;         /* Node type*/
  SRAT_NODE_INFO node_data;
} SRAT_INFO_ENTRY;

typedef struct {
  UINT32 num_of_srat_entries;
  UINT32 num_of_mem_ranges;
  SRAT_INFO_ENTRY  srat_info[];
} SRAT_INFO_TABLE;

/* SRAT node structure header. Can be removed after it is defined in EDKII*/
typedef struct {
  UINT8    Type;
  UINT8    Length;
} EFI_ACPI_6_4_SRAT_STRUCTURE_HEADER;


/* Cache info table structures and APIs */

#define CACHE_TYPE_SHARED  0x0
#define CACHE_TYPE_PRIVATE 0x1
#define CACHE_INVALID_NEXT_LVL_IDX 0xFFFFFFFF
#define CACHE_INVALID_IDX 0xFFFFFFFF

/*only the fields and flags required by ACS are parsed from ACPI PPTT table*/
/*Cache flags indicate validity of cache info provided by PPTT Table*/
typedef struct {
  UINT32 size_property_valid;
  UINT32 cache_type_valid;
  UINT32 cache_id_valid;
} CACHE_FLAGS;

/* Since most of platform doesn't support cache id field (ACPI 6.4+), ACS uses PPTT offset as key
   to uniquely identify a cache, In future once platforms align with ACPI 6.4+ my_offset member
   might be removed from cache entry*/
typedef struct {
  CACHE_FLAGS flags;        /* Cache flags */
  UINT32 my_offset;         /* Cache PPTT structure offset */
  UINT32 next_level_index;  /* Index of next level cache entry in CACHE_INFO_TABLE */
  UINT32 size;              /* Size of the cache in bytes */
  UINT32 cache_id;          /* Unique, non-zero identifier for this cache */
  UINT32 is_private;        /* Field indicate whether cache is private */
  UINT8  cache_type;        /* Cache type */
} CACHE_INFO_ENTRY;

typedef struct {
  UINT32 num_of_cache;            /* Total of number of cache info entries */
  CACHE_INFO_ENTRY cache_info[];  /* Array of cache info entries */
} CACHE_INFO_TABLE;

/* RAS Information */

typedef enum {
  NODE_TYPE_PE = 0x0,
  NODE_TYPE_MC = 0x1,
  NODE_TYPE_SMMU = 0x2,
  NODE_TYPE_VDR = 0x3,
  NODE_TYPE_GIC = 0x4,
  NODE_TYPE_LAST_ENTRY
} RAS_NODE_TYPE_e;

typedef enum {
  RAS_INTF_TYPE_SYS_REG,     /* System register RAS node interface type */
  RAS_INTF_TYPE_MMIO         /* MMIO RAS node interface type */
} RAS_NODE_INTF_TYPE;
typedef struct {
  UINT32  processor_id;
  UINT32  resource_type;
  UINT32  flags;
  UINT64  affinity;
  UINT64  res_specific_data;  /* Resource Specific Data */
} RAS_NODE_PE_DATA;

typedef struct {
  UINT32  proximity_domain;
} RAS_NODE_MC_DATA;

typedef struct {
  RAS_NODE_INTF_TYPE  intf_type;   /* Interface Type */
  UINT32  flags;
  UINT64  base_addr;               /* Base address to MMIO region, valid for MMIO intf type */
  UINT32  start_rec_index;         /* Start Record Index */
  UINT32  num_err_rec;             /* Number of error records (implemented & unimplemented)*/
  UINT64  err_rec_implement;       /* bitmap of error records implemented */
  UINT64  err_status_reporting;    /* bitmap indicates which error records within this error
                                      node support error status reporting using ERRGSR */
  UINT64  addressing_mode;         /* bitmap based policy for ERR<n>ADDR field of error records */
} RAS_INTERFACE_INFO;

typedef struct {
  UINT32  type;
  UINT32  flag;
  UINT32  gsiv;
  UINT32  its_grp_id;
} RAS_INTERRUPT_INFO;

typedef union {
  RAS_NODE_PE_DATA    pe;
  RAS_NODE_MC_DATA    mc;
} RAS_NODE_DATA;

typedef struct {
  RAS_NODE_TYPE_e     type;             /* Node Type PE/GIC/SMMU */
  UINT16              length;           /* Length of the Node */
  UINT64              num_intr_entries; /* Number of Interrupt Entry */
  RAS_NODE_DATA       node_data;        /* Node Specific Data */
  RAS_INTERFACE_INFO  intf_info;        /* Node Interface Info */
  RAS_INTERRUPT_INFO  intr_info[2];     /* Node Interrupt Info */
} RAS_NODE_INFO;

typedef struct {
  UINT32  num_nodes;      /* Number of total RAS Nodes */
  UINT32  num_pe_node;    /* Number of PE RAS Nodes */
  UINT32  num_mc_node;    /* Number of Memory Controller Nodes */
  RAS_NODE_INFO  node[];  /* Array of RAS nodes */
} RAS_INFO_TABLE;

typedef enum {
    ERR_UC = 0x1,         /* Uncorrectable Error */
    ERR_DE,               /* Deferred Error */
    ERR_CE,               /* Correctable Error */
    ERR_CRITICAL,         /* Critical Error */
    ERR_CONTAINABLE       /* Containable Error */
} RAS_ERROR_TYPE;

typedef struct {
   RAS_ERROR_TYPE ras_error_type;   /* Error Type */
   UINT64 error_pa;                 /* Error Phy Address */
   UINT32 rec_index;                /* Error Record Index */
   UINT32 node_index;               /* Error Node Index in Info table */
   UINT8 is_pfg_check;              /* Pseudo Fault Check or not */
} RAS_ERR_IN_t;

typedef struct {
   UINT32 intr_id;        /* Interrupt ID */
   UINT32 error_record;   /* Error Record Number */
} RAS_ERR_OUT_t;

void pal_ras_create_info_table(RAS_INFO_TABLE *ras_info_table);
UINT32 pal_ras_setup_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param);
UINT32 pal_ras_inject_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param);
void pal_ras_wait_timeout(UINT32 count);
UINT32 pal_ras_check_plat_poison_support(void);


typedef enum {
  RAS2_TYPE_MEMORY = 0   /* RAS2 memory feature type*/
} RAS2_FEAT_TYPE;

typedef struct {
  UINT32  proximity_domain;      /* Proximity domain of the memory */
  UINT32  patrol_scrub_support;  /* Patrol srub support flag */
} RAS2_MEM_INFO;

typedef union {
  RAS2_MEM_INFO mem_feat_info;   /* Memory feature specific info */
} RAS2_BLOCK_INFO;

typedef struct {
  RAS2_FEAT_TYPE type;                     /* RAS2 feature type*/
  RAS2_BLOCK_INFO block_info;     /* RAS2 block info */
} RAS2_BLOCK;

typedef struct {
  UINT32 num_all_block;        /* Number of RAS2 feature blocks */
  UINT32 num_of_mem_block;     /* Number of memory feature blocks */
  RAS2_BLOCK blocks[];
} RAS2_INFO_TABLE;

void pal_ras2_create_info_table(RAS2_INFO_TABLE *ras2_info_table);

/* HMAT info table structures and APIs*/

#define HMAT_MEM_HIERARCHY_MEMORY   0x00
#define HMAT_DATA_TYPE_ACCESS_BW    0x03
#define HMAT_DATA_TYPE_READ_BW      0x04
#define HMAT_DATA_TYPE_WRITE_BW     0x05
#define HMAT_BW_ENTRY_UNREACHABLE   0xFFFF
#define HMAT_BASE_UNIT_48BIT        0xFFFFFFFFFFFFULL
typedef struct {
  UINT32 mem_prox_domain;             /* Proximity domain of the memory region*/
  UINT64 write_bw;                    /* Maximum write bandwidth */
  UINT64 read_bw;                     /* Maximum read bandwidth */
} HMAT_BW_ENTRY;

typedef struct {
  UINT32 num_of_mem_prox_domain;      /* Number of Memory Proximity Domains */
  HMAT_BW_ENTRY bw_info[];            /* Array of bandwidth info based on proximity domain */
} HMAT_INFO_TABLE;

VOID pal_hmat_create_info_table(HMAT_INFO_TABLE *HmatTable);

#endif
