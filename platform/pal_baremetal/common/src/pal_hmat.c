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

#include "pal_common_support.h"
#include "pal_pcie_enum.h"
#include "platform_override_struct.h"

extern PLATFORM_OVERRIDE_HMAT_INFO_TABLE platform_hmat_cfg;
extern PLATFORM_OVERRIDE_HMAT_MEM_TABLE platform_hmat_mem_cfg;

/**
  @brief  This API prints hmat info table entries.

  @param  HmatTable pointer to pre-allocated memory for hmat info table.

  @return None
**/
void pal_hmat_dump_info_table(HMAT_INFO_TABLE *HmatTable)
{
  HMAT_BW_ENTRY *curr_entry;
  uint32_t i;

  if (HmatTable == NULL)
      return;

  curr_entry = HmatTable->bw_info;
  print(AVS_PRINT_INFO, "\n*** HMAT info table entries ***\n");
  for (i = 0 ; i < HmatTable->num_of_mem_prox_domain ; i++) {
      print(AVS_PRINT_INFO, "\nMemory Proximity domain  :   0x%llx", curr_entry->mem_prox_domain);
      print(AVS_PRINT_INFO, "\n  Write bandwidth        :   0x%llx", curr_entry->write_bw);
      print(AVS_PRINT_INFO, "\n  Read  bandwidth        :   0x%llx\n", curr_entry->read_bw);
      curr_entry++;
  }
}

/**
  @brief  Populates the local hmat info table with maximum read/write
          bandwidth for memory proximity domains.

  @param  HmatTable pointer to pre-allocated memory for hmat info table.

  @return None
**/
void pal_hmat_create_info_table(HMAT_INFO_TABLE *HmatTable)
{
  int32_t prox_domain = 0;
  uint32_t Index = 0;
  uint32_t data_type;
  uint64_t entry_base_unit;
  HMAT_BW_ENTRY *curr_info_entry;

  if (HmatTable == NULL) {
      print(AVS_PRINT_ERR, " Unable to create HMAT info table, input pointer is NULL \n");
      return;
  }

    /* initialize hmat info table number of entries */
  HmatTable->num_of_mem_prox_domain = 0;
  curr_info_entry = HmatTable->bw_info;


  for (Index = 0; Index < platform_hmat_cfg.num_of_prox_domain; Index++)
  {
      if (platform_hmat_cfg.bw_info[Index].type == HMAT_NODE_MEM_SLLBIC)
      {
          if (((platform_hmat_cfg.bw_info[Index].flags & 0xF) == HMAT_MEM_HIERARCHY_MEMORY))
          {
              data_type = platform_hmat_cfg.bw_info[Index].data_type;
              entry_base_unit = platform_hmat_cfg.bw_info[Index].entry_base_unit;
              while (prox_domain < PLATFORM_OVERRIDE_HMAT_MEM_ENTRIES)
              {
                  curr_info_entry->mem_prox_domain = platform_hmat_mem_cfg.bw_mem_info[prox_domain].mem_prox_domain;
                  if (data_type == HMAT_DATA_TYPE_ACCESS_BW || data_type == HMAT_DATA_TYPE_WRITE_BW)
                      curr_info_entry->write_bw = entry_base_unit * platform_hmat_mem_cfg.bw_mem_info[prox_domain].max_write_bw;
                  if (data_type == HMAT_DATA_TYPE_ACCESS_BW || data_type == HMAT_DATA_TYPE_READ_BW)
                      curr_info_entry->read_bw = entry_base_unit * platform_hmat_mem_cfg.bw_mem_info[prox_domain].max_read_bw;
                  prox_domain++;
                  curr_info_entry++;
                  HmatTable->num_of_mem_prox_domain++;
              }
          }
      }
  }

  pal_hmat_dump_info_table(HmatTable);
}
