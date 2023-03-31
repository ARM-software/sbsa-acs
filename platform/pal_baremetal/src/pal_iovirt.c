/** @file
 * Copyright (c) 2020-2023, Arm Limited or its affiliates. All rights reserved.
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

#include "include/pal_common_support.h"
#include "include/platform_override_struct.h"

extern PLATFORM_OVERRIDE_IOVIRT_INFO_TABLE platform_iovirt_cfg;
extern PLATFORM_OVERRIDE_NODE_DATA platform_node_type;
extern PLATFORM_OVERRIDE_SMMU_NODE_DATA platform_smmu_node_data;
extern PLATFORM_OVERRIDE_PMCG_NODE_DATA platform_pmcg_node_data;
extern PLATFORM_OVERRIDE_NAMED_NODE_DATA platform_named_node_data;

uint64_t
pal_iovirt_get_rc_smmu_base (
  IOVIRT_INFO_TABLE *Iovirt,
  uint32_t RcSegmentNum,
  uint32_t rid
  )
{
  uint32_t i, j;
  IOVIRT_BLOCK *block;
  NODE_DATA_MAP *map;
  uint32_t mapping_found;
  uint32_t oref, sid, id = 0;

  /* Search for root complex block with same segment number, and in whose id */
  /* mapping range 'rid' falls. Calculate the output id */
  block = &(Iovirt->blocks[0]);
  mapping_found = 0;
  for (i = 0; i < Iovirt->num_blocks; i++, block = IOVIRT_NEXT_BLOCK(block))
  {
      if (block->type == IOVIRT_NODE_PCI_ROOT_COMPLEX
          && block->data.rc.segment == RcSegmentNum)
      {
          for (j = 0, map = &block->data_map[0]; j < block->num_data_map; j++, map++)
          {
              if(rid >= (*map).map.input_base
                      && rid <= ((*map).map.input_base + (*map).map.id_count))
              {
                  id =  rid - (*map).map.input_base + (*map).map.output_base;
                  oref = (*map).map.output_ref;
                  mapping_found = 1;
                  break;
              }
          }
      }
  }

  if (!mapping_found) {
      print(AVS_PRINT_ERR,
               "\n       RID to Stream ID/Dev ID map not found ", 0);
      return 0xFFFFFFFF;

}

  block = (IOVIRT_BLOCK*)((uint8_t*)Iovirt + oref);
  if(block->type == IOVIRT_NODE_SMMU || block->type == IOVIRT_NODE_SMMU_V3)
  {
      sid = id;
      id = 0;
      for(i = 0, map = &block->data_map[0]; i < block->num_data_map; i++, map++)
      {
          if(sid >= (*map).map.input_base && sid <= ((*map).map.input_base +
                                                    (*map).map.id_count))
          {
              print(AVS_PRINT_DEBUG,
                         "\n       find RC block->data.smmu.base : %llx", block->data.smmu.base);
              return block->data.smmu.base;
          }
      }
  }

  /* The Root Complex represented by rc_seg_num
   * is not behind any SMMU. Return NULL pointer
   */
  print(AVS_PRINT_DEBUG,
             " No SMMU found behind the RootComplex with segment :%x", RcSegmentNum);
  return 0;
}


/**
  @brief   Check if the context bank interrupt ids for this smmu node are unique
  @param   ctx_int Context bank interrupt array
  @param   ctx_int Number of elements in the array
  @return  1 if the IDs are unique else 0
**/
static uint8_t
smmu_ctx_int_distinct(uint64_t *ctx_int, uint8_t ctx_int_cnt) {
  uint8_t i, j;
  for(i = 0; i < ctx_int_cnt - 1; i++) {
    for(j = i + 1; j < ctx_int_cnt; j++) {
      if(*((uint32_t *)&ctx_int[i]) == *((uint32_t *)&ctx_int[j]))
        return 0;
    }
  }

  return 1;

}

/**
  @brief  Dump the input block
**/
static void
dump_block(IOVIRT_BLOCK *block) {
  uint32_t i;
  NODE_DATA_MAP *map = &(block->data_map[0]);
  switch(block->type) {
      case IOVIRT_NODE_ITS_GROUP:
      print(AVS_PRINT_INFO, "\n ITS Group: Num ITS:%d\n", (*map).id[0]);
      for(i = 0; i < block->data.its_count; i++)
          print(AVS_PRINT_INFO, "  ITS ID : %d\n", (*map).id[i]);
      return;
      case IOVIRT_NODE_NAMED_COMPONENT:
      print(AVS_PRINT_INFO,
                 " Named Component:\n Device Name:%a", block->data.named_comp.name);
      print(AVS_PRINT_INFO, "\n CCA Attribute: 0x%lx\n", block->data.named_comp.cca);
      break;
      case IOVIRT_NODE_PCI_ROOT_COMPLEX:
      print(AVS_PRINT_INFO,
                 " Root Complex: PCI segment number:%d\n", block->data.rc.segment);
      break;
      case IOVIRT_NODE_SMMU:
      case IOVIRT_NODE_SMMU_V3:
      print(AVS_PRINT_INFO, " SMMU: Major Rev:%d Base Address:0x%x\n",
                 block->data.smmu.arch_major_rev, block->data.smmu.base);
      break;
      case IOVIRT_NODE_PMCG:
      print(AVS_PRINT_INFO,
                 " PMCG: Base:0x%x\n Overflow GSIV:0x%x Node Reference:0x%x\n",
                 block->data.pmcg.base, block->data.pmcg.overflow_gsiv, block->data.pmcg.node_ref);
      break;
  }
  print(AVS_PRINT_INFO,
             " Number of ID Mappings:%d\n", block->num_data_map);
  for(i = 0; i < block->num_data_map; i++, map++) {
      print(AVS_PRINT_INFO,
                 "  input_base:0x%x id_count:0x%x\n  output_base:0x%x output ref:0x%x\n",
            (*map).map.input_base, (*map).map.id_count,
            (*map).map.output_base, (*map).map.output_ref);
  }
  print(AVS_PRINT_INFO, "\n");
}

void
pal_iovirt_create_info_table(IOVIRT_INFO_TABLE *IoVirtTable)
{

  uint64_t iort;
  uint32_t its_count = NUM_ITS_COUNT;
  IOVIRT_BLOCK *block;
  NODE_DATA_MAP *data_map;
  uint32_t node[IORT_NODE_COUNT];
  uint32_t identifier[5][1] ={{0}, {1}, {2}, {3}, {4}};
  char device_name[1][5] = {"DMA"};
  uint32_t j, k = 0, m = 0, i = 0, z = 0, n = 0;

  if (IoVirtTable == NULL)
    return;

  /* Initialize counters */
  IoVirtTable->num_blocks = 0;
  IoVirtTable->num_smmus = 0;
  IoVirtTable->num_pci_rcs = 0;
  IoVirtTable->num_named_components = 0;
  IoVirtTable->num_its_groups = 0;
  IoVirtTable->num_pmcgs = 0;

  iort = platform_iovirt_cfg.Address;
  if(!iort)
  {
     return;
  }

  block = &(IoVirtTable->blocks[0]);
  for (i = 0; i < platform_iovirt_cfg.node_count; i++, block=IOVIRT_NEXT_BLOCK(block))
  {
     block->type = platform_iovirt_cfg.type[i];
     block->flags = 0;
     switch(platform_iovirt_cfg.type[i]){
          case IOVIRT_NODE_ITS_GROUP:
             block->data.its_count = platform_node_type.its_count;
             data_map = &block->data_map[0];
             memcpy(&((*data_map).id[0]), &identifier[i][0], sizeof(uint32_t) * block->data.its_count);
             block->num_data_map = (block->data.its_count +3)/4;
             IoVirtTable->num_its_groups++;
             break;
          case IOVIRT_NODE_NAMED_COMPONENT:
             strncpy(block->data.named_comp.name, device_name[0], MAX_NAMED_COMP_LENGTH);
             block->data.named_comp.cca = (platform_named_node_data.named[n].memory_properties) & IOVIRT_CCA_MASK;
             block->data.named_comp.smmu_base = platform_named_node_data.named[n].smmu_base;
             block->num_data_map = platform_iovirt_cfg.num_map[i];
             IoVirtTable->num_named_components++;
             break;
          case IOVIRT_NODE_PCI_ROOT_COMPLEX:
             block->data.rc.segment = platform_node_type.rc.segment;
             block->data.rc.cca = (platform_node_type.rc.cca & IOVIRT_CCA_MASK);
             block->data.rc.ats_attr = platform_node_type.rc.ats_attr;
             block->num_data_map = platform_iovirt_cfg.num_map[i];
             IoVirtTable->num_pci_rcs++;
             break;
          case IOVIRT_NODE_SMMU:
             block->data.smmu.base = platform_smmu_node_data.smmu[m].base;
             block->data.smmu.arch_major_rev = 2;
             block->num_data_map = platform_iovirt_cfg.num_map[i];
             if(!smmu_ctx_int_distinct(&platform_smmu_node_data.smmu[m].context_interrupt_offset,
                                           platform_smmu_node_data.smmu[m].context_interrupt_count))
             {
                  block->flags |= (1 << IOVIRT_FLAG_SMMU_CTX_INT_SHIFT);
             }
             IoVirtTable->num_smmus++;
             m++;
             break;
          case IOVIRT_NODE_SMMU_V3:
             block->data.smmu.base = platform_smmu_node_data.smmu[k].base;
             block->data.smmu.arch_major_rev = 3;
             block->num_data_map = platform_iovirt_cfg.num_map[i];
             IoVirtTable->num_smmus++;
             k++;
             break;
          case IOVIRT_NODE_PMCG:
             block->data.pmcg.base = platform_pmcg_node_data.pmcg[i].base;
             block->data.pmcg.overflow_gsiv = platform_pmcg_node_data.pmcg[i].overflow_gsiv;
             block->num_data_map = platform_iovirt_cfg.num_map[i];
             IoVirtTable->num_pmcgs++;
             break;
          default:
             print(AVS_PRINT_ERR, "Invalid IORT node type\n");
             return;
     }
     node[i] = (uint8_t*)(block) - (uint8_t*)IoVirtTable;

     if (platform_iovirt_cfg.type[i] == IOVIRT_NODE_PMCG)
     {
         block->data.pmcg.node_ref = node[i];
     }

     if (platform_iovirt_cfg.type[i] != IOVIRT_NODE_ITS_GROUP)
     {
         data_map = (NODE_DATA_MAP *)&(block->data_map[0]);
         for(j = 0; j < block->num_data_map; j++)
         {
             (*data_map).map.input_base = platform_iovirt_cfg.map[i].input_base[j];
             (*data_map).map.id_count = platform_iovirt_cfg.map[i].id_count[j];
             (*data_map).map.output_base = platform_iovirt_cfg.map[i].output_base[j];
             if (platform_iovirt_cfg.type[i] == IOVIRT_NODE_SMMU_V3)
             {
                 (*data_map).map.output_ref = node[z];
             }
             if (platform_iovirt_cfg.type[i] == IOVIRT_NODE_PCI_ROOT_COMPLEX)
             {
                 (*data_map).map.output_ref = node[j + its_count];
             }
             else
             {
                  (*data_map).map.output_ref = platform_iovirt_cfg.map[i].output_ref[j];
             }

             data_map++;
         }
         z++;
      }

     IoVirtTable->num_blocks++;
  }

  block = &(IoVirtTable->blocks[0]);
  print(AVS_PRINT_DEBUG, " Number of IOVIRT blocks = %d\n", IoVirtTable->num_blocks);
  for(i = 0; i < IoVirtTable->num_blocks; i++, block = IOVIRT_NEXT_BLOCK(block))
             dump_block(block);

}

/**
  @brief  Check if given SMMU node has unique context bank interrupt ids

  @param  smmu_block smmu IOVIRT block base address

  @return 0 if test fails, 1 if test passes
**/
uint32_t
pal_iovirt_check_unique_ctx_intid(uint64_t smmu_block)
{
  IOVIRT_BLOCK *block = (IOVIRT_BLOCK *)smmu_block;
  /* This test has already been done while parsing IORT */
  /* Check the flags to get the test result */
  if(block->flags & (1 << IOVIRT_FLAG_SMMU_CTX_INT_SHIFT)) {
    return 0;
  }
  return 1;

}

uint32_t
pal_iovirt_unique_rid_strid_map(uint64_t rc_block)
{
  IOVIRT_BLOCK *block = (IOVIRT_BLOCK *)rc_block;
  if(block->flags & (1 << IOVIRT_FLAG_STRID_OVERLAP_SHIFT))
    return 0;
  return 1;
}
