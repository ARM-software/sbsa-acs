/** @file
 * Copyright (c) 2017, ARM Limited or its affiliates. All rights reserved.

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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "Include/IndustryStandard/Acpi61.h"
#include "Include/IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h"
#include <Protocol/AcpiTable.h>
#include <Protocol/HardwareInterrupt.h>

#include "include/platform_override.h"
#include "include/pal_uefi.h"

static EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *gMcfgHdr;

UINT64
pal_get_mcfg_ptr();

/**
  @brief  Returns the PCI ECAM address from the ACPI MCFG Table address

  @param  None

  @return  None
**/
UINT64
pal_pcie_get_mcfg_ecam()
{
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE  *Entry;

  gMcfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *) pal_get_mcfg_ptr();

  if (gMcfgHdr == NULL) {
      sbsa_print(AVS_PRINT_WARN, L"ACPI - MCFG Table not found. Setting ECAM Base to 0. \n");
      return 0x0;
  }

  Entry = (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *) (gMcfgHdr + 1);

  return (Entry->BaseAddress);
}


/**
  @brief  Fill the PCIE Info table with the details of the PCIe sub-system

  @param  PcieTable - Address where the PCIe information needs to be filled.

  @return  None
 **/
VOID
pal_pcie_create_info_table(PCIE_INFO_TABLE *PcieTable)
{

  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE  *Entry = NULL;
  UINT32 length = sizeof(EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER);
  UINT32 i = 0;

  if (PcieTable == NULL) {
    sbsa_print(AVS_PRINT_ERR, L"Input PCIe Table Pointer is NULL. Cannot create PCIe INFO \n");
    return;
  }

  PcieTable->num_entries = 0;

  gMcfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *) pal_get_mcfg_ptr();

  if (gMcfgHdr == NULL) {
      sbsa_print(AVS_PRINT_DEBUG, L"ACPI - MCFG Table not found. \n");
      return;
  }

  if(PLATFORM_OVERRIDE_PCIE_ECAM_BASE) {
      PcieTable->block[i].ecam_base = PLATFORM_OVERRIDE_PCIE_ECAM_BASE;
      PcieTable->block[i].start_bus_num = PLATFORM_OVERRIDE_PCIE_START_BUS_NUM;
      PcieTable->block[i].segment_num = 0;
      PcieTable->num_entries = 1;
      return;
  }

  Entry = (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *) (gMcfgHdr + 1);

  do{
      if (Entry == NULL)  //Due to a buggy MCFG - first entry is null, then exit
          break;
      PcieTable->block[i].ecam_base     = Entry->BaseAddress;
      PcieTable->block[i].segment_num   = Entry->PciSegmentGroupNumber;
      PcieTable->block[i].start_bus_num = Entry->StartBusNumber;
      PcieTable->block[i].end_bus_num   = Entry->EndBusNumber;
      length += sizeof(EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE);
      Entry++;
      i++;
      PcieTable->num_entries++;
  } while((length < gMcfgHdr->Header.Length) && (Entry));

  return;
}
