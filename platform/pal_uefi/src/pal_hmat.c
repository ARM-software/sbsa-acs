/** @file
 * Copyright (c) 2023 Arm Limited or its affiliates. All rights reserved.
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

#define ADD_PTR(t, p, l) ((t*)((UINT8*)p + l))

UINT64 pal_get_hmat_ptr();

/**
  @brief  This function checks whether an entry already present for
          a input proximity domain else creates new entry and returns the
          index.

  @param  HmatTable        pointer to pre-allocated memory for hmat info table.
  @param  mem_prox_domain  input memory proximity domain.

  @return Returns index to hmat bw_info_entry for input memory proximity domain.
**/
UINT32 pal_hmat_get_entry_index(HMAT_INFO_TABLE *HmatTable, UINT32 mem_prox_domain)
{
  HMAT_BW_ENTRY *curr_entry;
  UINT32 i;

  curr_entry = HmatTable->bw_info;
  for (i = 0 ; i < HmatTable->num_of_mem_prox_domain ; i++) {
    /* match mem_prox_domain of the entry with input prox_domain */
    if (curr_entry->mem_prox_domain == mem_prox_domain) {
      return i;
    }
    curr_entry++;
  }

  /* if no matching entry found, add the proximity domain to info table
     and return the index*/
  curr_entry =  &HmatTable->bw_info[HmatTable->num_of_mem_prox_domain];
  curr_entry->mem_prox_domain = mem_prox_domain;
  curr_entry->write_bw = 0;  /* initialize write bandwidth */
  curr_entry->read_bw  = 0;  /* initialize read bandwidth */
  HmatTable->num_of_mem_prox_domain++;

  return HmatTable->num_of_mem_prox_domain - 1;
}

/**
  @brief  This function returns maximum bandwidth matrix entry for input
          target proximity domain among all available initiator proximity domains.

  @param  target_prox_index    index of target proximity domain in SLLBI target domain list.
  @param  curr_bw_struct       pointer to SLLBI HMAT structure.

  @return Returns maximum bandwidth matrix entry for input target_prox_index
          and SLLBI HMAT structure.
**/
UINT16 pal_hmat_get_max_bw_entry(UINT32 target_prox_index,
EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO *curr_bw_struct)
{
  UINT32 i, offset;
  UINT16 max_bw_entry = 0;
  UINT16 *bw_entry;

  /* calculate offset to first bandwidth matrix entry for passed target
     proximity domain index in current HMAT structure and get  */
  offset   = sizeof(EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO)
             + sizeof(UINT32) * curr_bw_struct->NumberOfInitiatorProximityDomains
             + sizeof(UINT32) * curr_bw_struct->NumberOfTargetProximityDomains
             + sizeof(UINT16) * target_prox_index;
  bw_entry = ADD_PTR(UINT16, curr_bw_struct, offset);

  /* check all available Initiator Proximity Domains */
  for (i = 0 ; i < curr_bw_struct->NumberOfInitiatorProximityDomains ; i++) {
      /* if entry value is 0xFFFF then the initiator and target domains are
         unreachable from each other, hence ignoring the entry */
      if (*bw_entry == HMAT_BW_ENTRY_UNREACHABLE) {
          bw_entry = bw_entry + curr_bw_struct->NumberOfTargetProximityDomains;
          continue;
      }

      if (max_bw_entry < *bw_entry)
          max_bw_entry = *bw_entry;

      /* increment pointer to next intitiator target pair */
      bw_entry = bw_entry + curr_bw_struct->NumberOfTargetProximityDomains;
  }

  return max_bw_entry;
}

/**
  @brief  This function updates hmat info table after parsing input SLLBI HMAT structure

  @param  HmatTable            pointer to pre-allocated memory for hmat info table.
  @param  curr_bw_struct       pointer to SLLBI HMAT structure.

  @return None
**/
VOID pal_hmat_update_info(HMAT_INFO_TABLE *HmatTable,
EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO *curr_bw_struct)
{
  HMAT_BW_ENTRY *curr_info_entry;
  UINT64 entry_base_unit, curr_max_bw;
  UINT32 i, bw_info_index;
  UINT32 *curr_tgt_prox_domain;
  UINT16 curr_max_bw_entry;

  entry_base_unit = curr_bw_struct->EntryBaseUnit;

  if (entry_base_unit > HMAT_BASE_UNIT_48BIT) {
      sbsa_print(AVS_PRINT_ERR, L"\nEntry Base unit exceeds 0x%llx Mbytes/s ",
                                HMAT_BASE_UNIT_48BIT);
      sbsa_print(AVS_PRINT_ERR, L"\n  BW info entries might overflow 64 bit boundary");
  }

  /* pointer to list of target proximity domains */
  curr_tgt_prox_domain = ADD_PTR(UINT32, curr_bw_struct,
    (sizeof(EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO)
    + 4 * curr_bw_struct->NumberOfInitiatorProximityDomains));

  for (i = 0 ; i < curr_bw_struct->NumberOfTargetProximityDomains ; i++) {
      /* get maximum bandwidth for curr_tgt_prox_domain among available initiator
         proxmity domains */
      curr_max_bw_entry = pal_hmat_get_max_bw_entry(i, curr_bw_struct);
      curr_max_bw = entry_base_unit * curr_max_bw_entry;

      sbsa_print(AVS_PRINT_INFO, L"\nMemory Proximity Domain  : 0x%llx", *curr_tgt_prox_domain);
      sbsa_print(AVS_PRINT_INFO, L"\nEntry Base Unit          : 0x%llx", entry_base_unit);
      sbsa_print(AVS_PRINT_INFO, L"\nMax Bandwidth entry      : 0x%llx", curr_max_bw_entry);
      sbsa_print(AVS_PRINT_INFO, L"\nMax Bandwidth            : 0x%llx", curr_max_bw);
      /* get index to HMAT info table entry for curr_tgt_prox_domain */
      bw_info_index = pal_hmat_get_entry_index(HmatTable, *curr_tgt_prox_domain);
      /* pointer to HMAT info table entry curr_tgt_prox_domain */
      curr_info_entry = &HmatTable->bw_info[bw_info_index];

      /* update the HMAT info table only if current seen bandwidth is
         greater than previously seen bandwidth. HMAT allows table developer
         to capture bandwidth for different scenarios */
      if (curr_bw_struct->DataType == HMAT_DATA_TYPE_ACCESS_BW ||
      curr_bw_struct->DataType == HMAT_DATA_TYPE_WRITE_BW ) {
          if (curr_max_bw > curr_info_entry->write_bw)
              curr_info_entry->write_bw = curr_max_bw;
      }

      if (curr_bw_struct->DataType == HMAT_DATA_TYPE_ACCESS_BW ||
      curr_bw_struct->DataType == HMAT_DATA_TYPE_READ_BW ) {
          if (curr_max_bw > curr_info_entry->read_bw)
              curr_info_entry->read_bw = curr_max_bw;
      }
  /* point to next target proximity domain */
  curr_tgt_prox_domain++;
  }
}

/**
  @brief  This API prints hmat info table entries.

  @param  HmatTable pointer to pre-allocated memory for hmat info table.

  @return None
**/
VOID pal_hmat_dump_info_table(HMAT_INFO_TABLE *HmatTable)
{
  HMAT_BW_ENTRY *curr_entry;
  UINT32 i;

  if (HmatTable == NULL)
      return;

  curr_entry = HmatTable->bw_info;
  sbsa_print(AVS_PRINT_INFO, L"\n*** HMAT info table entries ***\n");
  for (i = 0 ; i < HmatTable->num_of_mem_prox_domain ; i++) {
      sbsa_print(AVS_PRINT_INFO, L"\nMemory Proximity domain  :   0x%llx", curr_entry->mem_prox_domain);
      sbsa_print(AVS_PRINT_INFO, L"\n  Write bandwidth        :   0x%llx", curr_entry->write_bw);
      sbsa_print(AVS_PRINT_INFO, L"\n  Read  bandwidth        :   0x%llx\n", curr_entry->read_bw);
      curr_entry++;
  }
}

/**
  @brief  Parses ACPI HMAT table and populates the local hmat info table with
          maximum read/write bandwidth for memory proximity domains.

  @param  HmatTable pointer to pre-allocated memory for hmat info table.

  @return None
**/
VOID pal_hmat_create_info_table(HMAT_INFO_TABLE *HmatTable)
{
  EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER *HmatHdr;
  EFI_ACPI_6_4_HMAT_STRUCTURE_HEADER *hmat_struct, *hmat_end;
  EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO *curr_bw_struct;
  UINT32 TableLength = 0;

  if (HmatTable == NULL) {
      sbsa_print(AVS_PRINT_ERR, L" Unable to create HMAT info table, input pointer is NULL\n");
      return;
  }

    /* initialize hmat info table number of entries */
  HmatTable->num_of_mem_prox_domain = 0;

  HmatHdr = (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER *) pal_get_hmat_ptr();
  if (HmatHdr == NULL) {
      sbsa_print(AVS_PRINT_DEBUG, L" HMAT ACPI table not found\n");
      return;
  }
  else {
      TableLength = HmatHdr->Header.Length;
      sbsa_print(AVS_PRINT_INFO, L"HMAT ACPI table found at 0x%llx with length 0x%x\n",
                 HmatHdr, TableLength);
  }

  /* pointer to first HMAT structure in ACPI table */
  /* HMAT table has 4 bytes addition to ACPI header to
     make the HMAT structures 8 byte aligned */
  hmat_struct = ADD_PTR(EFI_ACPI_6_4_HMAT_STRUCTURE_HEADER, HmatHdr,
                         sizeof(EFI_ACPI_DESCRIPTION_HEADER) + 4);

  /* HMAT end boundary */
  hmat_end =  ADD_PTR(EFI_ACPI_6_4_HMAT_STRUCTURE_HEADER, HmatHdr, TableLength);
  /* iterate HMAT structs in HMAT ACPI table */
  while (hmat_struct < hmat_end) {
      /* look for system locality latency and bandwidth info HMAT structure */
      if (hmat_struct->Type == EFI_ACPI_6_4_HMAT_TYPE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO) {
          curr_bw_struct = (
          EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO *) hmat_struct;
          /* update info table, if the data captured by structure is of type bandwidth */
          if (curr_bw_struct->Flags.MemoryHierarchy == HMAT_MEM_HIERARCHY_MEMORY)
              if (curr_bw_struct->DataType == HMAT_DATA_TYPE_ACCESS_BW ||
                  curr_bw_struct->DataType == HMAT_DATA_TYPE_WRITE_BW  ||
                  curr_bw_struct->DataType == HMAT_DATA_TYPE_READ_BW)
                  pal_hmat_update_info(HmatTable, curr_bw_struct);
      }
      /* point to next hmat structure*/
      hmat_struct = ADD_PTR(EFI_ACPI_6_4_HMAT_STRUCTURE_HEADER, hmat_struct, hmat_struct->Length);
  }
  pal_hmat_dump_info_table(HmatTable);
}
