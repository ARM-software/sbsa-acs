/** @file
 * Copyright (c) 2020-2021, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __PAL_COMMON_SUPPORT_H_
#define __PAL_COMMON_SUPPORT_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

extern uint32_t g_print_level;

#define AVS_PRINT_ERR   5      /* Only Errors. use this to de-clutter the terminal and focus only on specifics */
#define AVS_PRINT_WARN  4      /* Only warnings & errors. use this to de-clutter the terminal and focus only on specifics */
#define AVS_PRINT_TEST  3      /* Test description and result descriptions. THIS is DEFAULT */
#define AVS_PRINT_DEBUG 2      /* For Debug statements. contains register dumps etc */
#define AVS_PRINT_INFO  1      /* Print all statements. Do not use unless really needed */

#define PCIE_EXTRACT_BDF_SEG(bdf)  ((bdf >> 24) & 0xFF)
#define PCIE_EXTRACT_BDF_BUS(bdf)  ((bdf >> 16) & 0xFF)
#define PCIE_EXTRACT_BDF_DEV(bdf)  ((bdf >> 8) & 0xFF)
#define PCIE_EXTRACT_BDF_FUNC(bdf) (bdf & 0xFF)

#define PCIE_CFG_SIZE  4096

#define PCIE_MAX_BUS   256
#define PCIE_MAX_DEV    32
#define PCIE_MAX_FUNC    8

#define print(verbose, string, ...)  if(verbose >= g_print_level) \
                                                   printf(string, ##__VA_ARGS__)

#define PCIE_CREATE_BDF(Seg, Bus, Dev, Func) ((Seg << 24) | (Bus << 16) | (Dev << 8) | Func)

#define PCIE_SUCCESS            0x00000000  /* Operation completed successfully */
#define PCIE_NO_MAPPING         0x10000001  /* A mapping to a Function does not exist */
#define PCIE_CAP_NOT_FOUND      0x10000010  /* The specified capability was not found */
#define PCIE_UNKNOWN_RESPONSE   0xFFFFFFFF  /* Function not found or UR response from completer */

/* TYPE 0/1 Cmn Cfg reg offsets and mask*/
#define TYPE01_CPR           0x34
#define TYPE01_CPR_MASK      0xff

/* Class Code Masks */
#define CC_SUB_MASK     0xFF   /* Sub Class */
#define CC_BASE_MASK    0xFF   /* Base Class */

/* Class Code Shifts */
#define CC_SUB_SHIFT    16
#define CC_BASE_SHIFT   24

#define HB_BASE_CLASS   0x06
#define HB_SUB_CLASS    0x00

/* Device Type Shift and mask*/
#define PCIE_DEVICE_TYPE_SHIFT  20
#define PCIE_DEVICE_TYPE_MASK   0xf
#define PCI_EXP_DEVCTL          8
#define DEVCTL_SNOOP_BIT        11

/* Bus Number reg shifts */
#define SECBN_SHIFT 8
#define SUBBN_SHIFT 16

/* Bus Number reg masks */
#define SECBN_MASK  0xff
#define SUBBN_MASK  0xff

/* Capability header reg shifts */
#define PCIE_CIDR_SHIFT      0
#define PCIE_NCPR_SHIFT      8
#define PCIE_ECAP_CIDR_SHIFT 0
#define PCIE_ECAP_NCPR_SHIFT 20

/* Capability header reg masks */
#define PCIE_CIDR_MASK       0xff
#define PCIE_NCPR_MASK       0xff
#define PCIE_ECAP_CIDR_MASK  0xffff
#define PCIE_ECAP_NCPR_MASK  0xfff

#define PCIE_ECAP_START      0x100

/* Capability Structure IDs */
#define CID_PCIECS           0x10
#define CID_MSI              0x05
#define CID_MSIX             0x11
#define ECID_PASID           0x001b

/* PCI Express capability struct offsets */
#define CIDR_OFFSET    0
#define PCIECR_OFFSET  2
#define DCAPR_OFFSET   4
#define DCTLR_OFFSET   8
#define DCAP2R_OFFSET  24
#define DCTL2R_OFFSET  28

/* PCIe capabilities reg shifts and masks */
#define PCIECR_DPT_SHIFT 4
#define PCIECR_DPT_MASK  0xf

#define PASID_OFFSET         0x04
#define PASID_NUM_SHIFT      8
#define PASID_NUM_MASK       0x1f
#define PER_FLAG_MSI_ENABLED 0x2

/* Device bitmask definitions */
#define RCiEP    (1 << 0b1001)
#define RCEC     (1 << 0b1010)
#define EP       (1 << 0b0000)
#define RP       (1 << 0b0100)
#define UP       (1 << 0b0101)
#define DP       (1 << 0b0110)
#define iEP_EP   (1 << 0b1100)
#define iEP_RP   (1 << 0b1011)

#define CLEAN_AND_INVALIDATE  0x1
#define CLEAN                 0x2
#define INVALIDATE            0x3


typedef struct {
  uint64_t   Arg0;
  uint64_t   Arg1;
  uint64_t   Arg2;
  uint64_t   Arg3;
  uint64_t   Arg4;
  uint64_t   Arg5;
  uint64_t   Arg6;
  uint64_t   Arg7;
} ARM_SMC_ARGS;

typedef struct {
  uint32_t num_of_pe;
} PE_INFO_HDR;

/**
  @brief  structure instance for PE entry
**/
typedef struct {
  uint32_t   pe_num;    ///< PE Index
  uint32_t   attr;      ///< PE attributes
  uint64_t   mpidr;     ///< PE MPIDR
  uint32_t   pmu_gsiv;  ///< PMU Interrupt ID
} PE_INFO_ENTRY;

typedef struct {
  PE_INFO_HDR    header;
  PE_INFO_ENTRY  pe_info[];
} PE_INFO_TABLE;

void pal_pe_data_cache_ops_by_va(uint64_t addr, uint32_t type);

typedef struct {
  uint32_t   gic_version;
  uint32_t   num_gicd;
  uint32_t   num_gicrd;
  uint32_t   num_its;
}GIC_INFO_HDR;


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
  uint32_t type;
  uint64_t base;
  uint32_t its_id;  /* This its_id is only used in case of ITS Type entry */
  uint32_t length;  /* This length is only used in case of Re-Distributor Range Address length */
}GIC_INFO_ENTRY;

/**
  @brief  GIC Information Table
**/
typedef struct {
  GIC_INFO_HDR   header;
  GIC_INFO_ENTRY gic_info[];  ///< Array of Information blocks - instantiated for each GIC type
}GIC_INFO_TABLE;

typedef struct {
  uint32_t s_el1_timer_flag;
  uint32_t ns_el1_timer_flag;
  uint32_t el2_timer_flag;
  uint32_t el2_virt_timer_flag;
  uint32_t s_el1_timer_gsiv;
  uint32_t ns_el1_timer_gsiv;
  uint32_t el2_timer_gsiv;
  uint32_t virtual_timer_flag;
  uint32_t virtual_timer_gsiv;
  uint32_t el2_virt_timer_gsiv;
  uint32_t num_platform_timer;
  uint32_t num_watchdog;
  uint32_t sys_timer_status;
}TIMER_INFO_HDR;

#define TIMER_TYPE_SYS_TIMER 0x2001

/**
  @brief  structure instance for TIMER entry
**/
typedef struct {
  uint32_t type;
  uint32_t timer_count;
  uint64_t block_cntl_base;
  uint8_t  frame_num[8];
  uint64_t GtCntBase[8];
  uint64_t GtCntEl0Base[8];
  uint32_t gsiv[8];
  uint32_t virt_gsiv[8];
  uint32_t flags[8];
}TIMER_INFO_GTBLOCK;

typedef struct {
  TIMER_INFO_HDR     header;
  TIMER_INFO_GTBLOCK gt_info[];
}TIMER_INFO_TABLE;

/**
  @brief  Watchdog Info header - Summary of Watchdog subsytem
**/
typedef struct {
  uint32_t num_wd;  ///< number of Watchdogs present in the system
}WD_INFO_HDR;

/**
  @brief  structure instance for Watchdog entry
**/
typedef struct {
  uint64_t wd_ctrl_base;     ///< Watchdog Control Register Frame
  uint64_t wd_refresh_base;  ///< Watchdog Refresh Register Frame
  uint32_t wd_gsiv;          ///< Watchdog Interrupt ID
  uint32_t wd_flags;
}WD_INFO_BLOCK;

/**
  @brief Watchdog Info Table
**/
typedef struct {
  WD_INFO_HDR    header;
  WD_INFO_BLOCK  wd_info[];  ///< Array of Information blocks - instantiated for each WD Controller
}WD_INFO_TABLE;

/**
  @brief PCIe Info Table
**/

typedef struct {
  uint64_t   ecam_base;     ///< ECAM Base address
  uint32_t   segment_num;   ///< Segment number of this ECAM
  uint32_t   start_bus_num; ///< Start Bus number for this ecam space
  uint32_t   end_bus_num;   ///< Last Bus number
} PCIE_INFO_BLOCK;

typedef struct {
  uint32_t  num_entries;
  PCIE_INFO_BLOCK block[];
} PCIE_INFO_TABLE;

typedef struct {
  uint64_t   class_code;
  uint32_t   device_id;
  uint32_t   vendor_id;
  uint32_t   bus;
  uint32_t   dev;
  uint32_t   function;
  uint32_t   seg;
} PCIE_READ_BLOCK;

typedef struct {
  uint32_t num_entries;
  PCIE_READ_BLOCK device[];
} PCIE_READ_TABLE;

typedef struct {
  uint32_t bdf;
  uint32_t rp_bdf;
} pcie_device_attr;

typedef struct {
  uint32_t num_entries;
  pcie_device_attr device[];         ///< in the format of Segment/Bus/Dev/Func
} pcie_device_bdf_table;


typedef struct {
  uint32_t    num_usb;   ///< Number of USB  Controllers
  uint32_t    num_sata;  ///< Number of SATA Controllers
  uint32_t    num_uart;  ///< Number of UART Controllers
  uint32_t    num_all;   ///< Number of all PCI Controllers
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
  uint32_t         bdf;   ///< Bus Device Function
  uint64_t         base0; ///< Base Address of the controller
  uint64_t         base1; ///< Base Address of the controller
  uint32_t         irq;   ///< IRQ to install an ISR
  uint32_t         flags;
  uint32_t         msi;   ///< MSI Enabled
  uint32_t         msix;  ///< MSIX Enabled
  uint32_t         max_pasids;
}PERIPHERAL_INFO_BLOCK;

/**
  @brief Peripheral Info Structure
**/
typedef struct {
  PERIPHERAL_INFO_HDR     header;
  PERIPHERAL_INFO_BLOCK   info[]; ///< Array of Information blocks - instantiated for each peripheral
}PERIPHERAL_INFO_TABLE;

typedef struct {
  uint64_t  Address;
  uint8_t   AddressSpaceId;
  uint8_t   RegisterBitWidth;
  uint8_t   RegisterBitOffset;
  uint8_t   AccessSize;
} PLATFORM_OVERRIDE_GENERIC_ADDRESS_STRUCTURE;

typedef struct {
  uint64_t                                     Address;
  PLATFORM_OVERRIDE_GENERIC_ADDRESS_STRUCTURE  BaseAddress;
  uint32_t                                     GlobalSystemInterrupt;
  uint32_t                                     PciFlags;
  uint16_t                                     PciDeviceId;
  uint16_t                                     PciVendorId;
  uint8_t                                      PciBusNumber;
  uint8_t                                      PciDeviceNumber;
  uint8_t                                      PciFunctionNumber;
  uint8_t                                      PciSegment;
} PLATFORM_OVERRIDE_UART_INFO_TABLE;

/**
  @brief MSI(X) controllers info structure
**/

typedef struct {
  uint32_t  vector_upper_addr; ///< Bus Device Function
  uint32_t  vector_lower_addr; ///< Base Address of the controller
  uint32_t  vector_data;       ///< Base Address of the controller
  uint32_t  vector_control;    ///< IRQ to install an ISR
  uint32_t  vector_irq_base;   ///< Base IRQ for the vectors in the block
  uint32_t  vector_n_irqs;     ///< Number of irq vectors in the block
  uint32_t  vector_mapped_irq_base; ///< Mapped IRQ number base for this MSI
}PERIPHERAL_VECTOR_BLOCK;

typedef struct PERIPHERAL_VECTOR_LIST_STRUCT
{
  PERIPHERAL_VECTOR_BLOCK vector;
  struct PERIPHERAL_VECTOR_LIST_STRUCT *next;
}PERIPHERAL_VECTOR_LIST;

uint32_t pal_get_msi_vectors (uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn, PERIPHERAL_VECTOR_LIST **mvector);

#define LEGACY_PCI_IRQ_CNT 4  // Legacy PCI IRQ A, B, C. and D
#define MAX_IRQ_CNT 0xFFFF    // This value is arbitrary and may have to be adjusted

typedef struct {
  uint32_t  irq_list[MAX_IRQ_CNT];
  uint32_t  irq_count;
} PERIFERAL_IRQ_LIST;

typedef struct {
  PERIFERAL_IRQ_LIST  legacy_irq_map[LEGACY_PCI_IRQ_CNT];
} PERIPHERAL_IRQ_MAP;

/**
  @brief  Instance of SMMU INFO block
**/
typedef struct {
  uint32_t arch_major_rev;  ///< Version 1 or 2 or 3
  uint64_t base;              ///< SMMU Controller base address
}SMMU_INFO_BLOCK;

typedef struct {
  uint32_t segment;
  uint32_t ats_attr;
  uint32_t cca;             //Cache Coherency Attribute
  uint64_t smmu_base;
}IOVIRT_RC_INFO_BLOCK;

typedef struct {
  uint64_t base;
  uint32_t overflow_gsiv;
  uint32_t node_ref;
} IOVIRT_PMCG_INFO_BLOCK;

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
  uint32_t input_base;
  uint32_t id_count;
  uint32_t output_base;
  uint32_t output_ref;
}ID_MAP;

typedef union {
  uint32_t id[4];
  ID_MAP map;
}NODE_DATA_MAP;

#define MAX_NAMED_COMP_LENGTH 256

typedef union {
  char  name[MAX_NAMED_COMP_LENGTH];
  IOVIRT_RC_INFO_BLOCK rc;
  IOVIRT_PMCG_INFO_BLOCK pmcg;
  uint32_t its_count;
  SMMU_INFO_BLOCK smmu;
}NODE_DATA;

typedef struct {
  uint32_t type;
  uint32_t num_data_map;
  NODE_DATA data;
  uint32_t flags;
  NODE_DATA_MAP data_map[];
}IOVIRT_BLOCK;

typedef struct {
  uint32_t num_blocks;
  uint32_t num_smmus;
  uint32_t num_pci_rcs;
  uint32_t num_named_components;
  uint32_t num_its_groups;
  uint32_t num_pmcgs;
  IOVIRT_BLOCK blocks[];
}IOVIRT_INFO_TABLE;

#define IOVIRT_NEXT_BLOCK(b) (IOVIRT_BLOCK *)((uint8_t*)(&b->data_map[0]) + b->num_data_map * sizeof(NODE_DATA_MAP))
#define IOVIRT_CCA_MASK ~((uint32_t)0)

/* Memory INFO table */
#define MEM_MAP_SUCCESS  0x0
#define MEM_MAP_NO_MEM   0x1
#define MEM_MAP_FAILURE  0x2

typedef enum {
  MEMORY_TYPE_DEVICE = 0x1000,
  MEMORY_TYPE_NORMAL,
  MEMORY_TYPE_RESERVED,
  MEMORY_TYPE_NOT_POPULATED,
  MEMORY_TYPE_LAST_ENTRY
}MEM_INFO_TYPE_e;


typedef struct {
  MEM_INFO_TYPE_e type;
  uint64_t        phy_addr;
  uint64_t        virt_addr;
  uint64_t        size;
  uint64_t        flags;  //To Indicate Cacheablility etc..
}MEM_INFO_BLOCK;


typedef struct {
  uint64_t  dram_base;
  uint64_t  dram_size;
  MEM_INFO_BLOCK  info[];
} MEMORY_INFO_TABLE;

/**
  @brief DMA controllers info structure
**/
typedef enum {
  DMA_TYPE_USB  =  0x2000,
  DMA_TYPE_SATA,
  DMA_TYPE_OTHER,
}DMA_INFO_TYPE_e;

typedef struct {
  DMA_INFO_TYPE_e type;
  void            *target;   ///< The actual info stored in these pointers is implementation specific.
  void            *port;
  void            *host;     // It will be used only by PAL. hence void.
  uint32_t        flags;
}DMA_INFO_BLOCK;

typedef struct {
  uint32_t         num_dma_ctrls;
  DMA_INFO_BLOCK   info[];    ///< Array of information blocks - per DMA controller
}DMA_INFO_TABLE;

typedef enum {
    EDMA_NO_SUPPORT   = 0x0,
    EDMA_COHERENT     = 0x1,
    EDMA_NOT_COHERENT = 0x2,
    EDMA_FROM_DEVICE  = 0x3,
    EDMA_TO_DEVICE    = 0x4
} EXERCISER_DMA_ATTR;

typedef enum {
    SNOOP_ATTRIBUTES = 0x1,
    LEGACY_IRQ       = 0x2,
    MSIX_ATTRIBUTES  = 0x3,
    DMA_ATTRIBUTES   = 0x4,
    P2P_ATTRIBUTES   = 0x5,
    PASID_ATTRIBUTES = 0x6,
    CFG_TXN_ATTRIBUTES = 0x7
} EXERCISER_PARAM_TYPE;

typedef enum {
    EXERCISER_RESET = 0x1,
    EXERCISER_ON    = 0x2,
    EXERCISER_OFF   = 0x3,
    EXERCISER_ERROR = 0x4
} EXERCISER_STATE;

typedef enum {
    START_DMA     = 0x1,
    GENERATE_MSI  = 0x2,
    GENERATE_L_INTR = 0x3,  //Legacy interrupt
    MEM_READ      = 0x4,
    MEM_WRITE     = 0x5,
    CLEAR_INTR    = 0x6,
    PASID_TLP_START = 0x7,
    PASID_TLP_STOP  = 0x8,
    TXN_NO_SNOOP_ENABLE = 0x9,
    TXN_NO_SNOOP_DISABLE  = 0xa,
    START_TXN_MONITOR    = 0xb,
    STOP_TXN_MONITOR     = 0xc,
    ATS_TXN_REQ          = 0xd
} EXERCISER_OPS;

#endif
