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

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/AcpiTable.h>
#include "Include/IndustryStandard/Acpi61.h"

#include "include/pal_uefi.h"
#include "include/platform_override.h"

static EFI_ACPI_6_1_GENERIC_TIMER_DESCRIPTION_TABLE *gGtdtHdr;

UINT64
pal_get_gtdt_ptr();

/* Information about only one timer can be mentioned as an Override */
static
VOID
pal_timer_platform_override(TIMER_INFO_TABLE *TimerTable)
{
  if (PLATFORM_OVERRIDE_PLATFORM_TIMER) {
      TimerTable->header.num_platform_timer = 1;
      TimerTable->gt_info[0].block_cntl_base = PLATFORM_OVERRIDE_CNTCTL_BASE;
      TimerTable->gt_info[0].timer_count = 1;
      TimerTable->gt_info[0].GtCntBase[0]  = PLATFORM_OVERRIDE_CNTBASE_N;
      TimerTable->gt_info[0].gsiv[0] = PLATFORM_OVERRIDE_PLATFORM_TIMER_GSIV;
  }

  //GTDT does not have this information yet.
  if (PLATFORM_OVERRIDE_EL2_VIR_TIMER_GSIV) {
      TimerTable->header.el2_virt_timer_gsiv = PLATFORM_OVERRIDE_EL2_VIR_TIMER_GSIV;
  }

}

/**
  @brief  This API fills in the TIMER_INFO_TABLE with information about local and system
          timers in the system. This is achieved by parsing the ACPI - GTDT table.

  @param  TimerTable  - Address where the Timer information needs to be filled.

  @return  None
**/
VOID
pal_timer_create_info_table(TIMER_INFO_TABLE *TimerTable)
{
  EFI_ACPI_6_1_GTDT_GT_BLOCK_STRUCTURE       *Entry = NULL;
  EFI_ACPI_6_1_GTDT_GT_BLOCK_TIMER_STRUCTURE *GtBlockTimer = NULL;
  TIMER_INFO_GTBLOCK         *GtEntry = NULL;
  UINT32                      Length= 0;
  UINT32                      i;
  UINT32                      num_of_entries;

  if (TimerTable == NULL) {
    sbsa_print(AVS_PRINT_ERR, L"Input Timer Table Pointer is NULL. Cannot create Timer INFO \n");
    return;
  }

  GtEntry = TimerTable->gt_info;
  TimerTable->header.num_platform_timer = 0;

  gGtdtHdr = (EFI_ACPI_6_1_GENERIC_TIMER_DESCRIPTION_TABLE *) pal_get_gtdt_ptr();

  if (gGtdtHdr == NULL) {
    sbsa_print(AVS_PRINT_ERR, L"GTDT not found \n");
    return;
  }
  sbsa_print(AVS_PRINT_INFO, L" GTDT is at %x and length is %x \n", gGtdtHdr, gGtdtHdr->Header.Length);

  //Fill in our internal table
  TimerTable->header.s_el1_timer_flag  = gGtdtHdr->SecurePL1TimerFlags;
  TimerTable->header.ns_el1_timer_flag = gGtdtHdr->NonSecurePL1TimerFlags;
  TimerTable->header.el2_timer_flag    = gGtdtHdr->NonSecurePL2TimerFlags;
  TimerTable->header.s_el1_timer_gsiv  = gGtdtHdr->SecurePL1TimerGSIV;
  TimerTable->header.ns_el1_timer_gsiv = gGtdtHdr->NonSecurePL1TimerGSIV;
  TimerTable->header.el2_timer_gsiv    = gGtdtHdr->NonSecurePL2TimerGSIV;
  TimerTable->header.virtual_timer_flag = gGtdtHdr->VirtualTimerFlags;
  TimerTable->header.virtual_timer_gsiv = gGtdtHdr->VirtualTimerGSIV;

  Length         = gGtdtHdr->PlatformTimerOffset;
  Entry          = (EFI_ACPI_6_1_GTDT_GT_BLOCK_STRUCTURE *) ((UINT8 *)gGtdtHdr + Length);
  Length         = sizeof (EFI_ACPI_6_1_GENERIC_TIMER_DESCRIPTION_TABLE);
  num_of_entries = gGtdtHdr->PlatformTimerCount;

  while(num_of_entries) {

    if (Entry->Type == EFI_ACPI_6_1_GTDT_GT_BLOCK) {
      sbsa_print(AVS_PRINT_INFO, L"Found block entry \n");
      GtEntry->type = TIMER_TYPE_SYS_TIMER;
      GtEntry->block_cntl_base = Entry->CntCtlBase;
      GtEntry->timer_count     = Entry->GTBlockTimerCount;
      sbsa_print(AVS_PRINT_DEBUG, L"CNTCTLBase = %x \n", GtEntry->block_cntl_base);
      GtBlockTimer = (EFI_ACPI_6_1_GTDT_GT_BLOCK_TIMER_STRUCTURE *)(((UINT8 *)Entry) + Entry->GTBlockTimerOffset);
      for (i = 0; i < GtEntry->timer_count; i++) {
      sbsa_print(AVS_PRINT_INFO, L"Found timer entry \n");
        GtEntry->GtCntBase[i]    = GtBlockTimer->CntBaseX;
        GtEntry->GtCntEl0Base[i] = GtBlockTimer->CntEL0BaseX;
        GtEntry->gsiv[i]         = GtBlockTimer->GTxPhysicalTimerGSIV;
        GtEntry->virt_gsiv[i]    = GtBlockTimer->GTxVirtualTimerGSIV;
        GtEntry->flags[i]        = GtBlockTimer->GTxPhysicalTimerFlags | (GtBlockTimer->GTxVirtualTimerFlags << 8) | (GtBlockTimer->GTxCommonFlags << 16);
        sbsa_print(AVS_PRINT_DEBUG, L"CNTBaseN = %x for sys counter = %d\n", GtEntry->GtCntBase[i], i);
        GtBlockTimer++;
        TimerTable->header.num_platform_timer++;
      }
      GtEntry++;
    }

    if (Entry->Type == EFI_ACPI_6_1_GTDT_SBSA_GENERIC_WATCHDOG) {
      //This is a Watchdog entry. Skip.. added in a separate function.
    }

    Entry = (EFI_ACPI_6_1_GTDT_GT_BLOCK_STRUCTURE *) ((UINT8 *)Entry + (Entry->Length));
    num_of_entries--;

  };

  pal_timer_platform_override(TimerTable);

}

/* Only one watchdog information can be assigned as an override */
VOID
pal_wd_platform_override(WD_INFO_TABLE *WdTable)
{

  if (PLATFORM_OVERRIDE_WD == 1) {
      WdTable->header.num_wd              = 1;
      WdTable->wd_info[0].wd_refresh_base = PLATFORM_OVERRIDE_WD_REFRESH_BASE;
      WdTable->wd_info[0].wd_ctrl_base    = PLATFORM_OVERRIDE_WD_CTRL_BASE;
      WdTable->wd_info[0].wd_gsiv         = PLATFORM_OVERRIDE_WD_GSIV;
      WdTable->wd_info[0].wd_flags        = 0;
  }

  return;
}

/**
  @brief  This API fills in the WD_INFO_TABLE with information about Watchdogs
          in the system. This is achieved by parsing the ACPI - GTDT table.

  @param  WdTable  - Address where the Timer information needs to be filled.

  @return  None
**/

VOID
pal_wd_create_info_table(WD_INFO_TABLE *WdTable)
{

  EFI_ACPI_6_1_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE    *Entry = NULL;
  WD_INFO_BLOCK               *WdEntry = NULL;
  UINT32                      Length= 0;
  UINT32                      num_of_entries;

  if (WdTable == NULL) {
    sbsa_print(AVS_PRINT_ERR, L"Input Watchdog Table Pointer is NULL. Cannot create Watchdog INFO \n");
    return;
  }

  WdEntry = WdTable->wd_info;
  WdTable->header.num_wd = 0;
  gGtdtHdr = (EFI_ACPI_6_1_GENERIC_TIMER_DESCRIPTION_TABLE *) pal_get_gtdt_ptr();

  if (gGtdtHdr == NULL) {
    sbsa_print(AVS_PRINT_ERR, L"GTDT not found \n");
    return;
  }

  Length         = gGtdtHdr->PlatformTimerOffset;
  Entry          = (EFI_ACPI_6_1_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE *) ((UINT8 *)gGtdtHdr + Length);
  Length         = sizeof (EFI_ACPI_6_1_GENERIC_TIMER_DESCRIPTION_TABLE);
  num_of_entries = gGtdtHdr->PlatformTimerCount;

  while(num_of_entries)
  {

    if (Entry->Type == EFI_ACPI_6_1_GTDT_GT_BLOCK) {
      //Skip. this info is added in the timer info function
    }

    if (Entry->Type == EFI_ACPI_6_1_GTDT_SBSA_GENERIC_WATCHDOG) {
      WdEntry->wd_refresh_base = Entry->RefreshFramePhysicalAddress;
      WdEntry->wd_ctrl_base    = Entry->WatchdogControlFramePhysicalAddress;
      WdEntry->wd_gsiv         = Entry->WatchdogTimerGSIV;
      WdEntry->wd_flags        = Entry->WatchdogTimerFlags;
      WdTable->header.num_wd++;
      sbsa_print(AVS_PRINT_DEBUG, L"Watchdog base = 0x%x INTID = 0x%x \n", WdEntry->wd_ctrl_base, WdEntry->wd_gsiv);
      WdEntry++;
    }
    Entry = (EFI_ACPI_6_1_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE *) ((UINT8 *)Entry + (Entry->Length));
    num_of_entries--;

  }

  pal_wd_platform_override(WdTable);

}
