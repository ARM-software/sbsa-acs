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

#ifndef __SBSA_AVS_SMMU_H__
#define __SBSA_AVS_SMMU_H__

#define SMMUv2_IDR0   0x20
#define SMMUv2_IDR1   0x24
#define SMMUv2_IDR2   0x28
#define SMMUv2_IDR3   0x2C
#define SMMUv2_IDR7   0x3C

#define SMMUv3_IDR0   0x00
#define SMMUv3_IDR1   0x04
#define SMMUv3_IDR5   0x14
#define SMMUv3_IIDR   0x18
#define SMMUv3_AIDR   0x1C

uint32_t
val_smmu_read_cfg(uint32_t offset, uint32_t index);

uint64_t
val_smmu_ops(SMMU_OPS_e ops, uint32_t index, void *param1, void *param2);

uint32_t
val_smmu_max_pasids(uint32_t smmu_index);

uint32_t
val_smmu_create_pasid_entry(uint32_t smmu_index, uint32_t pasid);

uint32_t
val_smmu_disable(uint32_t smmu_index);

uint64_t
val_smmu_pa2iova(uint32_t smmu_index, uint64_t pa);


uint32_t
i001_entry(uint32_t num_pe);
uint32_t
i002_entry(uint32_t num_pe);
uint32_t
i003_entry(uint32_t num_pe);
uint32_t
i004_entry(uint32_t num_pe);
uint32_t
i005_entry(uint32_t num_pe);
uint32_t
i006_entry(uint32_t num_pe);

#endif
