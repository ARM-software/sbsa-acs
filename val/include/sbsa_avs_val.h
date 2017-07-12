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

#ifndef __SBSA_AVS_VAL_H__
#define __SBSA_AVS_VAL_H__

#include "val_interface.h"
#include "pal_interface.h"
#include "sbsa_avs_cfg.h"
#include "sbsa_avs_common.h"


typedef struct {
  uint64_t    data0;
  uint64_t    data1;
  uint32_t    status;
}VAL_SHARED_MEM_t;

uint64_t
val_pe_reg_read(uint32_t reg_id);

void
val_pe_reg_write(uint32_t reg_id, uint64_t write_data);

uint8_t
val_is_el3_enabled(void);

uint8_t
val_is_el2_enabled(void);

void
val_report_status(uint32_t id, uint32_t status);

void
val_set_status(uint32_t index, uint32_t status);

uint32_t
val_get_status(uint32_t id);

#endif

