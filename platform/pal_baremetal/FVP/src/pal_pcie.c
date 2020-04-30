/** @file
 * Copyright (c) 2020, Arm Limited or its affiliates. All rights reserved.
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
#include "include/platform_override_fvp.h"

extern PCIE_INFO_TABLE platform_pcie_cfg;

void
pal_pcie_create_info_table(PCIE_INFO_TABLE *PcieTable)
{

  uint32_t i = 0;

  if (PcieTable == NULL) {
    print(AVS_PRINT_ERR, "Input PCIe Table Pointer is NULL. Cannot create PCIe INFO \n");
    return;
  }

  PcieTable->num_entries = 0;

  if(platform_pcie_cfg.num_entries == 0) {
    print(AVS_PRINT_ERR, "Number of ECAM is 0. Cannot create PCIe INFO \n");
    return;
  }


  for(i = 0; i < platform_pcie_cfg.num_entries; i++)
  {
      PcieTable->block[i].ecam_base      = platform_pcie_cfg.block[i].ecam_base;
      PcieTable->block[i].bar_start_addr = platform_pcie_cfg.block[i].bar_start_addr;
      PcieTable->block[i].segment_num    = platform_pcie_cfg.block[i].segment_num;
      PcieTable->block[i].start_bus_num  = platform_pcie_cfg.block[i].start_bus_num;
      PcieTable->block[i].end_bus_num    = platform_pcie_cfg.block[i].end_bus_num;
      PcieTable->num_entries++;
   }
  return;
}
