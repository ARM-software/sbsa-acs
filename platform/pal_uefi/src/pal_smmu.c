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

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/HardwareInterrupt.h>

#include "Include/IndustryStandard/Acpi61.h"

#include "include/pal_uefi.h"
#include "include/platform_override.h"


/**
  @brief  Gather Information about SMMU subsystem
**/
VOID
pal_smmu_create_info_table(SMMU_INFO_TABLE *SmmuTable)
{
  SmmuTable->smmu_num_ctrl = 0;

  if(PLATFORM_OVERRIDE_SMMU_BASE) {
    SmmuTable->smmu_block[0].base = PLATFORM_OVERRIDE_SMMU_BASE;
    SmmuTable->smmu_block[0].arch_major_rev = PLATFORM_OVERRIDE_SMMU_ARCH_MAJOR;
    SmmuTable->smmu_num_ctrl = 1;
  }
  return;
}
