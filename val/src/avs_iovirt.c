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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_common.h"
#include "include/sbsa_avs_iovirt.h"

IOVIRT_INFO_TABLE *g_iovirt_info_table;

/**
  @brief   This API is a single point of entry to retrieve
           SMMU information stored in the IoVirt Info table
           1. Caller       -  val_smmu_get_info
           2. Prerequisite -  val_iovirt_create_info_table
  @param   type   the type of information being requested
  @return  64-bit data
**/
uint64_t
val_iovirt_get_smmu_info(SMMU_INFO_e type, uint32_t index)
{
  uint32_t i,j = 0;
  IOVIRT_BLOCK *block;

  if (g_iovirt_info_table == NULL)
  {
      val_print(AVS_PRINT_ERR, "GET_SMMU_INFO: iovirt info table is not created \n", 0);
      return 0;
  }

  if(type == SMMU_NUM_CTRL)
      return g_iovirt_info_table->num_smmus;

  /* Go through the table return the relevant field value for the SMMU block */
  /* at the index position */
  block = &g_iovirt_info_table->blocks[0];
  for(i = 0; i < g_iovirt_info_table->num_blocks; i++, block=IOVIRT_NEXT_BLOCK(block))
  {
      if(block->type == IOVIRT_NODE_SMMU || block->type == IOVIRT_NODE_SMMU_V3)
      {
          if(j == index)
          {
              switch(type)
              {
                  case SMMU_CTRL_ARCH_MAJOR_REV:
                      return block->data.smmu.arch_major_rev;
                  case SMMU_CTRL_BASE:
                      return block->data.smmu.base;
                  case SMMU_IOVIRT_BLOCK:
                      return (uint64_t)block;
                  default:
                      val_print(AVS_PRINT_ERR, "This SMMU info option not supported %d \n", type);
                      return 0;
              }
          }
          j++;
      }
  }

  if (index > j-1)
  {
      val_print(AVS_PRINT_ERR, "GET_SMMU_INFO: Index (%d) is greater than num of SMMU \n", index);
      return 0;
  }
  return j;
}

/**
  @brief   This API is a single point of entry to retrieve
           PCIe Root Complex Node info stored in the iovirt Info table
           1. Caller       -  Test suite
           2. Prerequisite -  val_iovirt_create_info_table
  @param   type   the type of information being requested
  @return  64-bit data
**/
uint64_t
val_iovirt_get_pcie_rc_info(PCIE_RC_INFO_e type, uint32_t index)
{
  uint32_t i,j = 0;
  IOVIRT_BLOCK *block;

  if (g_iovirt_info_table == NULL)
  {
      val_print(AVS_PRINT_ERR, "GET_PCIe_RC_INFO: iovirt info table is not created \n", 0);
      return 0;
  }

  if(type == NUM_PCIE_RC)
      return g_iovirt_info_table->num_pci_rcs;

  /* Go through the table return the relevant field value for the SMMU block */
  /* at the index position */
  block = &g_iovirt_info_table->blocks[0];
  for(i = 0; i < g_iovirt_info_table->num_blocks; i++, block=IOVIRT_NEXT_BLOCK(block))
  {
      if(block->type == IOVIRT_NODE_PCI_ROOT_COMPLEX)
      {
          if(j == index)
          {
              switch(type)
              {
                  case RC_SEGMENT_NUM:
                      return block->data.rc.segment;
                  case RC_MEM_ATTRIBUTE:
                      return block->data.rc.cca;
                  case RC_ATS_ATTRIBUTE:
                      return block->data.rc.ats_attr;
                  case RC_IOVIRT_BLOCK:
                      return (uint64_t)block;
                  default:
                      val_print(AVS_PRINT_ERR, "This PCIe RC info option not supported %d \n", type);
                      return 0;
              }
          }
          j++;
      }
  }
  if (index > j-1)
  {
      val_print(AVS_PRINT_ERR, "GET_PCIe_RC_INFO: Index (%d) is greater than num of PCIe-RC \n", index);
      return 0;
  }
  return j;

}

uint32_t
val_iovirt_unique_rid_strid_map(uint32_t rc_index)
{
  uint64_t block = val_iovirt_get_pcie_rc_info(RC_IOVIRT_BLOCK, rc_index);
  return pal_iovirt_unique_rid_strid_map(block);
}

/**
  @brief  Calculate the device id and stream id orresponding to the requestor id
  @param  rid          Requestor ID
  @param  segment      pci_segment_number
  @param  *device_id   Pointer to device id
  @param  *stream_id   Pointer to stream id
  @return status
**/

int
val_iovirt_get_device_id(uint32_t rid, uint32_t segment, uint32_t *device_id, uint32_t *stream_id)
{
  uint32_t i, j, id = 0;
  uint32_t str_id, oref;
  IOVIRT_BLOCK *block;
  NODE_DATA_MAP *map;
  if (g_iovirt_info_table == NULL)
  {
      val_print(AVS_PRINT_ERR, "GET_DEVICE_ID: iovirt info table is not created \n", 0);
      return AVS_STATUS_ERR;
  }
  if(!device_id) {
      val_print(AVS_PRINT_ERR, "GET_DEVICE_ID: Invalid parameters\n", 0);
      return AVS_STATUS_ERR;
  }

  if(!stream_id)
      stream_id = &str_id;
  /* Search for root complex block with same segment number, and in whose id */
  /* mapping range 'rid' falls. Calculate the output id */
  block = &g_iovirt_info_table->blocks[0];
  for(i = 0; i < g_iovirt_info_table->num_blocks; i++, block = IOVIRT_NEXT_BLOCK(block))
  {
      if(block->type != IOVIRT_NODE_PCI_ROOT_COMPLEX)
          continue;
      if(block->data.rc.segment != segment)
          continue;
      for(j = 0, map = &block->data_map[0]; j < block->num_data_map; j++, map++)
      {
          if(rid >= (*map).map.input_base && rid <= ((*map).map.input_base + (*map).map.id_count))
          {
              id =  (rid - (*map).map.input_base) + (*map).map.output_base;
              oref = (*map).map.output_ref;
              break;
          }
      }
      if(id)
          break;
  }
  if(!id) {
      val_print(AVS_PRINT_ERR, "GET_DEVICE_ID: Requestor ID mapping not found\n", 0);
      return AVS_STATUS_ERR;
  }
  /* If output reference node is to ITS group, 'id' is device id */
  block = (IOVIRT_BLOCK*)((uint8_t*)g_iovirt_info_table + oref);
  if(block->type == IOVIRT_NODE_ITS_GROUP)
  {
      *device_id = id;
      *stream_id = 0;
      return 0;
  }
  /* If output reference is to SMMU block, 'id' is stream id */
  /* Go through id mappings of this block and find corresponding device id */
  if(block->type == IOVIRT_NODE_SMMU || block->type == IOVIRT_NODE_SMMU_V3)
  {
      *stream_id = id;
      id = 0;
      for(i = 0, map = &block->data_map[0]; i < block->num_data_map; i++, map++)
      {
          if(*stream_id >= (*map).map.input_base && *stream_id <= ((*map).map.input_base +
                                                    (*map).map.id_count))
          {
              *device_id =  (*stream_id - (*map).map.input_base) + (*map).map.output_base;
              return 0;
          }
      }
  }
  val_print(AVS_PRINT_ERR, "GET_DEVICE_ID: Device ID mapping not found\n", 0);
  return AVS_STATUS_ERR;
}

/**
  @brief   This API will call PAL layer to fill in the IO Virt information
           into the g_iovirt_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   iovirt_info_table  pre-allocated memory pointer for iovirt_info
  @return  Error if Input param is NULL
**/
void
val_iovirt_create_info_table(uint64_t *iovirt_info_table)
{

  if (iovirt_info_table == NULL)
  {
      val_print(AVS_PRINT_ERR, "\n   Input for Create Info table cannot be NULL \n", 0);
      return;
  }

  g_iovirt_info_table = (IOVIRT_INFO_TABLE *)iovirt_info_table;

  pal_iovirt_create_info_table(g_iovirt_info_table);

  val_print(AVS_PRINT_TEST,
            " SMMU_INFO: Number of SMMU CTRL       :    %x \n",
            val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0));
}

uint32_t
val_iovirt_check_unique_ctx_intid(uint32_t smmu_index)
{
  uint64_t smmu_block = val_iovirt_get_smmu_info(SMMU_IOVIRT_BLOCK, smmu_index);
  return pal_iovirt_check_unique_ctx_intid(smmu_block);
}

void
val_iovirt_free_info_table()
{
  pal_mem_free((void *)g_iovirt_info_table);
}
