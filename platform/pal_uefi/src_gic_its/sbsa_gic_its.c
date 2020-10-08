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
#include <Library/ArmLib.h>
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
#include "include/pal_uefi.h"

UINT64 ArmReadMpidr(VOID);

extern GIC_ITS_INFO    *g_gic_its_info;
static UINT32          cwriter_ptr[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

UINT64
GetCurrentCpuRDBase (
  UINT64     mGicRedistributorBase
  )
{
  UINT32     Index;
  UINT64     Mpidr;
  UINT32     Affinity, CpuAffinity;
  UINT32     GicRedistributorGranularity;
  UINT64     GicCpuRedistributorBase;
  UINT32     num_pe;

  Mpidr = ArmReadMpidr();

  CpuAffinity = (Mpidr & (ARM_CORE_AFF0 | ARM_CORE_AFF1 | ARM_CORE_AFF2)) |
                ((Mpidr & ARM_CORE_AFF3) >> 8);

  GicRedistributorGranularity = ARM_GICR_CTLR_FRAME_SIZE
                                  + ARM_GICR_SGI_PPI_FRAME_SIZE;

  GicCpuRedistributorBase = mGicRedistributorBase;
  num_pe = pal_pe_get_num();

  for (Index = 0; Index < num_pe; Index++) {
    Affinity = MmioRead32(GicCpuRedistributorBase + ARM_GICR_TYPER + NEXT_DW_OFFSET);
    if (Affinity == CpuAffinity) {
      return GicCpuRedistributorBase;
    }

    /* Move to the next GIC Redistributor frame */
    GicCpuRedistributorBase += GicRedistributorGranularity;
  }

  return 0;
}

EFIAPI
UINT32
ArmGICDSupportsLPIs (
  IN  UINT64    GicDistributorBase
  )
{
  return (MmioRead32(GicDistributorBase + ARM_GICD_TYPER) & ARM_GICD_TYPER_LPIS);
}

EFIAPI
UINT32
ArmGICRSupportsLPIs (
  IN  UINT64    GicRedistributorBase
  )
{
  return (MmioRead32(GicRedistributorBase + ARM_GICR_TYPER) & ARM_GICR_TYPER_PLPIS);
}

EFIAPI
EFI_STATUS
ArmGicSetItsCommandQueueBase (
  IN  UINT32     ItsIndex
  )
{
  /* Allocate Memory for Command queue. Set command queue base in GITS_CBASER. */
  EFI_PHYSICAL_ADDRESS      Address;
  UINT64                    write_value;
  UINT64                    ItsBase;

  ItsBase = g_gic_its_info->GicIts[ItsIndex].Base;

  Address = (EFI_PHYSICAL_ADDRESS)AllocateAlignedPages(EFI_SIZE_TO_PAGES(NUM_PAGES_8 * SIZE_4KB), SIZE_64KB);
  if (!Address) {
    DEBUG ((DEBUG_ERROR, "\n       ITS : Could Not Allocate Memory For Command Q. Test may not pass."));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem((VOID *)Address, (NUM_PAGES_8*SIZE_4KB));

  g_gic_its_info->GicIts[ItsIndex].CommandQBase = Address;
  DEBUG((DEBUG_INFO, "%a Address Allocated : %x\n", __func__, Address));

  write_value = MmioRead64(ItsBase + ARM_GITS_CBASER) & (~ARM_GITS_CBASER_PA_MASK);
  write_value = write_value | (Address & ARM_GITS_CBASER_PA_MASK);
  write_value = write_value | ARM_GITS_CBASER_VALID;
  MmioWrite64(ItsBase + ARM_GITS_CBASER, write_value);

  return EFI_SUCCESS;
}

EFIAPI
EFI_STATUS
ArmGicSetItsTables (
  IN  UINT32     ItsIndex
  )
{
  UINT32                Pages;
  UINT32                TableSize, entry_size;
  UINT64                its_baser, its_typer;
  UINT8                 it, table_type;
  UINT64                write_value;
  UINT32                DevBits, CIDBits;
  EFI_PHYSICAL_ADDRESS  Address;
  UINT64                ItsBase;

  ItsBase = g_gic_its_info->GicIts[ItsIndex].Base;

  /* Allocate Memory for Table Depending on the Type of the table in GITS_BASER<n>. */
  for(it=0; it<ARM_NUM_GITS_BASER; it++) {

    its_baser = MmioRead64(ItsBase + ARM_GITS_BASER(it));
    table_type = ARM_GITS_BASER_GET_TYPE(its_baser);
    entry_size = ARM_GITS_BASER_GET_ENTRY_SIZE(its_baser);

    its_typer = MmioRead64(ItsBase + ARM_GITS_TYPER);
    DevBits = ARM_GITS_TYPER_DevBits(its_typer);
    CIDBits = ARM_GITS_TYPER_CIDBits(its_typer);

    if (table_type == ARM_GITS_TBL_TYPE_DEVICE) {
      TableSize = (1 << (DevBits+1))*(entry_size+1); // Assuming Single Level Table

    } else if (table_type == ARM_GITS_TBL_TYPE_CLCN) {
      TableSize = (1 << (CIDBits+1))*(entry_size+1); // Assuming Single Level Table

    } else {
      continue;
    }

    Pages = EFI_SIZE_TO_PAGES (TableSize);
    Address = (EFI_PHYSICAL_ADDRESS)AllocateAlignedPages(Pages, SIZE_64KB);
    if (!Address) {
      DEBUG ((DEBUG_ERROR, "\n       ITS : Could Not Allocate Memory For DT/CT. Test may not pass."));
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem((VOID *)Address, EFI_PAGES_TO_SIZE(Pages));

    write_value = MmioRead64(ItsBase + ARM_GITS_BASER(it));
    write_value = write_value & (~ARM_GITS_BASER_PA_MASK);
    write_value = write_value | (Address & ARM_GITS_BASER_PA_MASK);
    write_value = write_value | ARM_GITS_BASER_VALID;
    write_value = write_value | (Pages-1);
    MmioWrite64(ItsBase + ARM_GITS_BASER(it), write_value);

  }

  /* Allocate Memory for Interrupt Translation Table */
  Address = (EFI_PHYSICAL_ADDRESS)AllocateAlignedPages(EFI_SIZE_TO_PAGES(NUM_PAGES_8 * SIZE_4KB), SIZE_64KB);
  if (!Address) {
    DEBUG ((DEBUG_ERROR, "\n       ITS : Could Not Allocate Memory For ITT. Test may not pass."));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem((VOID *)Address, (NUM_PAGES_8*SIZE_4KB));

  g_gic_its_info->GicIts[ItsIndex].ITTBase = Address;

  return EFI_SUCCESS;
}

EFIAPI
VOID
EnableITS (
  IN  UINT64    GicItsBase
  )
{
  /* Set GITS_CTLR.Enable as 1 to enable the ITS */
  UINT32    value;

  value = MmioRead32(GicItsBase + ARM_GITS_CTLR);
  MmioWrite32(GicItsBase + ARM_GITS_CTLR, (value | ARM_GITS_CTLR_ENABLE));
}

VOID
WriteCmdQMAPD (
  IN UINT32     ItsIndex,
  IN UINT64     *CMDQ_BASE,
  IN UINT64     DevID,
  IN UINT64     ITT_BASE,
  IN UINT32     Size,
  IN UINT64     Valid
  )
{
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex]), (UINT64)((DevID << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_MAPD));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 1), (UINT64)(Size));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 2), (UINT64)((Valid << ITS_CMD_SHIFT_VALID) | (ITT_BASE & ITT_PAR_MASK)));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 3), (UINT64)(0x0));
    cwriter_ptr[ItsIndex] = cwriter_ptr[ItsIndex] + ITS_NEXT_CMD_PTR;
}

VOID
WriteCmdQMAPC (
  IN UINT32     ItsIndex,
  IN UINT64     *CMDQ_BASE,
  IN UINT32     DevID,
  IN UINT32     Clctn_ID,
  IN UINT32     RDBase,
  IN UINT64     Valid
  )
{
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] ), (UINT64)(ARM_ITS_CMD_MAPC));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 1), (UINT64)(0x0));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 2), (UINT64)((Valid << ITS_CMD_SHIFT_VALID) | RDBase | Clctn_ID));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 3), (UINT64)(0x0));
    cwriter_ptr[ItsIndex] = cwriter_ptr[ItsIndex] + ITS_NEXT_CMD_PTR;
}

VOID
WriteCmdQMAPI (
  IN UINT32     ItsIndex,
  IN UINT64     *CMDQ_BASE,
  IN UINT64     DevID,
  IN UINT32     IntID,
  IN UINT32     Clctn_ID
  )
{
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex]), (UINT64)((DevID << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_MAPI));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 1), (UINT64)(IntID));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 2), (UINT64)(Clctn_ID));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 3), (UINT64)(0x0));
    cwriter_ptr[ItsIndex] = cwriter_ptr[ItsIndex] + ITS_NEXT_CMD_PTR;
}

VOID
WriteCmdQINV (
  IN UINT32     ItsIndex,
  IN UINT64     *CMDQ_BASE,
  IN UINT64     DevID,
  IN UINT32     IntID
  )
{
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex]), (UINT64)((DevID << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_INV));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 1), (UINT64)(IntID));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 2), (UINT64)(0x0));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 3), (UINT64)(0x0));
    cwriter_ptr[ItsIndex] = cwriter_ptr[ItsIndex] + ITS_NEXT_CMD_PTR;
}

VOID
WriteCmdQDISCARD (
  IN UINT32     ItsIndex,
  IN UINT64     *CMDQ_BASE,
  IN UINT64     DevID,
  IN UINT32     IntID
  )
{
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex]), (UINT64)((DevID << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_DISCARD));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 1), (UINT64)(IntID));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 2), (UINT64)(0x0));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 3), (UINT64)(0x0));
    cwriter_ptr[ItsIndex] = cwriter_ptr[ItsIndex] + ITS_NEXT_CMD_PTR;
}


VOID
WriteCmdQSYNC (
  IN UINT32     ItsIndex,
  IN UINT64     *CMDQ_BASE,
  IN UINT32     RDBase
  )
{
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex]), (UINT64)(ARM_ITS_CMD_SYNC));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 1), (UINT64)(0x0));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 2), (UINT64)(RDBase));
    MmioWrite64((UINT64)(CMDQ_BASE + cwriter_ptr[ItsIndex] + 3), (UINT64)(0x0));
    cwriter_ptr[ItsIndex] = cwriter_ptr[ItsIndex] + ITS_NEXT_CMD_PTR;
}

VOID
PollTillCommandQueueDone (
  IN UINT32     ItsIndex
  )
{
  UINT32    count;
  UINT64    creadr_value;
  UINT64    stall_value;
  UINT64    cwriter_value;
  UINT64    ItsBase;

  count = 0;
  ItsBase = g_gic_its_info->GicIts[ItsIndex].Base;
  cwriter_value = MmioRead64(ItsBase + ARM_GITS_CWRITER);
  creadr_value = MmioRead64(ItsBase + ARM_GITS_CREADR);

  while (creadr_value != cwriter_value) {
    /* Check Stall Value */
    stall_value = creadr_value & ARM_GITS_CREADR_STALL;

    if (stall_value) {
      /* Retry */
      MmioWrite64((ItsBase + ARM_GITS_CWRITER),
                  (cwriter_value | ARM_GITS_CWRITER_RETRY)
                 );
    }

    count++;
    if (count > WAIT_ITS_COMMAND_DONE) {
      DEBUG ((DEBUG_ERROR, "\n       ITS : Command Queue READR not moving, Test may not pass."));
      break;
    }

    creadr_value = MmioRead64(ItsBase + ARM_GITS_CREADR);
  }

}

UINT64
GetRDBaseFormat (
  IN UINT32     ItsIndex
  )
{
  UINT32    value;
  UINT64    ItsBase;

  ItsBase = g_gic_its_info->GicIts[ItsIndex].Base;

  /* Check GITS_TYPER.PTA.
     If PTA = 1 then RDBase = Physical Address,
     Else RDBase = GICR_TYPER.Processor_Number
  */
  value = MmioRead64(ItsBase + ARM_GITS_TYPER);
  if (value & ARM_GITS_TYPER_PTA) {
    return g_gic_its_info->GicRdBase;
  } else {
    value = MmioRead64(g_gic_its_info->GicRdBase + ARM_GICR_TYPER);
    return (value & ARM_GICR_TYPER_PN_MASK);
  }
}

EFIAPI
VOID
ArmGicItsClearLpiMappings (
  IN UINT32     ItsIndex,
  IN UINT32     DevID,
  IN UINT32     IntID
  )
{
  UINT64    value;
  UINT64    RDBase;
  UINT64    ItsBase;
  UINT64    ItsCommandBase;

  ItsBase        = g_gic_its_info->GicIts[ItsIndex].Base;
  ItsCommandBase = g_gic_its_info->GicIts[ItsIndex].CommandQBase;

  /* Clear Config table for LPI=IntID */
  ClearConfigTable(IntID);

  /* Get RDBase Depending on GITS_TYPER.PTA */
  RDBase = GetRDBaseFormat(ItsIndex);

  /* Discard Mappings */
  WriteCmdQDISCARD(ItsIndex, (UINT64 *)(ItsCommandBase), DevID, IntID);
  /* ITS SYNC Command */
  WriteCmdQSYNC(ItsIndex, (UINT64 *)(ItsCommandBase), RDBase);

  /* Update the CWRITER Register so that all the commands from Command queue gets executed.*/
  value = ((cwriter_ptr[ItsIndex] * NUM_BYTES_IN_DW));
  MmioWrite64((ItsBase + ARM_GITS_CWRITER), value);

  /* Check CREADR value which ensures Command Queue is processed */
  PollTillCommandQueueDone(ItsIndex);

}

EFIAPI
VOID
ArmGicItsCreateLpiMap (
  IN UINT32     ItsIndex,
  IN UINT32     DevID,
  IN UINT32     IntID,
  IN UINT32     Priority
  )
{
  UINT64    value;
  UINT64    RDBase;
  UINT64    ItsBase;
  UINT64    ItsCommandBase;

  ItsBase        = g_gic_its_info->GicIts[ItsIndex].Base;
  ItsCommandBase = g_gic_its_info->GicIts[ItsIndex].CommandQBase;

  /* Set Config table with enable the LPI = IntID, Priority. */
  SetConfigTable(IntID, Priority);

  /* Enable Redistributor */
  EnableLPIsRD(g_gic_its_info->GicRdBase);

  /* Enable ITS */
  EnableITS(ItsBase);

  /* Get RDBase Depending on GITS_TYPER.PTA */
  RDBase = GetRDBaseFormat(ItsIndex);

  /* Map Device using MAPD */
  WriteCmdQMAPD(ItsIndex, (UINT64 *)(ItsCommandBase), DevID,
                g_gic_its_info->GicIts[ItsIndex].ITTBase,
                g_gic_its_info->GicIts[ItsIndex].IDBits, 0x1 /*Valid*/);
  /* Map Collection using MAPC */
  WriteCmdQMAPC(ItsIndex, (UINT64 *)(ItsCommandBase), DevID, 0x1 /*Clctn_ID*/, RDBase, 0x1 /*Valid*/);
  /* Map Interrupt using MAPI */
  WriteCmdQMAPI(ItsIndex, (UINT64 *)(ItsCommandBase), DevID, IntID, 0x1 /*Clctn_ID*/);
  /* Invalid Entry */
  WriteCmdQINV(ItsIndex, (UINT64 *)(ItsCommandBase), DevID, IntID);
  /* ITS SYNC Command */
  WriteCmdQSYNC(ItsIndex, (UINT64 *)(ItsCommandBase), RDBase);

  /* Update the CWRITER Register so that all the commands from Command queue gets executed.*/
  value = ((cwriter_ptr[ItsIndex] * NUM_BYTES_IN_DW));
  MmioWrite64((ItsBase + ARM_GITS_CWRITER), value);

  /* Check CREADR value which ensures Command Queue is processed */
  PollTillCommandQueueDone(ItsIndex);

}

EFIAPI
UINT32
ArmGicItsGetMaxLpiID (
  )
{
  UINT32    index;
  UINT32    min_idbits = ARM_LPI_MAX_IDBITS;

  if (g_gic_its_info->GicNumIts == 0)
    return 0;

  /* Return The Minimum IDBits supported in ITS */
  for (index=0; index<g_gic_its_info->GicNumIts; index++)
  {
    min_idbits = (min_idbits < g_gic_its_info->GicIts[index].IDBits)?
                 (min_idbits):
                 (g_gic_its_info->GicIts[index].IDBits);
  }
  return ((1 << (min_idbits+1)) - 1);
}

EFIAPI
UINT64
ArmGicItsGetGITSTranslatorAddress (
  IN UINT32     ItsIndex
  )
{
  return (g_gic_its_info->GicIts[ItsIndex].Base + ARM_GITS_TRANSLATER);
}

EFIAPI
EFI_STATUS
SetInitialConfiguration (
  UINT32     ItsIndex
  )
{
  /* Program GIC Redistributor with the Min ID bits supported. */
  UINT32    gicd_typer_idbits, gits_typer_bits;
  UINT64    write_value;
  UINT64    ItsBase;

  ItsBase = g_gic_its_info->GicIts[ItsIndex].Base;

  gicd_typer_idbits = ARM_GICD_TYPER_IDbits(MmioRead32(g_gic_its_info->GicDBase + ARM_GICD_TYPER));
  gits_typer_bits = ARM_GITS_TYPER_IDbits(MmioRead64(ItsBase + ARM_GITS_TYPER));

  /* Check least bits implemented is 14 if LPIs are supported. */
  if (GET_MIN(gicd_typer_idbits, gits_typer_bits) < ARM_LPI_MIN_IDBITS) {
    return EFI_UNSUPPORTED;
  }

  write_value = MmioRead64(g_gic_its_info->GicRdBase + ARM_GICR_PROPBASER);
  write_value |= GET_MIN(gicd_typer_idbits, gits_typer_bits);

  g_gic_its_info->GicIts[ItsIndex].IDBits = GET_MIN(gicd_typer_idbits, gits_typer_bits);

  MmioWrite64((g_gic_its_info->GicRdBase + ARM_GICR_PROPBASER), write_value);

  return EFI_SUCCESS;
}

EFIAPI
EFI_STATUS
ArmGicItsConfiguration (
  )
{
  EFI_STATUS    Status;
  UINT32        index;

  for (index=0; index<g_gic_its_info->GicNumIts; index++)
  {
    /* Set Initial configuration */
    Status = SetInitialConfiguration(index);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  /* Configure Redistributor For LPIs */
  Status = ArmGicRedistributorConfigurationForLPI(g_gic_its_info->GicDBase, g_gic_its_info->GicRdBase);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  for (index=0; index<g_gic_its_info->GicNumIts; index++)
  {
    /* Set Command Queue Base */
    Status = ArmGicSetItsCommandQueueBase(index);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    /* Set Up the ITS tables */
    Status = ArmGicSetItsTables(index);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  DEBUG ((DEBUG_INFO, "\n       ITS : Info Block "));
  for (index=0; index<g_gic_its_info->GicNumIts; index++)
  {
      DEBUG ((DEBUG_INFO, "\nGIC ITS Index : %x", index));
      DEBUG ((DEBUG_INFO, "\nGIC ITS ID : %x", g_gic_its_info->GicIts[index].ID));
      DEBUG ((DEBUG_INFO, "\nGIC ITS Base : %x\n\n", g_gic_its_info->GicIts[index].Base));
  }

  return EFI_SUCCESS;
}
