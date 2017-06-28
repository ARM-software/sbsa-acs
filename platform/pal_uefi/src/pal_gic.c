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
#include <Protocol/AcpiTable.h>
#include <Protocol/HardwareInterrupt.h>

#include "include/pal_uefi.h"

static EFI_ACPI_6_1_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *gMadtHdr;

EFI_HARDWARE_INTERRUPT_PROTOCOL *gInterrupt = NULL;


UINT64
pal_get_madt_ptr();

/**
  @brief  Populate information about the GIC sub-system at the input address.
          In a UEFI-ACPI framework, this information is part of the MADT table.

  @param  GicTable  Address of the memory region where this information is to be filled in

  @return None
**/
VOID
pal_gic_create_info_table(GIC_INFO_TABLE *GicTable)
{
  EFI_ACPI_6_1_GIC_STRUCTURE    *Entry = NULL;
  GIC_INFO_ENTRY                *GicEntry = NULL;
  UINT32                         Length= 0;
  UINT32                         TableLength;

  if (GicTable == NULL) {
    sbsa_print(AVS_PRINT_ERR, L"Input GIC Table Pointer is NULL. Cannot create GIC INFO \n");
    return;
  }

  GicEntry = GicTable->gic_info;
  GicTable->header.gic_version = 0;
  GicTable->header.num_gicrd = 0;
  GicTable->header.num_gicd = 0;
  GicTable->header.num_its = 0;

  gMadtHdr = (EFI_ACPI_6_1_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *) pal_get_madt_ptr();

  if (gMadtHdr != NULL) {
    TableLength =  gMadtHdr->Header.Length;
    sbsa_print(AVS_PRINT_INFO, L" MADT is at %x and length is %x \n", gMadtHdr, TableLength);
  } else {
    sbsa_print(AVS_PRINT_ERR, L" MADT not found \n");
    return;
  }

  Entry = (EFI_ACPI_6_1_GIC_STRUCTURE *) (gMadtHdr + 1);
  Length = sizeof (EFI_ACPI_6_1_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);


  do {

    if (Entry->Type == EFI_ACPI_6_1_GIC) {
      if (Entry->PhysicalBaseAddress != 0) {
        GicEntry->type = ENTRY_TYPE_CPUIF;
        GicEntry->base = Entry->PhysicalBaseAddress;
        sbsa_print(AVS_PRINT_INFO, L"GIC CPUIF base %x \n", GicEntry->base);
        GicEntry++;
      }

      if (Entry->GICRBaseAddress != 0) {
        GicEntry->type = ENTRY_TYPE_GICRD;
        GicEntry->base = Entry->GICRBaseAddress;
        sbsa_print(AVS_PRINT_INFO, L"GIC RD base %x \n", GicEntry->base);
        GicTable->header.num_gicrd++;
        GicEntry++;
      }
    }

    if (Entry->Type == EFI_ACPI_6_1_GICD) {
        GicEntry->type = ENTRY_TYPE_GICD;
        GicEntry->base = ((EFI_ACPI_6_1_GIC_DISTRIBUTOR_STRUCTURE *)Entry)->PhysicalBaseAddress;
        GicTable->header.gic_version = ((EFI_ACPI_6_1_GIC_DISTRIBUTOR_STRUCTURE *)Entry)->GicVersion;
        GicTable->header.num_gicd++;
        GicEntry++;
    }

    if (Entry->Type == EFI_ACPI_6_1_GIC_ITS) {
        GicEntry->type = ENTRY_TYPE_GICITS;
        GicEntry->base = ((EFI_ACPI_6_1_GIC_ITS_STRUCTURE *)Entry)->PhysicalBaseAddress;
        sbsa_print(AVS_PRINT_INFO, L"GIC ITS base %x \n", GicEntry->base);
        GicTable->header.num_its++;
        GicEntry++;
    }
    Length += Entry->Length;
    Entry = (EFI_ACPI_6_1_GIC_STRUCTURE *) ((UINT8 *)Entry + (Entry->Length));


  } while(Length < TableLength);

  GicEntry->type = 0xFF;  //Indicate end of data

}

/**
  @brief  Enable the interrupt in the GIC Distributor and GIC CPU Interface and hook
          the interrupt service routine for the IRQ to the UEFI Framework

  @param  int_id  Interrupt ID which needs to be enabled and service routine installed for
  @param  isr     Function pointer of the Interrupt service routine

  @return Status of the operation
**/
UINT32
pal_gic_install_isr(UINT32 int_id,  VOID (*isr)())
{

  EFI_STATUS  Status;

 // Find the interrupt controller protocol.
  Status = gBS->LocateProtocol (&gHardwareInterruptProtocolGuid, NULL, (VOID **)&gInterrupt);
  if (EFI_ERROR(Status)) {
    return 0xFFFFFFFF;
  }

  //First disable the interrupt to enable a clean handoff to our Interrupt handler.
  gInterrupt->DisableInterruptSource(gInterrupt, int_id);

  //Register our handler
  Status = gInterrupt->RegisterInterruptSource (gInterrupt, int_id, isr);
  if (EFI_ERROR(Status)) {
    Status =  gInterrupt->RegisterInterruptSource (gInterrupt, int_id, NULL);  //Deregister existing handler
    Status = gInterrupt->RegisterInterruptSource (gInterrupt, int_id, isr);  //register our Handler.
    //Even if this fails. there is nothing we can do in UEFI mode
  }

  return 0;
}

/**
  @brief  Indicate that processing of interrupt is complete by writing to
          End of interrupt register in the GIC CPU Interface

  @param  int_id  Interrupt ID which needs to be acknowledged that it is complete

  @return Status of the operation
**/
UINT32
pal_gic_end_of_interrupt(UINT32 int_id)
{

  EFI_STATUS  Status;

 // Find the interrupt controller protocol.
  Status = gBS->LocateProtocol (&gHardwareInterruptProtocolGuid, NULL, (VOID **)&gInterrupt);
  if (EFI_ERROR(Status)) {
    return 0xFFFFFFFF;
  }

  //EndOfInterrupt.
  gInterrupt->EndOfInterrupt(gInterrupt, int_id);

  return 0;
}

