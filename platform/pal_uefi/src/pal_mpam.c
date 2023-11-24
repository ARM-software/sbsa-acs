/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include "include/pal_uefi.h"
#include "include/platform_override.h"
#include "include/pal_mpam.h"

#define ADD_PTR(t, p, l) ((t*)((UINT8*)p + l))

UINT64 pal_get_mpam_ptr();

UINT64 pal_get_srat_ptr();

/**
  @brief  Display MPAM info table details

  @param  MpamTable  - Address to the MPAM information table.

  @return  None
**/
VOID
pal_mpam_dump_table(MPAM_INFO_TABLE *MpamTable)
{
  UINT32 i, j;
  MPAM_MSC_NODE *curr_entry;

  if (MpamTable == NULL) {
      return;
  }

  /* point to first entry in MPAM info table*/
  curr_entry = &(MpamTable->msc_node[0]);

  for (i = 0; i < MpamTable->msc_count; i++) {
      sbsa_print(AVS_PRINT_INFO, L"\nMSC node Index      :%d ", i);
      sbsa_print(AVS_PRINT_INFO, L"\nMSC base addr       :%llx ",
                                        curr_entry->msc_base_addr);
      sbsa_print(AVS_PRINT_INFO, L"\nMSC resource count  :%lx ",
                                        curr_entry->rsrc_count);

      for (j = 0; j < curr_entry->rsrc_count; j++) {
          sbsa_print(AVS_PRINT_INFO, L"\nRESOURCE index  :%d ", j);
          sbsa_print(AVS_PRINT_INFO, L"\nRIS index       :%d ",
                               curr_entry->rsrc_node[j].ris_index);
          sbsa_print(AVS_PRINT_INFO, L"\nlocator type    :%08X ",
                            curr_entry->rsrc_node[j].locator_type);
          sbsa_print(AVS_PRINT_INFO, L"\ndescriptor1     :%llx ",
                             curr_entry->rsrc_node[j].descriptor1);
      }
      curr_entry = MPAM_NEXT_MSC(curr_entry);
  }
}

/**
  @brief  Display SRAT info table details

  @param  SratTable  - Address to the SRAT information table.

  @return  None
**/
VOID
pal_srat_dump_table(SRAT_INFO_TABLE *SratTable)
{
  UINT32 i;
  SRAT_INFO_ENTRY *curr_entry;

  if (SratTable == NULL) {
      return;
  }

  for (i = 0; i < SratTable->num_of_srat_entries; i++) {
      curr_entry = &(SratTable->srat_info[i]);
      if ( curr_entry->node_type == SRAT_NODE_MEM_AFF) {
          sbsa_print(AVS_PRINT_INFO, L"\n       SRAT mem prox domain :%x ",
                                                   curr_entry->node_data.mem_aff.prox_domain);
          sbsa_print(AVS_PRINT_INFO, L"\n       SRAT mem addr_base :%llx ",
                                                     curr_entry->node_data.mem_aff.addr_base);
          sbsa_print(AVS_PRINT_INFO, L"\n       SRAT mem addr_len :%llx ",
                                                      curr_entry->node_data.mem_aff.addr_len);
      }
      else if ( curr_entry->node_type == SRAT_NODE_GICC_AFF) {
          sbsa_print(AVS_PRINT_INFO, L"\n       SRAT Gicc prox domain :%x ",
                                                   curr_entry->node_data.gicc_aff.prox_domain);
          sbsa_print(AVS_PRINT_INFO, L"\n       SRAT Gicc processor uid :%x ",
                                                   curr_entry->node_data.gicc_aff.proc_uid);
      }
  }
}

/**
  @brief  This API fills in the MPAM_INFO_TABLE with the platform information
          This is achieved by parsing the ACPI - MPAM table.

  @param  MpamTable  - Address where the MPAM information needs to be filled.

  @return  None
**/
VOID
pal_mpam_create_info_table(MPAM_INFO_TABLE *MpamTable)
{
  UINT32 i;
  EFI_ACPI_MPAM_TABLE *mpam;
  EFI_ACPI_MPAM_MSC_NODE_STRUCTURE *msc_node, *msc_end;
  EFI_ACPI_MPAM_RESOURCE_NODE_STRUCTURE *rsrc_node;
  MPAM_MSC_NODE *curr_entry;

  if (MpamTable == NULL) {
      sbsa_print(AVS_PRINT_ERR, L" Input MPAM Table Pointer is NULL\n");
      return;
  }

  /* Initialize MpamTable */
  MpamTable->msc_count = 0;

  /* point to first entry in MPAM info table*/
  curr_entry = &(MpamTable->msc_node[0]);

  mpam = (EFI_ACPI_MPAM_TABLE *)pal_get_mpam_ptr();
  if (mpam == NULL) {
      sbsa_print(AVS_PRINT_DEBUG, L" MPAM table not found\n");
      return;
  }

  /* Pointer to the first MSC node */
  msc_node = ADD_PTR(EFI_ACPI_MPAM_MSC_NODE_STRUCTURE, mpam,
                                        sizeof(EFI_ACPI_DESCRIPTION_HEADER));
  msc_end = ADD_PTR(EFI_ACPI_MPAM_MSC_NODE_STRUCTURE, mpam,
                                                          mpam->header.Length);

  /* Populate MPAM table MSC node from acpi table MSC node*/
  while (msc_node < msc_end) {
      curr_entry->msc_base_addr =  msc_node->base_address;
      curr_entry->msc_addr_len  =  msc_node->mmio_size;
      curr_entry->max_nrdy = msc_node->max_nrdy_usec;
      curr_entry->rsrc_count = msc_node->num_resource_nodes;

      /* Each MSC can have multiple resource node, Populate info table resource
        node from acpi table resource node*/
      rsrc_node = ADD_PTR(EFI_ACPI_MPAM_RESOURCE_NODE_STRUCTURE, msc_node,
                                  sizeof(EFI_ACPI_MPAM_MSC_NODE_STRUCTURE));
      i = 0;

      while (i < curr_entry->rsrc_count) {
         curr_entry->rsrc_node[i].ris_index = rsrc_node->ris_index;
         curr_entry->rsrc_node[i].locator_type = rsrc_node->locator_type;
         curr_entry->rsrc_node[i].descriptor1 =  rsrc_node->descriptor1;
         rsrc_node = ADD_PTR(EFI_ACPI_MPAM_RESOURCE_NODE_STRUCTURE, rsrc_node,
                     sizeof(EFI_ACPI_MPAM_RESOURCE_NODE_STRUCTURE) +
                     (sizeof(EFI_ACPI_MPAM_FUNC_DEPEN_DESC_STRUCTURE) *
                                            rsrc_node->num_dependencies));
         i++;
      }
      MpamTable->msc_count++;
      msc_node = ADD_PTR(EFI_ACPI_MPAM_MSC_NODE_STRUCTURE, msc_node,
                                                            msc_node->length);
      curr_entry = MPAM_NEXT_MSC(curr_entry);
  }
  pal_mpam_dump_table(MpamTable);
}

/**
  @brief  This API fills in the SRAT_INFO_TABLE with the platform information
          This is achieved by parsing the ACPI - SRAT table.

  @param  SratTable  - Address where the SRAT information needs to be filled.

  @return  None
**/
VOID
pal_srat_create_info_table(SRAT_INFO_TABLE *SratTable)
{
  EFI_ACPI_6_4_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER *SratHdr;
  EFI_ACPI_6_4_SRAT_STRUCTURE_HEADER *Entry = NULL;
  EFI_ACPI_6_4_MEMORY_AFFINITY_STRUCTURE *Mem_Aff_Entry = NULL;
  EFI_ACPI_6_4_GICC_AFFINITY_STRUCTURE *Gicc_Aff_Entry = NULL;
  SRAT_INFO_ENTRY   *Ptr = NULL;
  UINT32  TableLength = 0;
  UINT32  Length = 0;

  if (SratTable == NULL) {
      sbsa_print(AVS_PRINT_ERR, L" Input SRAT Table Pointer is NULL\n");
      return;
  }

  /* Initialize SratTable */
  SratTable->num_of_mem_ranges = 0;
  SratTable->num_of_srat_entries = 0;

  SratHdr = (EFI_ACPI_6_4_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER *) pal_get_srat_ptr();

  if (SratHdr != NULL) {
    TableLength =  SratHdr->Header.Length;
    sbsa_print(AVS_PRINT_INFO, L" SRAT is at %x and length is %x\n", SratHdr, TableLength);
  } else {
    sbsa_print(AVS_PRINT_DEBUG, L" SRAT not found\n");
    return;
  }

  Entry = (EFI_ACPI_6_4_SRAT_STRUCTURE_HEADER *) (SratHdr + 1);
  Length = sizeof (EFI_ACPI_6_4_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER);
  Ptr = SratTable->srat_info;

  do {

    if (Entry->Type == EFI_ACPI_6_4_MEMORY_AFFINITY) {
      //Fill in the mem ranges proximity domain, addr, len
      Mem_Aff_Entry = (EFI_ACPI_6_4_MEMORY_AFFINITY_STRUCTURE *) Entry;
      Ptr->node_type = Mem_Aff_Entry->Type;
      Ptr->node_data.mem_aff.prox_domain = Mem_Aff_Entry->ProximityDomain;
      Ptr->node_data.mem_aff.addr_base   = ((UINT64) Mem_Aff_Entry->AddressBaseHigh << 32) |
                                                         Mem_Aff_Entry->AddressBaseLow;
      Ptr->node_data.mem_aff.addr_len    = ((UINT64) Mem_Aff_Entry->LengthHigh << 32) |
                                                              Mem_Aff_Entry->LengthLow;
      Ptr->node_data.mem_aff.flags       = Mem_Aff_Entry->Flags;
      sbsa_print(AVS_PRINT_DEBUG, L" Proximity Domain %x\n", Ptr->node_data.mem_aff.prox_domain);
      sbsa_print(AVS_PRINT_DEBUG, L" Address %x\n", Ptr->node_data.mem_aff.addr_base);
      sbsa_print(AVS_PRINT_DEBUG, L" Length %x\n", Ptr->node_data.mem_aff.addr_len);
      pal_pe_data_cache_ops_by_va((UINT64)Ptr, CLEAN_AND_INVALIDATE);
      Ptr++;
      SratTable->num_of_mem_ranges++;
      SratTable->num_of_srat_entries++;
    }

    else if (Entry->Type == EFI_ACPI_6_4_GICC_AFFINITY) {
      Gicc_Aff_Entry = (EFI_ACPI_6_4_GICC_AFFINITY_STRUCTURE *) Entry;
      Ptr->node_type = Gicc_Aff_Entry->Type;
      Ptr->node_data.gicc_aff.prox_domain = Gicc_Aff_Entry->ProximityDomain;
      Ptr->node_data.gicc_aff.proc_uid = Gicc_Aff_Entry->AcpiProcessorUid;
      Ptr->node_data.gicc_aff.flags = Gicc_Aff_Entry->Flags;
      Ptr->node_data.gicc_aff.clk_domain = Gicc_Aff_Entry->ClockDomain;
      sbsa_print(AVS_PRINT_DEBUG, L" Proximity Domain %x\n", Ptr->node_data.gicc_aff.prox_domain);
      sbsa_print(AVS_PRINT_DEBUG, L" Processor UID %x\n", Ptr->node_data.gicc_aff.proc_uid);
      sbsa_print(AVS_PRINT_DEBUG, L" Clock Domain %x\n", Ptr->node_data.gicc_aff.clk_domain);
      pal_pe_data_cache_ops_by_va((UINT64)Ptr, CLEAN_AND_INVALIDATE);
      Ptr++;
      SratTable->num_of_srat_entries++;
    }
    Length += Entry->Length;
    Entry = (EFI_ACPI_6_4_SRAT_STRUCTURE_HEADER *) ((UINT8 *)Entry +
                                                              (Entry->Length));
  } while(Length < TableLength);

  pal_srat_dump_table(SratTable);
}
