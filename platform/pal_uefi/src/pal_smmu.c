/** @file
 * Copyright (c) 2019, Arm Limited or its affiliates. All rights reserved.
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

/* This is a place-holder file. Need to be implemented if needed in later releases */

/**
  @brief   This API prepares the smmu page tables to support input PasId
  @param   SmmuBase - Physical addr of the SMMU for which PasId support is needed
  @param   PasId    - Process Address Space identifier
  @return  zero for success, one for failure
**/
UINT32
pal_smmu_create_pasid_entry (
  UINT64 SmmuBase,
  UINT32 PasId
  )
{
    return 1;
}

/**
  @brief   This API globally disables the SMMU based on input base address
  @param   SmmuBase - Physical addr of the SMMU that needs to be globally disabled
  @return  zero for success, one for failure
**/

UINT32
pal_smmu_disable (
  UINT64 SmmuBase
  )
{
  return 0;
}

/**
  @brief   This API converts physical address to IO virtual address
  @param   SmmuBase - Physical addr of the SMMU for pa to iova conversion
  @param   Pa       - Physical address to use in conversion
  @return  zero for success, one for failure
*/
UINT64
pal_smmu_pa2iova (
  UINT64 SmmuBase,
  UINT64 Pa
  )
{
  return 0;
}
