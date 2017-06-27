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

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/AcpiTable.h>
#include "Include/IndustryStandard/Acpi61.h"
#include "Include/IndustryStandard/SerialPortConsoleRedirectionTable.h"

#include "include/pal_uefi.h"
#include "include/sbsa_pcie_enum.h"
#include "include/platform_override.h"

#define USB_CLASSCODE   0x0C0300
#define SATA_CLASSCODE  0x010600
#define BAR0            0
#define BAR1            1
#define BAR2            2

UINT64
pal_get_spcr_ptr();

/**
  @brief  This API fills in the PERIPHERAL_INFO_TABLE with information about peripherals
          in the system. This is achieved by parsing the ACPI - SPCR table and PCIe config space.

  @param  peripheralInfoTable  - Address where the Peripheral information needs to be filled.

  @return  None
**/
VOID
pal_peripheral_create_info_table(PERIPHERAL_INFO_TABLE *peripheralInfoTable)
{
  UINT32   DeviceBdf = 0;
  UINT32   StartBdf  = 0;
  PERIPHERAL_INFO_BLOCK *per_info = NULL;
  EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE *spcr = NULL;

  if (peripheralInfoTable == NULL) {
    sbsa_print(AVS_PRINT_ERR, L"Input Peripheral Table Pointer is NULL. Cannot create Peripheral INFO \n");
    return;
  }

  per_info = peripheralInfoTable->info;

  peripheralInfoTable->header.num_usb = 0;
  peripheralInfoTable->header.num_sata = 0;
  peripheralInfoTable->header.num_uart = 0;

  /* check for any USB Controllers */
  do {

       DeviceBdf = palPcieGetBdf(USB_CLASSCODE, StartBdf);
       if (DeviceBdf != 0) {
          per_info->type  = PERIPHERAL_TYPE_USB;
          per_info->base0 = palPcieGetBase(DeviceBdf, BAR0);
          per_info->bdf   = DeviceBdf;
          sbsa_print(AVS_PRINT_INFO, L"Found a USB controller %4x \n", per_info->base0);
          peripheralInfoTable->header.num_usb++;
          per_info++;
       }
       StartBdf = incrementBusDev(DeviceBdf);

  } while (DeviceBdf != 0);

  StartBdf = 0;
  /* check for any SATA Controllers */
  do {

       DeviceBdf = palPcieGetBdf(SATA_CLASSCODE, StartBdf);
       if (DeviceBdf != 0) {
          per_info->type  = PERIPHERAL_TYPE_SATA;
          per_info->base0 = palPcieGetBase(DeviceBdf, BAR0);
          per_info->bdf   = DeviceBdf;
          sbsa_print(AVS_PRINT_INFO, L"Found a SATA controller %4x \n", per_info->base0);
          peripheralInfoTable->header.num_sata++;
          per_info++;
       }
       //Increment and check if we have more controllers
       StartBdf = incrementBusDev(DeviceBdf);

  } while (DeviceBdf != 0);

  /* Search for a SPCR table in the system to get the UART details */
  spcr = (EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE *)pal_get_spcr_ptr();

  if (spcr) {
    peripheralInfoTable->header.num_uart++;
    per_info->base0 = spcr->BaseAddress.Address;
    per_info->irq   = spcr->Irq;
    per_info->type  = PERIPHERAL_TYPE_UART;
    per_info++;
  }

  if (PLATFORM_GENERIC_UART_BASE) {
    peripheralInfoTable->header.num_uart++;
    per_info->base0 = PLATFORM_GENERIC_UART_BASE;
    per_info->irq   = PLATFORM_GENERIC_UART_INTID;
    per_info->type  = PERIPHERAL_TYPE_UART;
    per_info++;
  }

  per_info->type = 0xFF; //indicate end of table

}


/**
  @brief  Check if the memory type is reserved for UEFI

  @param  EFI_MEMORY_TYPE  - Type of UEFI memory.

  @return  true   if memory reserved for UEFI usage
           false  otherwise
**/
BOOLEAN
IsUefiMemory(EFI_MEMORY_TYPE type)
{

  switch(type) {
    case  EfiReservedMemoryType:
    case  EfiLoaderCode:
    case  EfiLoaderData:
    case  EfiBootServicesCode:
    case  EfiBootServicesData:
    case  EfiRuntimeServicesCode:
    case  EfiRuntimeServicesData:
    case  EfiACPIReclaimMemory:
    case  EfiACPIMemoryNVS:
      return TRUE;
    default:
      return FALSE;
  }

}

/**
  @brief  Check if the memory type is normal

  @param  EFI_MEMORY_TYPE  - Type of UEFI memory.

  @return  true   if memory is normal
           false  otherwise
**/
BOOLEAN
IsNormalMemory(EFI_MEMORY_TYPE type)
{

  switch(type) {
    case EfiConventionalMemory:
      return TRUE;
    default:
      return FALSE;
  }

}

/**
  @brief  Check if the memory type is device

  @param  EFI_MEMORY_TYPE  - Type of UEFI memory.

  @return  true   if memory is device
           false  otherwise
**/
BOOLEAN
IsDeviceMemory(EFI_MEMORY_TYPE type)
{

  switch(type) {
    case  EfiMemoryMappedIO:
    case  EfiMemoryMappedIOPortSpace:
    case  EfiPersistentMemory:
      return TRUE;
    default:
      return FALSE;
  }
}


/**
  @brief  This API fills in the MEMORY_INFO_TABLE with information about memory in the
          system. This is achieved by parsing the UEFI memory map.

  @param  peripheralInfoTable  - Address where the Peripheral information needs to be filled.

  @return  None
**/
VOID
pal_memory_create_info_table(MEMORY_INFO_TABLE *memoryInfoTable)
{

  UINTN                 MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
  EFI_MEMORY_DESCRIPTOR *MemoryMapPtr = NULL;
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 MapKey;
  UINTN                 DescriptorSize;
  UINT32                DescriptorVersion;
  UINTN                 Pages;
  EFI_STATUS            Status;
  UINT32                Index, i = 0;

  if (memoryInfoTable == NULL) {
    sbsa_print(AVS_PRINT_ERR, L"Input Memory Table Pointer is NULL. Cannot create Memory INFO \n");
    return;
  }

// Retrieve the UEFI Memory Map

  MemoryMap = NULL;
  MemoryMapSize = 0;
  Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    // The UEFI specification advises to allocate more memory for the MemoryMap buffer between successive
    // calls to GetMemoryMap(), since allocation of the new buffer may potentially increase memory map size.
    Pages = EFI_SIZE_TO_PAGES (MemoryMapSize) + 1;
    gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData, Pages, &Address);
    MemoryMap = (EFI_MEMORY_DESCRIPTOR *)Address;
    if (MemoryMap == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      return;
    }
    Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  }

  // Go through the list and add the reserved region to the Device Tree
  if (!EFI_ERROR (Status)) {
    MemoryMapPtr = MemoryMap;
    for (Index = 0; Index < (MemoryMapSize / DescriptorSize); Index++) {
          sbsa_print(AVS_PRINT_INFO, L"Reserved region of type %d [0x%lX, 0x%lX]\n",
            MemoryMapPtr->Type, (UINTN)MemoryMapPtr->PhysicalStart,
            (UINTN)(MemoryMapPtr->PhysicalStart + MemoryMapPtr->NumberOfPages * EFI_PAGE_SIZE));
      if (IsUefiMemory ((EFI_MEMORY_TYPE)MemoryMapPtr->Type)) {
        memoryInfoTable->info[i].type      = MEMORY_TYPE_RESERVED;
      } else {
        if (IsNormalMemory ((EFI_MEMORY_TYPE)MemoryMapPtr->Type)) {
          memoryInfoTable->info[i].type      = MEMORY_TYPE_NORMAL;
        } else {
          if (IsDeviceMemory ((EFI_MEMORY_TYPE)MemoryMapPtr->Type)) {
            memoryInfoTable->info[i].type      = MEMORY_TYPE_DEVICE;
          } else {
            memoryInfoTable->info[i].type      = MEMORY_TYPE_NOT_POPULATED;
          }
        }
      }
      memoryInfoTable->info[i].phy_addr  = MemoryMapPtr->PhysicalStart;
      memoryInfoTable->info[i].virt_addr = MemoryMapPtr->VirtualStart;
      memoryInfoTable->info[i].size      = (MemoryMapPtr->NumberOfPages * EFI_PAGE_SIZE);
      i++;

      MemoryMapPtr = (EFI_MEMORY_DESCRIPTOR*)((UINTN)MemoryMapPtr + DescriptorSize);
    }
    memoryInfoTable->info[i].type      = MEMORY_TYPE_LAST_ENTRY;
  }

}

UINT64
pal_memory_ioremap(VOID *ptr, UINT32 size, UINT32 attr)
{


  return (UINT64)ptr;
}


VOID
pal_memory_unmap(VOID *ptr)
{

  return;
}
