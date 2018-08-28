/** @file
 * Copyright (c) 2016-2018, Arm Limited or its affiliates. All rights reserved.
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

#define MAX_ARRAY_SIZE 32

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

enum SNOOP {
    DISABLE_NO_SNOOP = 0x0,
    ENABLE_NO_SNOOP  = 0x1
};

typedef enum {
    SNOOP_ATTRIBUTES = 0x1,
    LEGACY_IRQ       = 0x2,
    MSIX_ATTRIBUTES  = 0x3,
    DMA_ATTRIBUTES   = 0x4,
    P2P_ATTRIBUTES   = 0x5
} EXERCISER_PARAM_TYPE;

typedef enum {
    EXERCISER_RESET = 0x1,
    EXERCISER_ON    = 0x2,
    EXERCISER_OFF   = 0x3,
    EXERCISER_ERROR = 0x4
} EXERCISER_STATE;

typedef enum {
    START_DMA     = 0x1,
    GENERATE_INTR = 0x2,
    MEM_READ      = 0x3,
    MEM_WRITE     = 0x4
} EXERCISER_OPS;

VOID pal_exerciser_create_info_table (EXERCISER_INFO_TABLE *ExerciserInfoTable);
UINT32 pal_exerciser_get_info(EXERCISER_INFO_TYPE Type, UINT32 Instance);
UINT32 pal_exerciser_set_param(EXERCISER_PARAM_TYPE Type, UINT64 Value1, UINT64 Value2, UINT32 Instance);
UINT32 pal_exerciser_get_param(EXERCISER_PARAM_TYPE Type, UINT64 *Value1, UINT64 *Value2, UINT32 Instance);
UINT32 pal_exerciser_set_state(EXERCISER_STATE State, UINT64 *Value, UINT32 Instance);
UINT32 pal_exerciser_get_state(EXERCISER_STATE State, UINT64 *Value, UINT32 Instance);
UINT32 pal_exerciser_ops(EXERCISER_OPS Ops, UINT64 Param, UINT32 Instance);

#endif
