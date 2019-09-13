/** @file
 * Copyright (c) 2016-2019, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __PAL_EXERCISER_H__
#define __PAL_EXERCISER_H__

#define EXERCISER_CLASSCODE 0x010203
#define MAX_ARRAY_SIZE 32
#define TEST_REG_COUNT 10
#define TEST_DDR_REGION_CNT 16

typedef struct {
    UINT64 buf[MAX_ARRAY_SIZE];
} EXERCISER_INFO_BLOCK;

typedef struct {
    UINT32                  num_exerciser_cards;
    EXERCISER_INFO_BLOCK    info[];  //Array of information blocks - per stimulus generation controller
} EXERCISER_INFO_TABLE;

typedef enum {
    EXERCISER_NUM_CARDS = 0x1
} EXERCISER_INFO_TYPE;

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
    PASID_ATTRIBUTES = 0x6
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
    NO_SNOOP_CLEAR_TLP_START = 0x9,
    NO_SNOOP_CLEAR_TLP_STOP  = 0xa
} EXERCISER_OPS;

typedef enum {
    ACCESS_TYPE_RD = 0x0,
    ACCESS_TYPE_RW = 0x1
} ECAM_REG_ATTRIBUTE;

struct ecam_reg_data {
    UINT32 offset;    //Offset into 4096 bytes ecam config reg space
    UINT32 attribute;
    UINT32 value;
};

struct exerciser_data_cfg_space {
    struct ecam_reg_data reg[TEST_REG_COUNT];
};

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

typedef enum {
    MMIO_PREFETCHABLE = 0x0,
    MMIO_NON_PREFETCHABLE = 0x1
} BAR_MEM_TYPE;

struct exerciser_data_bar_space {
    void *base_addr;
    BAR_MEM_TYPE type;
};

typedef union exerciser_data {
    struct exerciser_data_cfg_space cfg_space;
    struct exerciser_data_bar_space bar_space;
} exerciser_data_t;

typedef enum {
    EXERCISER_DATA_CFG_SPACE = 0x1,
    EXERCISER_DATA_BAR0_SPACE = 0x2,
} EXERCISER_DATA_TYPE;

VOID pal_exerciser_create_info_table (EXERCISER_INFO_TABLE *ExerciserInfoTable);
UINT32 pal_exerciser_get_info(EXERCISER_INFO_TYPE Type, UINT32 Instance);
UINT32 pal_exerciser_set_param(EXERCISER_PARAM_TYPE Type, UINT64 Value1, UINT64 Value2, UINT32 Instance);
UINT32 pal_exerciser_get_param(EXERCISER_PARAM_TYPE Type, UINT64 *Value1, UINT64 *Value2, UINT32 Instance);
UINT32 pal_exerciser_set_state(EXERCISER_STATE State, UINT64 *Value, UINT32 Instance);
UINT32 pal_exerciser_get_state(EXERCISER_STATE State, UINT64 *Value, UINT32 Instance);
UINT32 pal_exerciser_ops(EXERCISER_OPS Ops, UINT64 Param, UINT32 Instance);
UINT32 pal_exerciser_get_data(EXERCISER_DATA_TYPE Type, exerciser_data_t *Data, UINT32 Instance);

#endif
