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

#include <Library/ArmGicLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>

#include "Include/IndustryStandard/Acpi61.h"
#include <Protocol/AcpiTable.h>
#include <Protocol/HardwareInterrupt.h>
#include <Protocol/HardwareInterrupt2.h>

#include "sbsa_gic_its.h"

static UINT64 ConfigBase;

EFIAPI
EFI_STATUS
ArmGicSetItsConfigTableBase (
  IN  UINT64    GicDistributorBase,
  IN  UINT64    GicRedistributorBase
  )
{
  /* Allocate Memory for Redistributor Configuration Table */
  /* Set GICR_PROPBASER with the Config table base */
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;
  UINT32                Pages;
  UINT32                ConfigTableSize;
  UINT64                write_value;
  UINT32                gicr_propbaser_idbits;

  /* Get Memory size by reading the GICR_PROPBASER.IDBits field */
  gicr_propbaser_idbits = ARM_GICR_PROPBASER_IDbits(MmioRead64(GicRedistributorBase + ARM_GICR_PROPBASER));
  ConfigTableSize = ((1 << (gicr_propbaser_idbits+1)) - ARM_LPI_MINID);

  Pages = EFI_SIZE_TO_PAGES (ConfigTableSize) + 1;

  Status = gBS->AllocatePages (
                    AllocateAnyPages,
                    EfiBootServicesData,
                    Pages,
                    &Address
                    );
  if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "\n       ITS : Could Not Allocate Memory For Config Table. Test may not pass."));
      return Status;
  }
  ZeroMem((VOID *)Address, EFI_PAGES_TO_SIZE(Pages));

  write_value = MmioRead64(GicRedistributorBase + ARM_GICR_PROPBASER);
  write_value = write_value & (~ARM_GICR_PROPBASER_PA_MASK);
  write_value = write_value | (Address & ARM_GICR_PROPBASER_PA_MASK);
  MmioWrite64(GicRedistributorBase + ARM_GICR_PROPBASER, write_value);

  ConfigBase = Address;

  return EFI_SUCCESS;
}

EFIAPI
EFI_STATUS
ArmGicSetItsPendingTableBase (
  IN  UINT64    GicDistributorBase,
  IN  UINT64    GicRedistributorBase
  )
{
  /* Allocate Memory for Pending Table for each Redistributor*/
  /* Set GICR_PENDBASER with the Config table base */
  EFI_PHYSICAL_ADDRESS  Address;
  UINT32                Pages;
  UINT32                PendingTableSize;
  UINT64                write_value;
  UINT32                gicr_propbaser_idbits;

  /* Get Memory size by reading the GICD_TYPER.IDBits, GICR_PROPBASER.IDBits field */
  gicr_propbaser_idbits = ARM_GICR_PROPBASER_IDbits(MmioRead64(GicRedistributorBase + ARM_GICR_PROPBASER));
  PendingTableSize = ((1 << (gicr_propbaser_idbits+1))/8);

  Pages = EFI_SIZE_TO_PAGES (PendingTableSize) + 1;

  Address = (EFI_PHYSICAL_ADDRESS)AllocateAlignedPages(Pages, SIZE_64KB);
  if (!Address) {
    DEBUG ((DEBUG_ERROR, "\n       ITS : Could Not Allocate Memory For Pending Table. Test may not pass."));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem((VOID *)Address, EFI_PAGES_TO_SIZE(Pages));

  write_value = MmioRead64(GicRedistributorBase + ARM_GICR_PENDBASER);
  write_value = write_value & (~ARM_GICR_PENDBASER_PA_MASK);
  write_value = write_value | (Address & ARM_GICR_PENDBASER_PA_MASK);
  MmioWrite64(GicRedistributorBase + ARM_GICR_PENDBASER, write_value);

  return EFI_SUCCESS;
}

EFIAPI
VOID
ClearConfigTable (
  IN  UINT32          IntID
  )
{
  MmioWrite8(ConfigBase + (IntID - ARM_LPI_MINID), LPI_DISABLE);
}

EFIAPI
VOID
SetConfigTable (
  IN  UINT32          IntID,
  IN  UINT32          Priority
  )
{
  UINT8    value;

  value = (Priority & LPI_PRIORITY_MASK) | LPI_ENABLE;
  MmioWrite8(ConfigBase + (IntID - ARM_LPI_MINID), value);
}

EFIAPI
VOID
EnableLPIsRD (
  IN  UINT64    GicRedistributorBase
  )
{
  UINT32    value;
  value = MmioRead32(GicRedistributorBase + ARM_GICR_CTLR);

  MmioWrite32(GicRedistributorBase + ARM_GICR_CTLR,
              (value | ARM_GICR_CTLR_ENABLE_LPIS));
}

EFIAPI
EFI_STATUS
ArmGicRedistributorConfigurationForLPI (
  IN  UINT64    GicDistributorBase,
  IN  UINT64    GicRedistributorBase
  )
{
  EFI_STATUS    Status;
  /* Set Configuration Table Base */
  Status = ArmGicSetItsConfigTableBase(GicDistributorBase, GicRedistributorBase);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  /* Set Pending Table Base For Each Redistributor */
  Status = ArmGicSetItsPendingTableBase(GicDistributorBase, GicRedistributorBase);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  return Status;
}
