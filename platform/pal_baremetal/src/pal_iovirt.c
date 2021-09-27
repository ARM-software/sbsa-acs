/** @file
 * Copyright (c) 2020, 2021 Arm Limited or its affiliates. All rights reserved.
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
#include "FVP/include/platform_override_fvp.h"

extern PLATFORM_OVERRIDE_IOVIRT_INFO_TABLE platform_iovirt_cfg;
extern PLATFORM_OVERRIDE_NODE_DATA platform_node_type;

uint64_t
pal_iovirt_get_rc_smmu_base (
  IOVIRT_INFO_TABLE *Iovirt,
  uint32_t RcSegmentNum,
  uint32_t rid
  )
{
  return IOVIRT_SMMUV3_BASE_ADDRESS;
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

void
pal_iovirt_create_info_table(IOVIRT_INFO_TABLE *IoVirtTable)
{

  uint64_t iort;
  IOVIRT_BLOCK *block;
  NODE_DATA_MAP *data_map;
  uint32_t j, i=0;

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
             block->num_data_map = (block->data.its_count +3)/4;
             IoVirtTable->num_its_groups++;
             break;
          case IOVIRT_NODE_NAMED_COMPONENT:
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
             block->data.smmu.base = platform_node_type.smmu.base;
             block->data.smmu.arch_major_rev = 2;
             block->num_data_map = platform_iovirt_cfg.num_map[i];
             if(!smmu_ctx_int_distinct(&platform_node_type.smmu.context_interrupt_offset,platform_node_type.smmu.context_interrupt_count))
             {
                  block->flags |= (1 << IOVIRT_FLAG_SMMU_CTX_INT_SHIFT);
             }
             IoVirtTable->num_smmus++;
             break;
          case IOVIRT_NODE_SMMU_V3:
             block->data.smmu.base = platform_node_type.smmu.base;
             block->data.smmu.arch_major_rev = 3;
             block->num_data_map = platform_iovirt_cfg.num_map[i];
             IoVirtTable->num_smmus++;
             break;
          case IOVIRT_NODE_PMCG:
             block->num_data_map = platform_iovirt_cfg.num_map[i];
             IoVirtTable->num_pmcgs++;
             break;
          default:
             print(AVS_PRINT_ERR, "Invalid IORT node type\n");
             return;
     }
     if (platform_iovirt_cfg.type[i] != IOVIRT_NODE_ITS_GROUP)
     {
         data_map = (NODE_DATA_MAP *)&(block->data_map[0]);
         for(j = 0; j < block->num_data_map; j++)
         {
             (*data_map).map.input_base = platform_iovirt_cfg.map[i].input_base[j];
             (*data_map).map.id_count = platform_iovirt_cfg.map[i].id_count[j];
             (*data_map).map.output_base = platform_iovirt_cfg.map[i].output_base[j];
             (*data_map).map.output_ref = platform_iovirt_cfg.map[i].output_ref[j];
             data_map++;
         }
      }

     IoVirtTable->num_blocks++;
  }
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
