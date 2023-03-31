/** @file
 * Copyright (c) 2020, 2022-2023 Arm Limited or its affiliates. All rights reserved.
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

#include "include/pal_common_support.h"
#include "include/pal_pcie_enum.h"

#define SMMU_V3_IDR1 0x4
#define SMMU_V3_IDR1_PASID_SHIFT 6
#define SMMU_V3_IDR1_PASID_MASK  0x1f

uint32_t
pal_smmu_max_pasids(uint64_t smmu_base)
{
  uint32_t reg = pal_mmio_read(smmu_base + SMMU_V3_IDR1);
  uint32_t pasid_bits = (reg >> SMMU_V3_IDR1_PASID_SHIFT) & SMMU_V3_IDR1_PASID_MASK;

  return pasid_bits;
}
