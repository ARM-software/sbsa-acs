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

#include "FVP/RDN2/include/platform_override_struct.h"
#include "include/pal_common_support.h"
#include "include/pal_pcie_enum.h"

extern SRAT_INFO_TABLE platform_srat_cfg;
extern PLATFORM_OVERRIDE_SRAT_NODE_INFO_TABLE platform_srat_node_type;
extern PLATFORM_OVERRIDE_MPAM_INFO_TABLE platform_mpam_cfg;

/**
  @brief  Display MPAM info table details

  @param  MpamTable  - Address to the MPAM information table.

  @return  None
**/
void
pal_mpam_dump_table(MPAM_INFO_TABLE *MpamTable)
{
  uint32_t i, j;
  MPAM_MSC_NODE *curr_entry;

  if (MpamTable == NULL) {
      return;
  }

  /* point to first entry in MPAM info table*/
  curr_entry = &(MpamTable->msc_node[0]);

  for (i = 0; i < MpamTable->msc_count; i++) {
      print(AVS_PRINT_INFO, "\nMSC node Index      :%d ", i);
      print(AVS_PRINT_INFO, "\nMSC base addr       :%llx ",
                                        curr_entry->msc_base_addr);
      print(AVS_PRINT_INFO, "\nMSC resource count  :%lx ",
                                        curr_entry->rsrc_count);

      for (j = 0; j < curr_entry->rsrc_count; j++) {
          print(AVS_PRINT_INFO, "\nRESOURCE index  :%d ", j);
          print(AVS_PRINT_INFO, "\nRIS index       :%d ",
                               curr_entry->rsrc_node[j].ris_index);
          print(AVS_PRINT_INFO, "\nlocator type    :%08X ",
                            curr_entry->rsrc_node[j].locator_type);
          print(AVS_PRINT_INFO, "\ndescriptor1     :%llx ",
                             curr_entry->rsrc_node[j].descriptor1);
          print(AVS_PRINT_INFO, "\ndescriptor2     :%llx ",
                             curr_entry->rsrc_node[j].descriptor2);
      }
      curr_entry = MPAM_NEXT_MSC(curr_entry);
  }
}

/**
  @brief  Display SRAT info table details

  @param  SratTable  - Address to the SRAT information table.

  @return  None
**/
void
pal_srat_dump_table(SRAT_INFO_TABLE *SratTable)
{
  uint32_t i;
  SRAT_INFO_ENTRY *curr_entry;

  if (SratTable == NULL) {
      return;
  }

  for (i = 0; i < SratTable->num_of_srat_entries; i++) {
      curr_entry = &(SratTable->srat_info[i]);
      if (curr_entry->node_type == SRAT_NODE_MEM_AFF) {
          print(AVS_PRINT_INFO, "\n       SRAT mem prox domain :%x ",
                                              curr_entry->node_data.mem_aff.prox_domain);
          print(AVS_PRINT_INFO, "\n       SRAT mem addr_base :%llx ",
                                                curr_entry->node_data.mem_aff.addr_base);
          print(AVS_PRINT_INFO, "\n       SRAT mem addr_len :%llx ",
                                                      curr_entry->node_data.mem_aff.addr_len);
      }
      else if (curr_entry->node_type == SRAT_NODE_GICC_AFF) {
          print(AVS_PRINT_INFO, "\n       SRAT Gicc prox domain :%x ",
                                              curr_entry->node_data.gicc_aff.prox_domain);
          print(AVS_PRINT_INFO, "\n       SRAT Gicc processor uid :%x ",
                                                   curr_entry->node_data.gicc_aff.proc_uid);
      }
  }
}


/**
  @brief  This API fills in the MPAM_INFO_TABLE with the platform information
          This employs baremetal platform specific data. 

  @param  MpamTable  - Address where the MPAM information needs to be filled.

  @return  None
**/
void
pal_mpam_create_info_table(MPAM_INFO_TABLE *MpamTable)
{
  uint32_t i;
  uint32_t Index;
  MPAM_MSC_NODE *curr_entry;

  if (MpamTable == NULL) {
      print(AVS_PRINT_ERR, " Input MPAM Table Pointer is NULL \n");
      return;
  }

  /* Initialize MpamTable */
  MpamTable->msc_count = 0;

  /* point to first entry in MPAM info table*/
  curr_entry = &(MpamTable->msc_node[0]);

  for (Index = 0; Index < platform_mpam_cfg.msc_count; Index++)
  {
      curr_entry->msc_base_addr = platform_mpam_cfg.msc_node[Index].msc_base_addr;
      curr_entry->msc_addr_len  = platform_mpam_cfg.msc_node[Index].msc_addr_len;
      curr_entry->max_nrdy      = platform_mpam_cfg.msc_node[Index].max_nrdy;
      curr_entry->rsrc_count    = platform_mpam_cfg.msc_node[Index].rsrc_count;

      i = 0;

      while (i < curr_entry->rsrc_count) {
         curr_entry->rsrc_node[i].ris_index    =
                                  platform_mpam_cfg.msc_node[Index].rsrc_node[i].ris_index;
         curr_entry->rsrc_node[i].locator_type =
                                  platform_mpam_cfg.msc_node[Index].rsrc_node[i].locator_type;
         curr_entry->rsrc_node[i].descriptor1  =
                                  platform_mpam_cfg.msc_node[Index].rsrc_node[i].descriptor1;
         curr_entry->rsrc_node[i].descriptor2  =
                                  platform_mpam_cfg.msc_node[Index].rsrc_node[i].descriptor2;
         i++;
      }
      MpamTable->msc_count++;
      curr_entry = MPAM_NEXT_MSC(curr_entry);
  }
  pal_mpam_dump_table(MpamTable);
}

/**
  @brief  This API fills in the SRAT_INFO_TABLE with the platform information

  @param  SratTable  - Address where the SRAT information needs to be filled.

  @return  None
**/
void
pal_srat_create_info_table(SRAT_INFO_TABLE *SratTable)
{
  SRAT_INFO_ENTRY   *Ptr = NULL;
  uint32_t Index, mem_index = 0, gicc_index = 0;

  if (SratTable == NULL) {
      print(AVS_PRINT_ERR, " Input SRAT Table Pointer is NULL \n");
      return;
  }

  /* Initialize SratTable */
  SratTable->num_of_mem_ranges = 0;
  SratTable->num_of_srat_entries = platform_srat_cfg.num_of_srat_entries;

  Ptr = SratTable->srat_info;

  for (Index = 0; Index < platform_srat_cfg.num_of_srat_entries; Index++)
  {
      Ptr->node_type = platform_srat_cfg.srat_info[Index].node_type;
      if (Ptr->node_type == SRAT_NODE_MEM_AFF)
      {
         Ptr->node_data.mem_aff.prox_domain = platform_srat_node_type.mem_aff[mem_index].prox_domain;
         Ptr->node_data.mem_aff.flags       = platform_srat_node_type.mem_aff[mem_index].flags;
         Ptr->node_data.mem_aff.addr_base   = platform_srat_node_type.mem_aff[mem_index].addr_base;
         Ptr->node_data.mem_aff.addr_len    = platform_srat_node_type.mem_aff[mem_index].addr_len;
         mem_index++;
         pal_pe_data_cache_ops_by_va((uint64_t)Ptr, CLEAN_AND_INVALIDATE);
         SratTable->num_of_mem_ranges++;
      }

      else if (Ptr->node_type == SRAT_NODE_GICC_AFF)
      {
         Ptr->node_data.gicc_aff.prox_domain = platform_srat_node_type.gicc_aff[gicc_index].prox_domain;
         Ptr->node_data.gicc_aff.proc_uid    = platform_srat_node_type.gicc_aff[gicc_index].proc_uid;
         Ptr->node_data.gicc_aff.flags       = platform_srat_node_type.gicc_aff[gicc_index].flags;
         Ptr->node_data.gicc_aff.clk_domain  = platform_srat_node_type.gicc_aff[gicc_index].clk_domain;
         gicc_index++;
         pal_pe_data_cache_ops_by_va((uint64_t)Ptr, CLEAN_AND_INVALIDATE);
      }

      Ptr++;
  }

  pal_srat_dump_table(SratTable);
}
