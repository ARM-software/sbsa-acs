/** @file
 * Copyright (c) 2016-2020, Arm Limited or its affiliates. All rights reserved.
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

uint32_t e001_entry(void);
uint32_t e002_entry(void);
uint32_t e003_entry(void);
uint32_t e004_entry(void);
uint32_t e005_entry(void);
uint32_t e006_entry(void);
uint32_t e007_entry(void);
uint32_t e008_entry(void);
uint32_t e009_entry(void);
uint32_t e010_entry(void);
uint32_t e011_entry(void);
uint32_t e012_entry(void);
uint32_t e013_entry(void);
uint32_t e014_entry(void);
uint32_t e015_entry(void);

#endif
