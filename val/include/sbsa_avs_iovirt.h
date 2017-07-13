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

#ifndef __SBSA_AVS_IOVIRT_H__
#define __SBSA_AVS_IOVIRT_H__

uint64_t val_iovirt_get_smmu_info(SMMU_INFO_e type, uint32_t index);
uint32_t val_iovirt_check_unique_ctx_intid(uint32_t smmu_index);
uint32_t val_iovirt_unique_rid_strid_map(uint32_t rc_index);
int val_iovirt_get_device_id(uint32_t rid, uint32_t segment, uint32_t *device_id, uint32_t *stream_id);
uint64_t val_iovirt_get_pcie_rc_info(PCIE_RC_INFO_e type, uint32_t index);

#endif
