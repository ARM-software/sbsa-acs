/** @file
 * Copyright (c) 2020-2023 Arm Limited or its affiliates. All rights reserved.
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


#include "FVP/RDN2/include/platform_override_struct.h"
#include "include/pal_common_support.h"
#include "include/pal_pcie_enum.h"

extern DMA_INFO_TABLE platform_dma_cfg;

/**
  @brief   Populate DMA_INFO_TABLE with the information of DMA Controllers
           in the system.

  @param   Pointer to the DMA_INFO_TABLE data structure

  @return  None
**/
void
pal_dma_create_info_table(DMA_INFO_TABLE *dma_info_table)
{

    uint32_t i;
    dma_info_table->num_dma_ctrls = platform_dma_cfg.num_dma_ctrls;

    for (i = 0; i < dma_info_table->num_dma_ctrls; i++)
    {
       dma_info_table->info[i].host   = platform_dma_cfg.info[i].host;
       dma_info_table->info[i].port   = platform_dma_cfg.info[i].port;
       dma_info_table->info[i].target = platform_dma_cfg.info[i].target;
       dma_info_table->info[i].flags  = platform_dma_cfg.info[i].flags;
       dma_info_table->info[i].type   = platform_dma_cfg.info[i].type;
    }
}
