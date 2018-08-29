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

#ifndef __VAL_EXERCISER_H__
#define __VAL_EXERCISER_H__


#define MAX_ARRAY_SIZE 32

typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef struct {
    uint64_t buf[MAX_ARRAY_SIZE];
} EXERCISER_INFO_BLOCK;

typedef struct {
    uint32_t                num_exerciser_cards;
    EXERCISER_INFO_BLOCK    info[];  //Array of information blocks - per stimulus generation controller
} EXERCISER_INFO_TABLE;

typedef enum {
    EXERCISER_NUM_CARDS = 0x1
} EXERCISER_INFO_TYPE;

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
    DO_READ       = 0x3,
    DOWRITE       = 0x4
} EXERCISER_OPS;

void pal_exerciser_create_info_table(EXERCISER_INFO_TABLE *exerciser_info_table);
uint32_t pal_exerciser_get_info(EXERCISER_INFO_TYPE type, uint32_t instance);
uint32_t pal_exerciser_set_param(EXERCISER_PARAM_TYPE type, uint64_t value1, uint64_t value2, uint32_t instance);
uint32_t pal_exerciser_get_param(EXERCISER_PARAM_TYPE type, uint64_t *value1, uint64_t *value2, uint32_t instance);
uint32_t pal_exerciser_set_state(EXERCISER_STATE state, uint64_t *value, uint32_t instance);
uint32_t pal_exerciser_get_state(EXERCISER_STATE state, uint64_t *value, uint32_t instance);
uint32_t pal_exerciser_do_ops(EXERCISER_OPS ops, uint64_t param, uint32_t instance);

#endif
