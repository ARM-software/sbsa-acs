/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

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

#ifndef __PAL_INTERFACE_H__
#define __PAL_INTERFACE_H__

typedef CHAR8  char8_t;
typedef UINT8  uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;
typedef UINT64 addr_t;


#define TIMEOUT_LARGE    0x1000000
#define TIMEOUT_MEDIUM   0x100000
#define TIMEOUT_SMALL    0x1000


/**  PE Test related Definitions **/

/**
  @brief  number of PEs discovered
**/
typedef struct {
  uint32_t num_of_pe;
}PE_INFO_HDR;

/**
  @brief  structure instance for PE entry
**/
typedef struct {
  uint32_t   pe_num;    ///< PE Index
  uint32_t   attr;      ///< PE attributes
  uint64_t   mpidr;     ///< PE MPIDR
  uint32_t   pmu_gsiv;  ///< PMU Interrupt ID
}PE_INFO_ENTRY;

typedef struct {
  PE_INFO_HDR    header;
  PE_INFO_ENTRY  pe_info[];
}PE_INFO_TABLE;

void pal_pe_create_info_table(PE_INFO_TABLE *pe_info_table);

/**
  @brief  Structure to Pass SMC arguments. Return data is also filled into 
          the same structure.
**/
typedef struct {
  uint64_t  Arg0;
  uint64_t  Arg1;
  uint64_t  Arg2;
  uint64_t  Arg3;
  uint64_t  Arg4;
  uint64_t  Arg5;
  uint64_t  Arg6;
  uint64_t  Arg7;
} ARM_SMC_ARGS;

void pal_pe_call_smc(ARM_SMC_ARGS *args);
void pal_pe_execute_payload(ARM_SMC_ARGS *args);
uint32_t pal_pe_install_esr(uint32_t exception_type, void (*esr)(void));
/* ********** PE INFO END **********/


/** GIC Tests Related definitions **/

/**
  @brief  GIC Info header - Summary of GIC subsytem
**/
typedef struct {
  uint32_t gic_version;
  uint32_t num_gicd;
  uint32_t num_gicrd;
  uint32_t num_its;
}GIC_INFO_HDR;

typedef enum {
  ENTRY_TYPE_CPUIF = 0x1000,
  ENTRY_TYPE_GICD,
  ENTRY_TYPE_GICRD,
  ENTRY_TYPE_GICITS
}GIC_INFO_TYPE_e;

/**
  @brief  structure instance for GIC entry
**/
typedef struct {
  uint32_t type;
  uint64_t base;
}GIC_INFO_ENTRY;

/**
  @brief  GIC Information Table
**/
typedef struct {
  GIC_INFO_HDR   header;
  GIC_INFO_ENTRY gic_info[];  ///< Array of Information blocks - instantiated for each GIC type
}GIC_INFO_TABLE;

void     pal_gic_create_info_table(GIC_INFO_TABLE *gic_info_table);
uint32_t pal_gic_install_isr(uint32_t int_id, void (*isr)(void));
uint32_t pal_gic_end_of_interrupt(uint32_t int_id);


/** Timer tests related definitions **/

/**
  @brief  GIC Info header - Summary of Timer subsytem
**/
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
  uint64_t CntControl_base;
  uint64_t CntRead_base;
  uint32_t num_platform_timer;
  uint32_t num_watchdog;
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

void pal_timer_create_info_table(TIMER_INFO_TABLE *timer_info_table);

/** Watchdog tests related definitions **/

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

void pal_wd_create_info_table(WD_INFO_TABLE  *wd_table);


/* PCIe Tests related definitions */

/**
  @brief PCI Express Info Table
**/
typedef struct {
  addr_t   ecam_base;   ///< ECAM Base address
  uint32_t max_bus_num; ///< Maximum Bus number
}PCIE_INFO_TABLE;

uint64_t pal_pcie_get_mcfg_ecam(void);
void     pal_pcie_create_info_table(PCIE_INFO_TABLE *PcieTable);

/**
  @brief  Instance of SMMU INFO block
**/
typedef struct {
  uint32_t arch_major_rev;  ///< Version 1 or 2 or 3
  addr_t base;              ///< SMMU Controller base address
}SMMU_INFO_BLOCK;

/**
  @brief SMMU Info Table
**/
typedef struct {
  uint32_t smmu_num_ctrl;       ///< Number of SMMU Controllers in the system
  SMMU_INFO_BLOCK smmu_block[]; ///< Array of Information blocks - instantiated for each SMMU Controller
}SMMU_INFO_TABLE;

void     pal_smmu_create_info_table(SMMU_INFO_TABLE *SmmuTable);

/** Peripheral Tests related definitions **/

/**
  @brief  Summary of Peripherals in the system
**/
typedef struct {
  uint32_t    num_usb;   ///< Number of USB  Controllers
  uint32_t    num_sata;  ///< Number of SATA Controllers
  uint32_t    num_uart;  ///< Number of UART Controllers
}PERIPHERAL_INFO_HDR;

typedef enum {
  PERIPHERAL_TYPE_USB = 0x2000,
  PERIPHERAL_TYPE_SATA,
  PERIPHERAL_TYPE_UART
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
}PERIPHERAL_INFO_BLOCK;

/**
  @brief Peripheral Info Structure
**/
typedef struct {
  PERIPHERAL_INFO_HDR     header;
  PERIPHERAL_INFO_BLOCK   info[]; ///< Array of Information blocks - instantiated for each peripheral
}PERIPHERAL_INFO_TABLE;

void  pal_peripheral_create_info_table(PERIPHERAL_INFO_TABLE *peripheralInfoTable);

/* Memory INFO table */
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

void  pal_memory_create_info_table(MEMORY_INFO_TABLE *memoryInfoTable);

/* Common Definitions */
void     pal_print(char8_t *string, uint64_t data);

void     *pal_mem_alloc(uint32_t size);
void     pal_mem_free(void *buffer);

void     pal_mem_allocate_shared(uint32_t num_pe, uint32_t sizeofentry);
void     pal_mem_free_shared(void);
uint64_t pal_mem_get_shared_addr(void);

uint32_t pal_mmio_read(uint64_t addr);
void     pal_mmio_write(uint64_t addr, uint32_t data);

void     pal_pe_update_elr(uint64_t offset);
void     pal_pe_data_cache_ci_va(uint64_t addr);
#endif

