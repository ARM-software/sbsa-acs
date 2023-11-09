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

#include "pal_common_support.h"
#include "platform_override_struct.h"

extern RAS_INFO_TABLE platform_ras_cfg;
extern PLATFORM_OVERRIDE_RAS_NODE_DATA_INFO platform_ras_node_data;
extern PLATFORM_OVERRIDE_RAS_NODE_INTERFACE_INFO platform_ras_node_interface;
extern PLATFORM_OVERRIDE_RAS_NODE_INTERRUPT_INFO platform_ras_node_interrupt;

extern PLATFORM_OVERRIDE_RAS2_INFO_TABLE platform_ras2_cfg;

/**
  @brief  Platform Defined way of Timeout/Wait loop

  @param  count  - Timeout/Wait Multiplier.

  @return None
**/
void
pal_ras_wait_timeout(uint32_t count)
{
  uint32_t timeout = count * PLATFORM_OVERRIDE_TIMEOUT_MEDIUM;

  /* Wait Loop */
  while (timeout > 0)
    timeout--;
}

/**
  @brief  Display RAS info table details

  @param  RasInfoTable  - Address to the RAS information table.

  @return  None
**/
void
pal_ras_dump_info_table(RAS_INFO_TABLE *RasInfoTable)
{
  uint32_t i, j;
  RAS_NODE_INFO *curr;

  if (RasInfoTable == NULL) {
      return;
  }

  print(AVS_PRINT_INFO, "\nRAS Info :");
  print(AVS_PRINT_INFO, "\nRAS Num Nodes : %d ", RasInfoTable->num_nodes);

  curr = &(RasInfoTable->node[0]);

  for (i = 0; i < RasInfoTable->num_nodes; i++) {
      print(AVS_PRINT_INFO, "\n Index    : %d ", i);
      print(AVS_PRINT_INFO, "\n Type     : 0x%x ", curr->type);
      print(AVS_PRINT_INFO, "\n Num Intr : 0x%x ", curr->num_intr_entries);

      switch (curr->type) {
       case NODE_TYPE_PE:
           /* Print Processor Node Details */
           print(AVS_PRINT_INFO, "\n ProcessorID : 0x%x ", curr->node_data.pe.processor_id);
           print(AVS_PRINT_INFO, "\n resource_type : 0x%x ",
                                      curr->node_data.pe.resource_type);
           print(AVS_PRINT_INFO, "\n flags : 0x%x ", curr->node_data.pe.flags);
           print(AVS_PRINT_INFO, "\n affinity : 0x%x ", curr->node_data.pe.affinity);
           break;
       case NODE_TYPE_MC:
           /* Print Memory Controller Node Details */
           print(AVS_PRINT_INFO, "\n proximity_domain : 0x%x ",
                                      curr->node_data.mc.proximity_domain);
           break;
       default:
           break;
      }

      print(AVS_PRINT_INFO, "\n Interface Info :");
      print(AVS_PRINT_INFO, "\n  type    : 0x%x ", curr->intf_info.intf_type);
      print(AVS_PRINT_INFO, "\n  base    : 0x%x ", curr->intf_info.base_addr);
      print(AVS_PRINT_INFO, "\n  num_err : 0x%x ", curr->intf_info.num_err_rec);

      print(AVS_PRINT_INFO, "\n Interrupt Info :");
      for (j = 0; j < curr->num_intr_entries; j++) {
        print(AVS_PRINT_INFO, "\n  type    : 0x%x ", curr->intr_info[j].type);
        print(AVS_PRINT_INFO, "\n  gsiv    : 0x%x ", curr->intr_info[j].gsiv);
      }

      curr++;
  }
  print(AVS_PRINT_INFO, "\n");
}

void fill_node_specific_data (
     RAS_INFO_TABLE *RasInfoTable,
     RAS_NODE_INFO *curr_node,
     uint32_t node_index
     )
{
   switch (curr_node->type) {
     case NODE_TYPE_PE:
       /* Fill Processor Node Details */
       curr_node->node_data.pe.processor_id =
                platform_ras_node_data.node_data[node_index].pe.processor_id;
       curr_node->node_data.pe.resource_type =
                platform_ras_node_data.node_data[node_index].pe.resource_type;
       curr_node->node_data.pe.flags =
                platform_ras_node_data.node_data[node_index].pe.flags;
       curr_node->node_data.pe.affinity =
                platform_ras_node_data.node_data[node_index].pe.affinity;
       RasInfoTable->num_pe_node++;
       break;
     case NODE_TYPE_MC:
       /* Fill Memory Controller Node Details */
       curr_node->node_data.mc.proximity_domain =
                platform_ras_node_data.node_data[node_index].mc.proximity_domain;
       RasInfoTable->num_mc_node++;
       break;
     default:
       break;
   }
}

void fill_node_interface_data (
     RAS_INFO_TABLE *RasInfoTable,
     RAS_NODE_INFO *curr_node,
     uint32_t node_index
     )
{
    (void) RasInfoTable;

    curr_node->intf_info.intf_type =
                platform_ras_node_interface.intf_info[node_index].intf_type;
    curr_node->intf_info.flags = platform_ras_node_interface.intf_info[node_index].flags;
    curr_node->intf_info.base_addr =
                platform_ras_node_interface.intf_info[node_index].base_addr;
    curr_node->intf_info.start_rec_index =
                platform_ras_node_interface.intf_info[node_index].start_rec_index;
    curr_node->intf_info.num_err_rec =
                platform_ras_node_interface.intf_info[node_index].num_err_rec;
    curr_node->intf_info.err_rec_implement =
                platform_ras_node_interface.intf_info[node_index].err_rec_implement;
    curr_node->intf_info.err_status_reporting =
                platform_ras_node_interface.intf_info[node_index].err_status_reporting;
    curr_node->intf_info.addressing_mode =
                platform_ras_node_interface.intf_info[node_index].addressing_mode;
}

void fill_node_interrupt_data (
     RAS_INFO_TABLE *RasInfoTable,
     RAS_NODE_INFO *curr_node,
     uint32_t node_index
     )
{
  (void) RasInfoTable;
  uint32_t count = 0;

  for (count = 0; count < curr_node->num_intr_entries; count++)
  {
    curr_node->intr_info[count].type =
            platform_ras_node_interrupt.intr_info[node_index][count].type;
    curr_node->intr_info[count].gsiv =
            platform_ras_node_interrupt.intr_info[node_index][count].gsiv;
    curr_node->intr_info[count].flag =
            platform_ras_node_interrupt.intr_info[node_index][count].flag;
    curr_node->intr_info[count].its_grp_id =
            platform_ras_node_interrupt.intr_info[node_index][count].its_grp_id;
  }

}
/**
  @brief  This API fills in the RAS_INFO_TABLE with information about RAS Nodes.
          This is achieved by platform_ras_cfg details.

  @param  RasInfoTable  - Address where the RAS information needs to be filled.

  @return  None
**/
void
pal_ras_create_info_table(RAS_INFO_TABLE *RasInfoTable)
{
  RAS_NODE_INFO *curr_node;
  uint32_t i;

  if (RasInfoTable == NULL) {
      print(AVS_PRINT_ERR, "\n Input RAS Table Pointer is NULL");
      return;
  }

  /* Initialize RasInfoTable */
  RasInfoTable->num_nodes = 0;
  RasInfoTable->num_pe_node = 0;
  RasInfoTable->num_mc_node = 0;

  /* Pointer to the first RAS node */
  curr_node = &(RasInfoTable->node[0]);

  /* Create RAS info block for each RAS node*/
  for (i = 0; i < platform_ras_cfg.num_nodes; i++)
  {
      /* Fill Node Info */
      curr_node->type = platform_ras_cfg.node[i].type;
      curr_node->length = platform_ras_cfg.node[i].length;
      curr_node->num_intr_entries = platform_ras_cfg.node[i].num_intr_entries;

      /* Fill Node Specific data */
      fill_node_specific_data(RasInfoTable, curr_node, i);

      /* Fill Node Interface data */
      fill_node_interface_data(RasInfoTable, curr_node, i);

      /* Fill Node Interrupt data */
      fill_node_interrupt_data(RasInfoTable, curr_node, i);

      RasInfoTable->num_nodes++;

      if (RasInfoTable->num_nodes >= RAS_MAX_NUM_NODES) {
          print(AVS_PRINT_WARN, "\n Number of RAS nodes greater than %d",
                     RAS_MAX_NUM_NODES);
          break;
      }

      /* Move to next RAS node */
      curr_node++;
  }

  pal_ras_dump_info_table(RasInfoTable);
}

/**
  @brief  Display RAS info table details

  @param  RasInfoTable  - Address to the RAS information table.

  @return  None
**/
void
pal_ras2_dump_info_table(RAS2_INFO_TABLE *RasFeatInfoTable)
{
  RAS2_BLOCK *curr_block;

  if (RasFeatInfoTable == NULL) {
      print(AVS_PRINT_ERR, "\n Input RAS Table Pointer is NULL");
      return;
  }

  curr_block = RasFeatInfoTable->blocks;
  uint32_t i;

  print(AVS_PRINT_INFO, "\n RAS2 Feature Info :");
  print(AVS_PRINT_INFO,
             "\n Total number of RAS2 feature info blocks  : %d",
             RasFeatInfoTable->num_all_block);
  print(AVS_PRINT_INFO,
             "\n Number of RAS2 memory feature info blocks : %d\n",
             RasFeatInfoTable->num_of_mem_block);

  /*Iterate RAS2 feature info table and print info fields */
  for (i = 0 ; i < RasFeatInfoTable->num_all_block ; i++) {
    print(AVS_PRINT_INFO, "\n RAS2 feature info * Index %d *", i);
    switch(curr_block->type) {
    case RAS2_TYPE_MEMORY:
        print(AVS_PRINT_INFO, "\n  Type                            : 0x%x",
                   curr_block->type);
        print(AVS_PRINT_INFO, "\n  Proximity Domain                : 0x%llx",
                   curr_block->block_info.mem_feat_info.proximity_domain);
        print(AVS_PRINT_INFO, "\n  Patrol scrub support            : 0x%lx\n",
                   curr_block->block_info.mem_feat_info.patrol_scrub_support);
        break;
    default:
        print(AVS_PRINT_INFO,
             "\n  Invalid RAS feature type : 0x%x",
                   curr_block->type);
    }
    curr_block++;
  }
}

/**
  @brief  This API fills in the RAS2_INFO_TABLE with memory patrol scrub info
          from parsing RAS2 Platform Cfg.

  @return  None
**/
void
pal_ras2_create_info_table(RAS2_INFO_TABLE *RasFeatInfoTable)
{
  RAS2_BLOCK *curr_block;

  uint32_t i;

  if (RasFeatInfoTable == NULL) {
      print(AVS_PRINT_ERR, "\n Input RAS Table Pointer is NULL");
      return;
  }

  /* initialise RAS2_INFO_TABLE fields */
  RasFeatInfoTable->num_all_block = 0;
  RasFeatInfoTable->num_of_mem_block = 0;

  /* point to first info block */
  curr_block = RasFeatInfoTable->blocks;

  /* Iterate PCC descriptors and parse corresponding PCC info for each proxomity domain */
  for (i = 0; i < platform_ras2_cfg.num_all_block; i++) {
      if (platform_ras2_cfg.blocks[i].type == RAS2_TYPE_MEMORY) {

          /* Fill required Fields */
          curr_block->type = platform_ras2_cfg.blocks[i].type;
          curr_block->block_info.mem_feat_info.proximity_domain =
                                    platform_ras2_cfg.blocks[i].proximity_domain;
          curr_block->block_info.mem_feat_info.patrol_scrub_support =
                                    platform_ras2_cfg.blocks[i].patrol_scrub_support;

          /* Increment block count and move the pointer to next info block*/
          RasFeatInfoTable->num_of_mem_block++;
          curr_block++;
      }

      RasFeatInfoTable->num_all_block++;
  }
  pal_ras2_dump_info_table(RasFeatInfoTable);
}
