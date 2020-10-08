/** @file
 * Copyright (c) 2016-2020, Arm Limited or its affiliates. All rights reserved.
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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "Include/IndustryStandard/Acpi61.h"
#include "Include/IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h"
#include <Protocol/AcpiTable.h>
#include <Protocol/HardwareInterrupt.h>

#include "Include/IndustryStandard/Pci.h"
#include "Include/IndustryStandard/Pci22.h"
#include <Protocol/PciIo.h>

#include "include/platform_override.h"
#include "include/pal_uefi.h"
#include "include/sbsa_pcie_enum.h"

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

/**
    @brief   Reads 32-bit data from PCIe config space pointed by Bus,
           Device, Function and register offset, using UEFI PciIoProtocol

    @param   Bdf      - BDF value for the device
    @param   offset - Register offset within a device PCIe config space
    @param   *data - 32 bit value at offset from ECAM base of the device specified by BDF value
    @return  success/failure
**/
UINT32
pal_pcie_io_read_cfg(UINT32 Bdf, UINT32 offset, UINT32 *data)
{

  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *Pci;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         Seg, Bus, Dev, Func;
  UINT32                        Index;
  UINT32                        InputSeg, InputBus, InputDev, InputFunc;


  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiPciIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    sbsa_print(AVS_PRINT_INFO,L"No PCI devices found in the system\n");
    return PCIE_NO_MAPPING;
  }

  InputSeg = PCIE_EXTRACT_BDF_SEG(Bdf);
  InputBus = PCIE_EXTRACT_BDF_BUS(Bdf);
  InputDev = PCIE_EXTRACT_BDF_DEV(Bdf);
  InputFunc = PCIE_EXTRACT_BDF_FUNC(Bdf);

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID **)&Pci);
    if (!EFI_ERROR (Status)) {
      Pci->GetLocation (Pci, &Seg, &Bus, &Dev, &Func);
      if (InputSeg == Seg && InputBus == Bus && InputDev == Dev && InputFunc == Func) {
          Status = Pci->Pci.Read (Pci, EfiPciIoWidthUint32, offset, 1, data);
          if (!EFI_ERROR (Status))
            return 0;
          else
            return PCIE_NO_MAPPING;
      }
    }
  }
  return PCIE_NO_MAPPING;
}

/**
    @brief Write 32-bit data to PCIe config space pointed by Bus,
           Device, Function and register offset, using UEFI PciIoProtocol

    @param   Bdf      - BDF value for the device
    @param   offset - Register offset within a device PCIe config space
    @param   data - 32 bit value at offset from ECAM base of the device specified by BDF value
    @return  success/failure
**/
VOID
pal_pcie_io_write_cfg(UINT32 Bdf, UINT32 offset, UINT32 data)
{

  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *Pci;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         Seg, Bus, Dev, Func;
  UINT32                        Index;
  UINT32                        InputSeg, InputBus, InputDev, InputFunc;


  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiPciIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    sbsa_print(AVS_PRINT_INFO,L"No PCI devices found in the system\n");
    return;
  }

  InputSeg = PCIE_EXTRACT_BDF_SEG(Bdf);
  InputBus = PCIE_EXTRACT_BDF_BUS(Bdf);
  InputDev = PCIE_EXTRACT_BDF_DEV(Bdf);
  InputFunc = PCIE_EXTRACT_BDF_FUNC(Bdf);

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID **)&Pci);
    if (!EFI_ERROR (Status)) {
      Pci->GetLocation (Pci, &Seg, &Bus, &Dev, &Func);
      if (InputSeg == Seg && InputBus == Bus && InputDev == Dev && InputFunc == Func) {
          Status = Pci->Pci.Write (Pci, EfiPciIoWidthUint32, offset, 1, &data);
      }
    }
  }
}

/**
  @brief   This API checks the PCIe Hierarchy Supports P2P
           1. Caller       -  Test Suite
  @return  1 - P2P feature not supported 0 - P2P feature supported
**/
UINT32
pal_pcie_p2p_support()
{
  /*
   * TODO
   * PCIe support for peer to peer
   * transactions is platform implementation specific
   */

  return 1;
}

/**
  @brief   This API checks the PCIe device P2P support
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  1 - P2P feature not supported 0 - P2P feature supported
**/
UINT32
pal_pcie_dev_p2p_support (
  UINT32 Seg,
  UINT32 Bus,
  UINT32 Dev,
  UINT32 Fn)
{
  /*
   * TODO
   * Root port or Switch support for peer to peer
   * transactions is platform implementation specific
   */

  return 1;
}

/* Place holder function. Need to be
 * implemented if needed in later releases
 */
UINT32
pal_get_msi_vectors (
  UINT32 Seg,
  UINT32 Bus,
  UINT32 Dev,
  UINT32 Fn,
  PERIPHERAL_VECTOR_LIST **MVector
  )
{
  return 0;
}

/* Place holder function. Need to be
 * implemented if needed in later releases
 */
UINT32
pal_pcie_get_legacy_irq_map (
  UINT32 Seg,
  UINT32 Bus,
  UINT32 Dev,
  UINT32 Fn,
  PERIPHERAL_IRQ_MAP *IrqMap
  )
{
  return 0;
}

/* Place holder function. Need to be
 * implemented if needed in later releases
 */
UINT32
pal_pcie_get_root_port_bdf (
  UINT32 *Seg,
  UINT32 *Bus,
  UINT32 *Dev,
  UINT32 *Func
  )
{
  return 0;
}

/**
  @brief   Platform dependent API checks the Address Translation
           Cache Support for BDF
           1. Caller       -  Test Suite
  @return  0 - ATC not supported 1 - ATC supported
**/
UINT32
pal_pcie_is_cache_present (
  UINT32 Seg,
  UINT32 Bus,
  UINT32 Dev,
  UINT32 Fn
  )
{
  return 0;
}
