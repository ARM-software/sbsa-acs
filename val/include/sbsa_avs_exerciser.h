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

#ifndef __SBSA_AVS_EXERCISER_H__
#define __SBSA_AVS_EXERCISER_H__

void val_exerciser_create_info_table(EXERCISER_INFO_TABLE *exerciser_info_table);
uint32_t val_exerciser_get_info(EXERCISER_INFO_TYPE type, uint32_t instance);
uint32_t val_exerciser_set_param(EXERCISER_PARAM_TYPE type, uint64_t value1, uint64_t value2, uint32_t instance);
uint32_t val_exerciser_get_param(EXERCISER_PARAM_TYPE type, uint64_t *value1, uint64_t *value2, uint32_t instance);
uint32_t val_exerciser_set_state(EXERCISER_STATE state, uint64_t *value, uint32_t instance);
uint32_t val_exerciser_get_state(EXERCISER_STATE state, uint64_t *value, uint32_t instance);
uint32_t val_exerciser_ops(EXERCISER_OPS ops, uint64_t param, uint32_t instance);

#endif
