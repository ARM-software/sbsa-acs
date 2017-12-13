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

#ifndef __VAL_INTERFACE_H__
#define __VAL_INTERFACE_H__

#include "pal_interface.h"

/* set G_PRINT_LEVEL to one of the below values in your application entry 
  to control the verbosity of the prints */
#define AVS_PRINT_ERR   5      /* Only Errors. use this to de-clutter the terminal and focus only on specifics */
#define AVS_PRINT_WARN  4      /* Only warnings & errors. use this to de-clutter the terminal and focus only on specifics */
#define AVS_PRINT_TEST  3      /* Test description and result descriptions. THIS is DEFAULT */
#define AVS_PRINT_DEBUG 2      /* For Debug statements. contains register dumps etc */
#define AVS_PRINT_INFO  1      /* Print all statements. Do not use unless really needed */


#define AVS_STATUS_FAIL 0x90000000
#define AVS_STATUS_ERR  0xEDCB1234  //some impropable value?
#define AVS_STATUS_SKIP 0x10000000
#define AVS_STATUS_PASS 0x0

/* GENERIC VAL APIs */
void val_allocate_shared_mem(void);
void val_free_shared_mem(void);
void val_print(uint32_t level, char8_t *string, uint64_t data);
void val_print_raw(uint32_t level, char8_t *string, uint64_t data);
void val_set_test_data(uint32_t index, uint64_t addr, uint64_t test_data);
void val_get_test_data(uint32_t index, uint64_t *data0, uint64_t *data1);



/* VAL PE APIs */
uint32_t val_pe_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_pe_create_info_table(uint64_t *pe_info_table);
void     val_pe_free_info_table(void);
uint32_t val_pe_get_num(void);
uint64_t val_pe_get_mpid_index(uint32_t index);
uint32_t val_pe_get_pmu_gsiv(uint32_t index);
uint64_t val_pe_get_mpid(void);
uint32_t val_pe_get_index_mpid(uint64_t mpid);
uint32_t val_pe_install_esr(uint32_t exception_type, void (*esr)(uint64_t, void *));

void     val_execute_on_pe(uint32_t index, void (*payload)(void), uint64_t args);
void     val_suspend_pe(uint32_t power_state, uint64_t entry, uint32_t context_id);

/* GIC VAL APIs */
uint32_t    val_gic_create_info_table(uint64_t *gic_info_table);
typedef enum {
  GIC_INFO_VERSION=1,
  GIC_INFO_SEC_STATES,
  GIC_INFO_DIST_BASE,
  GIC_INFO_CITF_BASE,
  GIC_INFO_NUM_RDIST,
  GIC_INFO_RDIST_BASE,
  GIC_INFO_NUM_ITS,
  GIC_INFO_ITS_BASE
}GIC_INFO_e;

uint32_t
val_gic_get_info(GIC_INFO_e type);
void     val_gic_free_info_table(void);
uint32_t val_gic_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_gic_install_isr(uint32_t int_id, void (*isr)(void));
uint32_t val_gic_end_of_interrupt(uint32_t int_id);
uint32_t val_gic_route_interrupt_to_pe(uint32_t int_id, uint64_t mpidr);
uint32_t val_gic_get_interrupt_state(uint32_t int_id);
void val_gic_clear_interrupt(uint32_t int_id);
void val_gic_cpuif_init(void);

/*TIMER VAL APIs */
typedef enum {
  TIMER_INFO_CNTFREQ = 1,
  TIMER_INFO_PHY_EL1_INTID,
  TIMER_INFO_PHY_EL1_FLAGS,
  TIMER_INFO_VIR_EL1_INTID,
  TIMER_INFO_VIR_EL1_FLAGS,
  TIMER_INFO_PHY_EL2_INTID,
  TIMER_INFO_PHY_EL2_FLAGS,
  TIMER_INFO_VIR_EL2_INTID,
  TIMER_INFO_VIR_EL2_FLAGS,
  TIMER_INFO_NUM_PLATFORM_TIMERS,
  TIMER_INFO_IS_PLATFORM_TIMER_SECURE,
  TIMER_INFO_SYS_CNTL_BASE,
  TIMER_INFO_SYS_CNT_BASE_N,
  TIMER_INFO_SYS_INTID,
  TIMER_INFO_SYS_TIMER_STATUS
}TIMER_INFO_e;

#define SBSA_TIMER_FLAG_ALWAYS_ON 0x4

void     val_timer_create_info_table(uint64_t *timer_info_table);
void     val_timer_free_info_table(void);
uint32_t val_timer_execute_tests(uint32_t level, uint32_t num_pe);
uint64_t val_timer_get_info(TIMER_INFO_e info_type, uint64_t instance);
void     val_timer_set_phy_el1(uint64_t timeout);
void     val_timer_set_vir_el1(uint64_t timeout);
void     val_timer_set_phy_el2(uint64_t timeout);
void     val_timer_set_vir_el2(uint64_t timeout);
void     val_timer_set_system_timer(addr_t cnt_base_n, uint32_t timeout);
void     val_timer_disable_system_timer(addr_t cnt_base_n);
uint32_t val_timer_skip_if_cntbase_access_not_allowed(uint64_t index);
void val_platform_timer_get_entry_index(uint64_t instance, uint32_t *block, uint32_t *index);

/* Watchdog VAL APIs */
typedef enum {
  WD_INFO_COUNT = 1,
  WD_INFO_CTRL_BASE,
  WD_INFO_REFRESH_BASE,
  WD_INFO_GSIV,
  WD_INFO_ISSECURE
}WD_INFO_TYPE_e;

void     val_wd_create_info_table(uint64_t *wd_info_table);
void     val_wd_free_info_table(void);
uint32_t val_wd_execute_tests(uint32_t level, uint32_t num_pe);
uint64_t val_wd_get_info(uint32_t index, uint32_t info_type);
void     val_wd_set_ws0(uint32_t index, uint32_t timeout);


/* PCIE VAL APIs */
void     val_pcie_create_info_table(uint64_t *pcie_info_table);
void     val_pcie_free_info_table(void);
uint32_t val_pcie_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_pcie_is_devicedma_64bit(uint32_t bdf);

/* IO-VIRT APIs */
typedef enum {
  SMMU_NUM_CTRL = 1,
  SMMU_CTRL_BASE,
  SMMU_CTRL_ARCH_MAJOR_REV,
  SMMU_IOVIRT_BLOCK
}SMMU_INFO_e;

typedef enum {
  SMMU_CAPABLE     = 1,
  SMMU_CHECK_DEVICE_IOVA,
  SMMU_START_MONITOR_DEV,
  SMMU_STOP_MONITOR_DEV,
  SMMU_CREATE_MAP,
  SMMU_UNMAP,
  SMMU_IOVA_PHYS,
  SMMU_DEV_DOMAIN,
  SMMU_GET_ATTR,
  SMMU_SET_ATTR,
}SMMU_OPS_e;

typedef enum {
  NUM_PCIE_RC = 1,
  RC_SEGMENT_NUM,
  RC_ATS_ATTRIBUTE,
  RC_MEM_ATTRIBUTE,
  RC_IOVIRT_BLOCK
}PCIE_RC_INFO_e;

void     val_iovirt_create_info_table(uint64_t *iovirt_info_table);
void     val_iovirt_free_info_table(void);
uint32_t val_smmu_execute_tests(uint32_t level, uint32_t num_pe);
uint64_t val_smmu_get_info(SMMU_INFO_e, uint32_t index);


typedef enum {
    DMA_NUM_CTRL = 1,
    DMA_HOST_INFO,
    DMA_PORT_INFO,
    DMA_TARGET_INFO,
    DMA_HOST_COHERENT,
    DMA_HOST_IOMMU_ATTACHED,
    DMA_HOST_PCI
} DMA_INFO_e;

void     val_dma_create_info_table(uint64_t *dma_info_ptr);
uint64_t val_dma_get_info(DMA_INFO_e type, uint32_t index);
uint32_t val_dma_start_from_device(void *buffer, uint32_t length, uint32_t index);
uint32_t val_dma_start_to_device(void *buffer, uint32_t length, uint32_t index);
uint32_t val_dma_iommu_check_iova(uint32_t ctrl_index, addr_t dma_addr, addr_t cpu_addr);
void     val_dma_device_get_dma_addr(uint32_t ctrl_index, void *dma_addr, uint32_t *cpu_len);
int      val_dma_mem_get_attrs(void *buf, uint32_t *attr, uint32_t *sh);


/* POWER and WAKEUP APIs */
typedef enum {
    SBSA_POWER_SEM_B = 1,
    SBSA_POWER_SEM_c,
    SBSA_POWER_SEM_D,
    SBSA_POWER_SEM_E,
    SBSA_POWER_SEM_F,
    SBSA_POWER_SEM_G,
    SBSA_POWER_SEM_H,
    SBSA_POWER_SEM_I
} SBSA_POWER_SEM_e;

uint32_t val_power_enter_semantic(SBSA_POWER_SEM_e semantic);
uint32_t val_wakeup_execute_tests(uint32_t level, uint32_t num_pe);

typedef enum {
    PER_FLAG_MSI_ENABLED = 0x2
}PERIPHERAL_FLAGS_e;

/* Peripheral Tests APIs */
typedef enum {
  NUM_USB,
  NUM_SATA,
  NUM_UART,
  NUM_ALL,
  USB_BASE0,
  USB_FLAGS,
  USB_GSIV,
  USB_BDF,
  SATA_BASE0,
  SATA_BASE1,
  SATA_FLAGS,
  SATA_GSIV,
  SATA_BDF,
  UART_BASE0,
  UART_GSIV,
  UART_FLAGS,
  ANY_FLAGS,
  ANY_GSIV,
  ANY_BDF,
  MAX_PASIDS
}PERIPHERAL_INFO_e;

void     val_peripheral_create_info_table(uint64_t *peripheral_info_table);
void     val_peripheral_free_info_table(void);
uint32_t val_peripheral_execute_tests(uint32_t level, uint32_t num_pe);
uint64_t val_peripheral_get_info(PERIPHERAL_INFO_e info_type, uint32_t index);

/* Memory Tests APIs */
typedef enum {
  MEM_TYPE_DEVICE = 0x1000,
  MEM_TYPE_NORMAL,
  MEM_TYPE_RESERVED,
  MEM_TYPE_NOT_POPULATED,
  MEM_TYPE_LAST_ENTRY
}MEMORY_INFO_e;

#define MEM_ATTR_UNCACHED  0x2000
#define MEM_ATTR_CACHED    0x1000

/* Identify memory type using MAIR attribute, refer to ARM ARM VMSA for details */

#define MEM_NORMAL_WB_IN_OUT(attr) (((attr & 0xcc) == 0xcc) || (((attr & 0x7) >= 5) && (((attr >> 4) & 0x7) >= 5)))
#define MEM_NORMAL_NC_IN_OUT(attr) (attr == 0x44)
#define MEM_DEVICE(attr) ((attr & 0xf0) == 0)
#define MEM_SH_INNER(sh) (sh == 0x3)

void     val_memory_create_info_table(uint64_t *memory_info_table);
void     val_memory_free_info_table(void);
uint32_t val_memory_execute_tests(uint32_t level, uint32_t num_pe);
uint64_t val_memory_get_info(addr_t addr, uint64_t *attr);


/* Secure mode EL3 Firmware tests */

typedef struct {
  uint64_t   test_index;
  uint64_t   test_arg01;
  uint64_t   test_arg02;
  uint64_t   test_arg03;
} SBSA_SMC_t;

void     val_secure_call_smc(SBSA_SMC_t *smc);
uint32_t val_secure_get_result(SBSA_SMC_t *smc, uint32_t timeout);
uint32_t val_secure_execute_tests(uint32_t level, uint32_t num_pe);
uint32_t val_secure_trusted_firmware_init(void);

#endif
