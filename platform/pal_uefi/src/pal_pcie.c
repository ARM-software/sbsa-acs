/** @file
 * Copyright (c) 2016-2023, Arm Limited or its affiliates. All rights reserved.
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
#include "Include/IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h"
#include <Protocol/HardwareInterrupt.h>

#include "Include/IndustryStandard/Pci.h"
#include "Include/IndustryStandard/Pci22.h"
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>

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
      sbsa_print(AVS_PRINT_WARN, L" ACPI - MCFG Table not found. Setting ECAM Base to 0. \n");
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
    sbsa_print(AVS_PRINT_ERR, L" Input PCIe Table Pointer is NULL. Cannot create PCIe INFO \n");
    return;
  }

  PcieTable->num_entries = 0;

  gMcfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *) pal_get_mcfg_ptr();

  if (gMcfgHdr == NULL) {
      sbsa_print(AVS_PRINT_DEBUG, L" ACPI - MCFG Table not found. \n");
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
    sbsa_print(AVS_PRINT_INFO, L" No PCI devices found in the system\n");
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
          pal_mem_free(HandleBuffer);
          if (!EFI_ERROR (Status))
            return 0;
          else
            return PCIE_NO_MAPPING;
      }
    }
  }

  pal_mem_free(HandleBuffer);
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
    sbsa_print(AVS_PRINT_INFO, L" No PCI devices found in the system\n");
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

  pal_mem_free(HandleBuffer);
}

/**
    @brief   Reads 32-bit data from BAR space pointed by Bus,
             Device, Function and register offset, using UEFI PciRootBridgeIoProtocol

    @param   Bdf     - BDF value for the device
    @param   address - BAR memory address
    @param   *data   - 32 bit value at BAR address
    @return  success/failure
**/
UINT32
pal_pcie_bar_mem_read(UINT32 Bdf, UINT64 address, UINT32 *data)
{

  EFI_STATUS                       Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *Pci;
  UINTN                            HandleCount;
  EFI_HANDLE                       *HandleBuffer;
  UINT32                           Index;
  UINT32                           InputSeg;


  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiPciRootBridgeIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    sbsa_print(AVS_PRINT_INFO,L" No Root Bridge found in the system\n");
    return PCIE_NO_MAPPING;
  }

  InputSeg = PCIE_EXTRACT_BDF_SEG(Bdf);

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciRootBridgeIoProtocolGuid, (VOID **)&Pci);
    if (!EFI_ERROR (Status)) {
      if (Pci->SegmentNumber == InputSeg) {
          Status = Pci->Mem.Read (Pci, EfiPciIoWidthUint32, address, 1, data);
          pal_mem_free(HandleBuffer);
          if (!EFI_ERROR (Status))
            return 0;
          else
            return PCIE_NO_MAPPING;
      }
    }
  }

  pal_mem_free(HandleBuffer);
  return PCIE_NO_MAPPING;
}

/**
    @brief   Write 32-bit data to BAR space pointed by Bus,
             Device, Function and register offset, using UEFI PciRootBridgeIoProtocol

    @param   Bdf     - BDF value for the device
    @param   address - BAR memory address
    @param   data    - 32 bit value to writw BAR address
    @return  success/failure
**/

UINT32
pal_pcie_bar_mem_write(UINT32 Bdf, UINT64 address, UINT32 data)
{

  EFI_STATUS                       Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *Pci;
  UINTN                            HandleCount;
  EFI_HANDLE                       *HandleBuffer;
  UINT32                           Index;
  UINT32                           InputSeg;


  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiPciRootBridgeIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    sbsa_print(AVS_PRINT_INFO,L" No Root Bridge found in the system\n");
    return PCIE_NO_MAPPING;
  }

  InputSeg = PCIE_EXTRACT_BDF_SEG(Bdf);

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciRootBridgeIoProtocolGuid, (VOID **)&Pci);
    if (!EFI_ERROR (Status)) {
      if (Pci->SegmentNumber == InputSeg) {
          Status = Pci->Mem.Write (Pci, EfiPciIoWidthUint32, address, 1, &data);
          pal_mem_free(HandleBuffer);
          if (!EFI_ERROR (Status))
            return 0;
          else
            return PCIE_NO_MAPPING;
      }
    }
  }

  pal_mem_free(HandleBuffer);
  return PCIE_NO_MAPPING;
}

/**
  @brief   This API checks the PCIe Hierarchy Supports P2P
           This is platform dependent API.If the system supports peer 2 peer
           traffic, return 0 else return 1
           1. Caller       -  Test Suite
  @return  1 - P2P feature not supported 0 - P2P feature supported
**/
UINT32
pal_pcie_p2p_support()
{
  /*
   * This is platform specific API which needs to be populated with system p2p capability
   * PCIe support for peer to peer
   * transactions is platform implementation specific
   */
  if (g_pcie_p2p)
      return 0;
  else
      return NOT_IMPLEMENTED;
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
   * This is platform specific API which needs to be populated with pcie device  p2p capability
   * Root port or Switch support for peer to peer
   * transactions is platform implementation specific
   */

  return 1;
}

/**
    @brief   Create a list of MSI(X) vectors for a device

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @param   mvector    pointer to a MSI(X) list address

    @return  mvector    list of MSI(X) vectors
    @return  number of MSI(X) vectors
**/
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

/**
    @brief   Get legacy IRQ routing for a PCI device
             This is Platform dependent API and needs to be filled
             with legacy IRQ map for a pcie devices.
    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @param   irq_map    pointer to IRQ map structure

    @return  irq_map    IRQ routing map
    @return  status code If the device legacy irq map information is filled
                         return 0, else returns NOT_IMPLEMENTED
**/
UINT32
pal_pcie_get_legacy_irq_map (
  UINT32 Seg,
  UINT32 Bus,
  UINT32 Dev,
  UINT32 Fn,
  PERIPHERAL_IRQ_MAP *IrqMap
  )
{
  return NOT_IMPLEMENTED;
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
  @brief   Checks the Address Translation Cache Support for BDF
           Platform dependent API. Fill this with system ATC support
           information for bdf's
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
  if (g_pcie_cache_present)
      return 1;
  else
      return NOT_IMPLEMENTED;
}

/**
    @brief   Gets RP support of transaction forwarding.

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @param   seg        PCI segment number

    @return  0 if rp not involved in transaction forwarding
             1 if rp is involved in transaction forwarding
**/
UINT32
pal_pcie_get_rp_transaction_frwd_support(UINT32 seg, UINT32 bus, UINT32 dev, UINT32 fn)
{
  return 1;
}

/**
  @brief  Returns whether a PCIe Function is an on-chip peripheral or not

  @param  bdf        - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return Returns TRUE if the Function is on-chip peripheral, FALSE if it is
          not an on-chip peripheral
**/
UINT32
pal_pcie_is_onchip_peripheral(UINT32 bdf)
{
  return 0;
}

/**
  @brief  Checks the discovered PCIe hierarchy is matching with the
          topology described in info table.
  @return Returns 0 if device entries matches , 1 if there is mismatch.
**/
UINT32
pal_pcie_check_device_list(void)
{
  return 0;
}

/**
  @brief  Returns the memory offset that can be
          accessed from the BAR base and is within
          BAR limit value

  @param  type
  @return memory offset
**/
UINT32
pal_pcie_mem_get_offset(UINT32 type)
{

  return MEM_OFFSET_SMALL;
}
