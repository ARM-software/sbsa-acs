/** @file
 * Copyright (c) 2022 Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
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

#ifndef __SBSA_AVS_RAS_H
#define __SBSA_AVS_RAS_H


#define ERR_FR_INJ_MASK  (0x3ull << 20)
#define ERR_FR_DUI_MASK  (0x3ull << 16)
#define ERR_FR_CEC_MASK  (0x7ull << 12)
#define ERR_FR_CFI_MASK  (0x3ull << 10)
#define ERR_FR_UI_MASK   (0x3ull << 4)

#define ERR_STATUS_V_MASK   (0x1 << 30)
#define ERR_STATUS_AV_MASK  (0x1 << 31)
#define ERR_STATUS_UE_MASK  (0x1 << 29)
#define ERR_STATUS_CE_MASK  (0x3 << 24)
#define ERR_STATUS_DE_MASK  (0x1 << 23)
#define ERR_STATUS_PN_MASK  (0x1 << 22)
#define ERR_STATUS_CI_MASK  (0x1 << 19)
#define ERR_STATUS_CLEAR    (0xFFF80000)

#define ERR_CTLR_CLEAR_MASK     0x3FFD
#define ERR_CTLR_ED_ENABLE      0x1

#define ERR_ADDR_AI_SHIFT 61

#define ERR_PFGCTL_UC_ENABLE      (0x1ull << 1)
#define ERR_PFGCTL_DE_ENABLE      (0x1ull << 5)
#define ERR_PFGCTL_CE_NON_ENABLE  (0x1ull << 6)
#define ERR_PFGCTL_CI_ENABLE      (0x1ull << 8)
#define ERR_PFGCTL_CDNEN_ENABLE   (0x1ull << 31)

#define ERR_FR_OFFSET           0x000
#define ERR_CTLR_OFFSET         0x008
#define ERR_STATUS_OFFSET       0x010
#define ERR_ADDR_OFFSET         0x018
#define ERR_PFGCTL_OFFSET       0x808
#define ERR_PFGCDN_OFFSET       0x810
#define ERR_ERRDEVAFF_OFFSET    0xFA8

#define RAS_INTERFACE_SR        0x0
#define RAS_INTERFACE_MMIO      0x1

#define AVS_ALL_1_64BIT         0xFFFFFFFFFFFFFFFF

typedef enum {
    RAS_ERR_FR = 0x1,
    RAS_ERR_CTLR,
    RAS_ERR_STATUS,
    RAS_ERR_ADDR,
    RAS_ERR_PFGCDN,
    RAS_ERR_PFGCTL,
    RAS_ERR_ERRDEVAFF
} RAS_REG_LIST;

typedef enum {
    RAS_INFO_NUM_PE = 0x1,       /* Number of PE RAS Node */
    RAS_INFO_NUM_MC,             /* Number of MC RAS Node */
    RAS_INFO_NUM_NODES,          /* Number of RAS Nodes */
    RAS_INFO_NODE_TYPE,          /* RAS Node Type */
    RAS_INFO_PE_RES_TYPE,        /* PE Resource Type */
    RAS_INFO_MC_RES_PROX_DOMAIN, /* Memory controller RAS node proximity domain */
    RAS_INFO_INTF_TYPE,          /* RAS registers interface type */
    RAS_INFO_ADDR_MODE,          /* bitmap based policy for ERR<n>ADDR field of error records */
    RAS_INFO_BASE_ADDR,          /* Base Address */
    RAS_INFO_NUM_ERR_REC,        /* Number of Error Record */
    RAS_INFO_START_INDEX,        /* Error Record Start Index */
    RAS_INFO_ERR_REC_IMP,        /* Error Record Implemented */
    RAS_INFO_STATUS_REPORT,      /* Error Status Reporting */
    RAS_INFO_ERI_ID,             /* ERI Interrupt ID */
    RAS_INFO_FHI_ID,             /* FHI Interrupt ID */
    RAS_INFO_PFG_SUPPORT,        /* Pseudo Fault Inj Support */
    RAS_INFO_NODE_INDEX_FOR_AFF  /* RAS Node Index for Affinity */
} RAS_INFO_TYPE;

uint32_t val_ras_setup_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param);
uint32_t val_ras_inject_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param);
void val_ras_wait_timeout(uint32_t count);

uint32_t val_ras_check_err_record(uint32_t node_index, uint32_t error_type);
uint32_t val_ras_check_plat_poison_support(void);

uint64_t val_ras_reg_read(uint32_t node_index, uint32_t reg, uint32_t err_rec_idx);
void val_ras_reg_write(uint32_t node_index, uint32_t reg, uint64_t write_data);

uint32_t ras001_entry(uint32_t num_pe);
uint32_t ras002_entry(uint32_t num_pe);
uint32_t ras003_entry(uint32_t num_pe);
uint32_t ras004_entry(uint32_t num_pe);
uint32_t ras005_entry(uint32_t num_pe);
uint32_t ras006_entry(uint32_t num_pe);
uint32_t ras007_entry(uint32_t num_pe);
uint32_t ras008_entry(uint32_t num_pe);
uint32_t ras009_entry(uint32_t num_pe);
uint32_t ras010_entry(uint32_t num_pe);
uint32_t ras011_entry(uint32_t num_pe);
uint32_t ras012_entry(uint32_t num_pe);

uint64_t AA64ReadErrIdr1(void);
uint64_t AA64ReadErrAddr1(void);
uint64_t AA64ReadErrCtlr1(void);
uint64_t AA64ReadErrFr1(void);
uint64_t AA64ReadErrStatus1(void);
uint64_t AA64ReadErrSelr1(void);
uint64_t AA64ReadErrPfgf1(void);
uint64_t AA64ReadErrPfgctl1(void);
uint64_t AA64ReadErrPfgcdn1(void);

void AA64WriteErrIdr1(uint64_t write_data);
void AA64WriteErrAddr1(uint64_t write_data);
void AA64WriteErrCtlr1(uint64_t write_data);
void AA64WriteErrStatus1(uint64_t write_data);
void AA64WriteErrSelr1(uint64_t write_data);
void AA64WriteErrPfgf1(uint64_t write_data);
void AA64WriteErrPfgctl1(uint64_t write_data);
void AA64WriteErrPfgcdn1(uint64_t write_data);

#endif // __SBSA_AVS_RAS_H
