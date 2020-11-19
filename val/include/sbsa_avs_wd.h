/** @file
 * Copyright (c) 2016-2018, 2020 Arm Limited or its affiliates. All rights reserved.
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

#ifndef __SBSA_AVS_WD_H
#define __SBSA_AVS_WD_H

#define  WD_IIDR_OFFSET     0xFCC

uint32_t
w001_entry(uint32_t num_pe);

uint32_t
w002_entry(uint32_t num_pe);

uint32_t
w003_entry(uint32_t num_pe);

#endif // __SBSA_AVS_WD_H
