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

#ifndef __SBSA_AVS_EXERCISER_H__
#define __SBSA_AVS_EXERCISER_H__


#define MAX_EXERCISER_CARDS 20
#define BUS_MEM_EN_MASK 0x06

/* PCIe Config space Offset */
#define COMMAND_REG_OFFSET 0x04

#define MASTER_ABORT_MASK  0x20000000
#define MASTER_ABORT_SHIFT 29

#define CORR_RCVR_ERR_OFFSET           0
#define CORR_BAD_TLP_OFFSET            6
#define CORR_BAD_DLLP_OFFSET           7
#define CORR_RPL_NUM_ROLL_OFFSET       8
#define CORR_RPL_TMR_TIMEOUT_OFFSET    12
#define CORR_ADV_NF_ERR_OFFSET         13
#define CORR_INT_ERR_OFFSET            14
#define CORR_HDR_LOG_OVRFL_OFFSET      15
#define UNCORR_DL_ERROR_OFFSET         4
#define UNCORR_SD_ERROR_OFFSET         5
#define UNCORR_PTLP_REC_OFFSET         12
#define UNCORR_FL_CTRL_ERR_OFFSET      13
#define UNCORR_CMPT_TO_OFFSET          14
#define UNCORR_AMPT_ABORT_OFFSET       15
#define UNCORR_UNEXP_CMPT_OFFSET       16
#define UNCORR_RCVR_ERR_OFFSET         17
#define UNCORR_MAL_TLP_OFFSET          18
#define UNCORR_ECRC_ERR_OFFSET         19
#define UNCORR_UR_OFFSET               20
#define UNCORR_ACS_VIOL_OFFSET         21
#define UNCORR_INT_ERR_OFFSET          22
#define UNCORR_MC_BLK_TLP_OFFSET       23
#define UNCORR_ATOP_EGR_BLK_OFFSET     24
#define UNCORR_TLP_PFX_EGR_BLK_OFFSET  25
#define UNCORR_PTLP_EGR_BLK_OFFSET     26

typedef struct {
    uint32_t bdf;
    uint32_t initialized;
} EXERCISER_INFO_BLOCK;

typedef struct {
    uint32_t                num_exerciser;
    EXERCISER_INFO_BLOCK    e_info[MAX_EXERCISER_CARDS];
} EXERCISER_INFO_TABLE;

typedef enum {
    EXERCISER_NUM_CARDS = 0x1
} EXERCISER_INFO_TYPE;

typedef enum {
    CORR_RCVR_ERR = 0x0,
    CORR_BAD_TLP  = 0x1,
    CORR_BAD_DLLP = 0x2,
    CORR_RPL_NUM_ROLL = 0x3,
    CORR_RPL_TMR_TIMEOUT = 0x4,
    CORR_ADV_NF_ERR = 0x5,
    CORR_INT_ERR = 0x6,
    CORR_HDR_LOG_OVRFL = 0x7,
    UNCORR_DL_ERROR = 0x8,
    UNCORR_SD_ERROR = 0x9,
    UNCORR_PTLP_REC = 0xA,
    UNCORR_FL_CTRL_ERR = 0xB,
    UNCORR_CMPT_TO = 0xC,
    UNCORR_AMPT_ABORT = 0xD,
    UNCORR_UNEXP_CMPT = 0xE,
    UNCORR_RCVR_ERR = 0xF,
    UNCORR_MAL_TLP = 0x10,
    UNCORR_ECRC_ERR = 0x11,
    UNCORR_UR = 0x12,
    UNCORR_ACS_VIOL = 0x13,
    UNCORR_INT_ERR = 0x14,
    UNCORR_MC_BLK_TLP = 0x15,
    UNCORR_ATOP_EGR_BLK = 0x16,
    UNCORR_TLP_PFX_EGR_BLK = 0x17,
    UNCORR_PTLP_EGR_BLK = 0x18,
    INVALID_CFG = 0x19
} EXERCISER_ERROR_CODE;

typedef struct {
  uint32_t bdf;
  uint32_t rp_bdf;
} device_attr;

typedef struct {
  uint32_t num_entries;
  device_attr device[];         ///< in the format of Segment/Bus/Dev/Func
} exerciser_device_bdf_table;

void val_exerciser_create_info_table(void);
uint32_t val_exerciser_init(uint32_t instance);
uint32_t val_exerciser_get_info(EXERCISER_INFO_TYPE type, uint32_t instance);
uint32_t val_exerciser_set_param(EXERCISER_PARAM_TYPE type, uint64_t value1, uint64_t value2, uint32_t instance);
uint32_t val_exerciser_get_param(EXERCISER_PARAM_TYPE type, uint64_t *value1, uint64_t *value2, uint32_t instance);
uint32_t val_exerciser_set_state(EXERCISER_STATE state, uint64_t *value, uint32_t instance);
uint32_t val_exerciser_get_state(EXERCISER_STATE *state, uint32_t instance);
uint32_t val_exerciser_ops(EXERCISER_OPS ops, uint64_t param, uint32_t instance);
uint32_t val_exerciser_get_data(EXERCISER_DATA_TYPE type, exerciser_data_t *data, uint32_t instance);
uint32_t val_exerciser_execute_tests(uint32_t level);
uint32_t val_exerciser_get_bdf(uint32_t instance);
uint32_t val_get_exerciser_err_info(uint32_t type);

uint32_t e001_entry(void);
uint32_t e002_entry(void);
uint32_t e003_entry(void);
uint32_t e004_entry(void);
uint32_t e005_entry(void);
uint32_t e006_entry(void);
uint32_t e007_entry(void);
uint32_t e008_entry(void);

#endif
