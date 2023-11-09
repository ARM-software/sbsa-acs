/** @file
 * Copyright (c) 2020-2023, Arm Limited or its affiliates. All rights reserved.
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

#include "pal_common_support.h"
#include "platform_override_struct.h"

extern PLATFORM_OVERRIDE_GIC_INFO_TABLE platform_gic_cfg;

/**
  @brief  Populate information about the GIC sub-system at the input address.

  @param  GicTable  Address of the memory region where this information is to be filled in

  @return None
**/
void
pal_gic_create_info_table(GIC_INFO_TABLE *GicTable)
{

  uint32_t Index;
  uint32_t InfoIndex = 0;

  if (GicTable == NULL) {
    return;
  }

  GicTable->header.gic_version    = platform_gic_cfg.gic_version;
  GicTable->header.num_gicrd      = platform_gic_cfg.num_gicrd;
  GicTable->header.num_gicd       = platform_gic_cfg.num_gicd;
  GicTable->header.num_its        = platform_gic_cfg.num_gicits;
  GicTable->header.num_gich       = platform_gic_cfg.num_gich;
  GicTable->header.num_msi_frames  = platform_gic_cfg.num_msiframes;

  for (Index = 0; Index < platform_gic_cfg.num_gicc; Index++) {
    GicTable->gic_info[InfoIndex].type   = PLATFORM_OVERRIDE_GICC_TYPE;
    GicTable->gic_info[InfoIndex++].base = platform_gic_cfg.gicc_base[Index];
  }

  for (Index = 0; Index < platform_gic_cfg.num_gicrd; Index++) {
    GicTable->gic_info[InfoIndex].type     = PLATFORM_OVERRIDE_GICR_GICRD_TYPE;
    GicTable->gic_info[InfoIndex].base     = platform_gic_cfg.gicrd_base[Index];
    GicTable->gic_info[InfoIndex++].length = platform_gic_cfg.gicrd_length;
  }

  for (Index = 0; Index < platform_gic_cfg.num_gicd; Index++) {
    GicTable->gic_info[InfoIndex].type   = PLATFORM_OVERRIDE_GICD_TYPE;
    GicTable->gic_info[InfoIndex++].base = platform_gic_cfg.gicd_base[Index];
  }

  for (Index = 0; Index < platform_gic_cfg.num_gicits; Index++) {
    GicTable->gic_info[InfoIndex].type     = PLATFORM_OVERRIDE_GICITS_TYPE;
    GicTable->gic_info[InfoIndex].base     = platform_gic_cfg.gicits_base[Index];
    GicTable->gic_info[InfoIndex++].entry_id = platform_gic_cfg.gicits_id[Index];
  }

  for (Index = 0; Index < platform_gic_cfg.num_gich; Index++) {
    GicTable->gic_info[InfoIndex].type     = PLATFORM_OVERRIDE_GICH_TYPE;
    GicTable->gic_info[InfoIndex].base     = platform_gic_cfg.gich_base[Index];
    GicTable->gic_info[InfoIndex++].length = 0;
  }

  for (Index = 0; Index < platform_gic_cfg.num_msiframes; Index++) {
    GicTable->gic_info[InfoIndex].type       = PLATFORM_OVERRIDE_GICMSIFRAME_TYPE;
    GicTable->gic_info[InfoIndex].base       = platform_gic_cfg.gicmsiframe_base[Index];
    GicTable->gic_info[InfoIndex].entry_id   = platform_gic_cfg.gicmsiframe_id[Index];
    GicTable->gic_info[InfoIndex].flags      = platform_gic_cfg.gicmsiframe_flags[Index];
    GicTable->gic_info[InfoIndex].spi_count  = platform_gic_cfg.gicmsiframe_spi_count[Index];
    GicTable->gic_info[InfoIndex++].spi_base   = platform_gic_cfg.gicmsiframe_spi_base[Index];
  }

  GicTable->gic_info[InfoIndex].type = 0xFF;  //Indicate end of data

}
