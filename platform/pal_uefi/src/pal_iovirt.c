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
#include <Protocol/HardwareInterrupt.h>

#include "Include/IndustryStandard/Acpi61.h"

#include "include/pal_uefi.h"
#include "include/platform_override.h"
#include "include/pal_iovirt.h"

#define ADD_PTR(t, p, l) ((t*)((UINT8*)p + l))

UINT64 pal_get_iort_ptr();

STATIC VOID
iovirt_create_override_table(IOVIRT_INFO_TABLE *table) {
  IOVIRT_BLOCK *block;

  table->num_blocks = 1;
  table->num_smmus = 1;
  block = &table->blocks[0];
  block->data.smmu.base = PLATFORM_OVERRIDE_SMMU_BASE;
  block->data.smmu.arch_major_rev = PLATFORM_OVERRIDE_SMMU_ARCH_MAJOR;
}

/**
  @brief  Dump the input block
**/
STATIC VOID
dump_block(IOVIRT_BLOCK *block) {
  UINT32 i;
  NODE_DATA_MAP *map = &block->data_map[0];
  switch(block->type) {
      case IOVIRT_NODE_ITS_GROUP:
      sbsa_print(AVS_PRINT_INFO, L"\nITS Group:\n Num ITS:%d\n", (*map).id[0]);
      for(i = 0; i < block->data.its_count; i++)
          sbsa_print(AVS_PRINT_INFO, L"%d ", (*map).id[i]);
      sbsa_print(AVS_PRINT_INFO, L"\n");
      return;
      case IOVIRT_NODE_NAMED_COMPONENT:
      sbsa_print(AVS_PRINT_INFO, L"\nNamed Component:\n Device Name:%a\n", block->data.name);
      break;
      case IOVIRT_NODE_PCI_ROOT_COMPLEX:
      sbsa_print(AVS_PRINT_INFO, L"\nRoot Complex:\n PCI segment number:%d\n", block->data.rc.segment);
      break;
      case IOVIRT_NODE_SMMU:
      case IOVIRT_NODE_SMMU_V3:
      sbsa_print(AVS_PRINT_INFO, L"\nSMMU:\n Major Rev:%d\n Base Address:0x%x\n",
                 block->data.smmu.arch_major_rev, block->data.smmu.base);
      break;
      case IOVIRT_NODE_PMCG:
      sbsa_print(AVS_PRINT_INFO, L"\nPMCG:\n Base:0x%x\n Overflow GSIV:0x%x\n Node Reference:0x%x\n",
                 block->data.pmcg.base, block->data.pmcg.overflow_gsiv, block->data.pmcg.node_ref);
      break;
  }
  sbsa_print(AVS_PRINT_INFO, L"Number of ID Mappings:%d\n", block->num_data_map);
  for(i = 0; i < block->num_data_map; i++, map++) {
      sbsa_print(AVS_PRINT_INFO, L"\n input_base:0x%x\n id_count:0x%x\n output_base:0x%x\n output ref:0x%x\n",
            (*map).map.input_base, (*map).map.id_count,
            (*map).map.output_base, (*map).map.output_ref);
  }
  sbsa_print(AVS_PRINT_INFO, L"\n");
}

STATIC UINTN
smmu_ctx_int_distinct(UINT64 *ctx_int, INTN ctx_int_cnt) {
  INTN i, j;
  for(i = 0; i < ctx_int_cnt - 1; i++) {
    for(j = i + 1; j < ctx_int_cnt; j++) {
      if(*((UINT32*)&ctx_int[i]) == *((UINT32*)&ctx_int[j]))
        return 0;
    }
  }
  return 1;
}

STATIC VOID
dump_iort_table(IOVIRT_INFO_TABLE *iovirt)
{
  UINT32 i;
  IOVIRT_BLOCK *block = &iovirt->blocks[0];
  sbsa_print(AVS_PRINT_INFO, L"Number of IOVIRT blocks = %d\n", iovirt->num_blocks);
  for(i = 0; i < iovirt->num_blocks; i++, block = IOVIRT_NEXT_BLOCK(block))
    dump_block(block);
}

/**
  @brief  Check ID mappings in all blocks for any overlap of ID ranges
  @param iort IoVirt table
**/
STATIC VOID
check_mapping_overlap(IOVIRT_INFO_TABLE *iovirt)
{
  IOVIRT_BLOCK *key_block = &iovirt->blocks[0], *block, *tmp;
  NODE_DATA_MAP *key_map = &key_block->data_map[0], *map;
  UINT32 n_key_blocks, n_blocks, n_key_maps, n_maps;
  UINT32 key_start, key_end, start, end;
  /* Starting from first block, compare each mapping with all the */
  /* mappings that follow it in the table */
  for(key_block = &iovirt->blocks[0], n_key_blocks = iovirt->num_blocks;
      n_key_blocks > 0;
      key_block = IOVIRT_NEXT_BLOCK(key_block), n_key_blocks--)
  {
    if(key_block->type == IOVIRT_NODE_ITS_GROUP)
      continue;
    for(key_map = &key_block->data_map[0], n_key_maps = key_block->num_data_map;
        n_key_maps > 0;
        key_map++, n_key_maps--)
    {
      key_start = (*key_map).map.output_base;
      key_end = key_start + (*key_map).map.id_count - 1;
      for(block = key_block, n_blocks = n_key_blocks;
          n_blocks > 0;
          block = IOVIRT_NEXT_BLOCK(block), n_blocks--)
      {
        if(block->type == IOVIRT_NODE_ITS_GROUP)
          continue;
        n_maps = block->num_data_map;
        map = &block->data_map[0];
        if(block == key_block) {
          map = key_map+1;
          n_maps--;
        }
        for(;n_maps > 0; map++, n_maps--)
        {
          if((*map).map.output_ref != (*key_map).map.output_ref)
            continue;
          start = (*map).map.output_base;
          end = start + (*map).map.id_count - 1;
          if((key_start >= start && key_start <= end) ||
             (key_end >= start && key_end <= end) ||
             (key_start < start && key_end > end))
          {
            tmp = ADD_PTR(IOVIRT_BLOCK, iovirt, (*map).map.output_ref);
            if(tmp->type == IOVIRT_NODE_ITS_GROUP) {
               key_block->flags |= (1 << IOVIRT_FLAG_DEVID_OVERLAP_SHIFT);
               block->flags |= (1 << IOVIRT_FLAG_DEVID_OVERLAP_SHIFT);
               sbsa_print(AVS_PRINT_INFO, L"\nOverlapping device ids %x-%x and %x-%x \n",
                          key_start, key_end, start, end);
            }
            else {
               key_block->flags |= (1 << IOVIRT_FLAG_STRID_OVERLAP_SHIFT);
               block->flags |= (1 << IOVIRT_FLAG_STRID_OVERLAP_SHIFT);
               sbsa_print(AVS_PRINT_INFO, L"\nOverlapping stream ids %x-%x and %x-%x \n",
                          key_start, key_end, start, end);
            }
          }
        }
      }
    }
  }
}

/**
  @brief Find block in IovirtTable
  @param key Block to search
  @param IoVirtTable Table to in which block is to be searched
  @return offset of block, if found
          0, if block not found
**/
STATIC UINT32
find_block(IOVIRT_BLOCK *key, IOVIRT_INFO_TABLE *IoVirtTable) {
  IOVIRT_BLOCK *block = &IoVirtTable->blocks[0];
  UINT8 *cmp_end;
  UINT32 i, cmp_size;
  for(i = 0; i < IoVirtTable->num_blocks; i++, block = IOVIRT_NEXT_BLOCK(block)) {
    cmp_end = (UINT8*) &block->flags;
    cmp_size = cmp_end - (UINT8*)block;
    if(key->type == block->type) {
       /* Compare identfiers array as well in case of ITS group */
       if(block->type == IOVIRT_NODE_ITS_GROUP)
          cmp_size += (block->data.its_count * sizeof(UINT32) + sizeof(block->flags));
       if(!CompareMem(key, block, cmp_size))
          return (UINT8*)block - (UINT8*)IoVirtTable;
    }
  }
  return 0;
}

/**
  @brief  Add iort block for given iort node

  @param  *iort         ACPI IORT table base pointer
  @param  *iort_node    IORT node base address pointer
  @param  *IoVirtTable  IO Virt Table base address pointer
  @param  **block       Pointer to IOVIRT block base address pointer,
                        where IOVIRT block is to be added. This is modified
                        to the next address where new IOVIRT block
                        can be created.
  @return offset from the IOVirt Table base address to the IOVIRT block
          base address passed in **block
          OR
          offset from the IO Virt Table base address to the IOVIRT block
          base address where this block is already present in the
          table.
**/
STATIC UINT32
iort_add_block(IORT_TABLE *iort, IORT_NODE *iort_node, IOVIRT_INFO_TABLE *IoVirtTable, IOVIRT_BLOCK **block)
{
  UINT32 offset, *count;
  IOVIRT_BLOCK *next_block;
  NODE_DATA_MAP *data_map = &((*block)->data_map[0]);
  NODE_DATA *data = &((*block)->data);
  VOID *node_data = &(iort_node->node_data[0]);

  sbsa_print(AVS_PRINT_INFO, L"IORT node offset:%x, type: %d\n", (UINT8*)iort_node - (UINT8*)iort, iort_node->type);

  SetMem(data, 0, sizeof(NODE_DATA));

  /* Populate the fields that are independent of node type */
  (*block)->type = iort_node->type;
  (*block)->num_data_map = iort_node->mapping_count;
  /* Populate fields dependent on node type */
  switch(iort_node->type) {
    case IOVIRT_NODE_ITS_GROUP:
      (*data).its_count = ((IORT_ITS_GROUP*)node_data)->its_count;
      /* ITS Group does not have ID mappings, but variable length array of identifiers */
      /* Populate the array here itself, and we are done with ITS group node */
      CopyMem(&(*data_map).id[0], &((IORT_ITS_GROUP*)node_data)->identifiers[0], sizeof(UINT32) * (*data).its_count);
      /* Override the num_data_map value. For every 4 ITS identifiers, */
      /* we have one data map */
      (*block)->num_data_map = ((*data).its_count + 3)/4;
      count = &IoVirtTable->num_its_groups;
      break;
    case IOVIRT_NODE_NAMED_COMPONENT:
      AsciiStrnCpyS((CHAR8*)(*data).name, 16, (CHAR8*)((IORT_NAMED_COMPONENT*)node_data)->device_name, 16);
      count = &IoVirtTable->num_named_components;
      break;
    case IOVIRT_NODE_PCI_ROOT_COMPLEX:
      (*data).rc.segment = ((IORT_ROOT_COMPLEX*)node_data)->pci_segment_number;
      (*data).rc.cca = (UINT32)(((IORT_ROOT_COMPLEX*)node_data)->memory_properties & IOVIRT_CCA_MASK);
      (*data).rc.ats_attr = ((IORT_ROOT_COMPLEX*)node_data)->ats_attribute;
      count = &IoVirtTable->num_pci_rcs;
      break;
    case IOVIRT_NODE_SMMU:
      (*data).smmu.base = ((IORT_SMMU *)node_data)->base_address;
      (*data).smmu.arch_major_rev = 2;
      count = &IoVirtTable->num_smmus;
      break;
    case IOVIRT_NODE_SMMU_V3:
      (*data).smmu.base = ((IORT_SMMU *)node_data)->base_address;
      (*data).smmu.arch_major_rev = 3;
      count = &IoVirtTable->num_smmus;
      break;
    case IOVIRT_NODE_PMCG:
      (*data).pmcg.base = ((IORT_PMCG *)node_data)->base_address;
      (*data).pmcg.overflow_gsiv = ((IORT_PMCG *)node_data)->overflow_interrupt_gsiv;
      (*data).pmcg.node_ref = ((IORT_PMCG *)node_data)->node_reference;
      next_block = ADD_PTR(IOVIRT_BLOCK, data_map, (*block)->num_data_map * sizeof(NODE_DATA_MAP));
      offset = iort_add_block(iort, ADD_PTR(IORT_NODE, iort, (*data).pmcg.node_ref),
                              IoVirtTable, &next_block);
      (*data).pmcg.node_ref = offset;
      count = &IoVirtTable->num_pmcgs;
      break;
    default:
       sbsa_print(AVS_PRINT_ERR, L"Invalid IORT node type\n");
       return (UINT32) -1;
  }

  (*block)->flags = 0;
  /* Have we already added this block? */
  /* If so, return the block offset */
  offset = find_block(*block, IoVirtTable);
  if(offset)
    return offset;

  /* Calculate the position where next block should be added */
  next_block = ADD_PTR(IOVIRT_BLOCK, data_map, (*block)->num_data_map * sizeof(NODE_DATA_MAP));
  if(iort_node->type == IOVIRT_NODE_SMMU) {
    /* Check if the context bank interrupt ids for this smmu node are unique. Set the flags accordingly */
    if(!smmu_ctx_int_distinct(ADD_PTR(UINT64, iort_node, ((IORT_SMMU *)node_data)->context_interrupt_offset),
                              ((IORT_SMMU *)node_data)->context_interrupt_count))
    {
      (*block)->flags |= (1 << IOVIRT_FLAG_SMMU_CTX_INT_SHIFT);
    }
  }

  if((*block)->type != IOVIRT_NODE_ITS_GROUP) {
    IORT_ID_MAPPING *map = ADD_PTR(IORT_ID_MAPPING, iort_node, iort_node->mapping_offset);
    /* For each id mapping copy the fields to corresponding data map fields */
    for(UINT32 i = 0; i < (*block)->num_data_map; i++) {
      (*data_map).map.input_base = map->input_base;
      (*data_map).map.id_count = map->id_count;
      (*data_map).map.output_base = map->output_base;
      /* We don't know if the iort node referred to by map->output_*/
      /* reference is already added as a block. So try to add it and */
      /* store the returned offset in the relevant data map field. */
      /* We know this function will return offset of newly block or */
      /* already added block */
      offset = iort_add_block(iort,
               ADD_PTR(IORT_NODE, iort, map->output_reference),
               IoVirtTable,
               &next_block);
      (*data_map).map.output_ref = offset;
      data_map++;
      map++;
    }
  }
  /* So we successfully added a new block. Calculate its offset */
  offset = (UINT8*)(*block) - (UINT8*)IoVirtTable;
  /* Inform the caller about the address at which next block must be added */
  *block = next_block;
  /* Increment the general and type specific block counters */
  IoVirtTable->num_blocks++;
  *count =  *count + 1;
  return offset;
}

/**
  @brief  Parses ACPI IORT table and populates the local iovirt table
**/
VOID
pal_iovirt_create_info_table(IOVIRT_INFO_TABLE *IoVirtTable)
{
  IORT_TABLE  *iort;
  IORT_NODE   *iort_node, *iort_end;
  IOVIRT_BLOCK  *next_block;
  UINT32 i;

  if (IoVirtTable == NULL)
    return;

  /* Initialize counters */
  IoVirtTable->num_blocks = 0;
  IoVirtTable->num_smmus = 0;
  IoVirtTable->num_pci_rcs = 0;
  IoVirtTable->num_named_components = 0;
  IoVirtTable->num_its_groups = 0;
  IoVirtTable->num_pmcgs = 0;

  if(PLATFORM_OVERRIDE_SMMU_BASE) {
    iovirt_create_override_table(IoVirtTable);
    return;
  }

  iort = (IORT_TABLE *)pal_get_iort_ptr();

  if (iort == NULL) {
    return;
  }

  /* Point to the first Iovirt table block */
  next_block = &(IoVirtTable->blocks[0]);

  /* Point to the first IORT node */
  iort_node = ADD_PTR(IORT_NODE, iort, iort->node_offset);
  iort_end = ADD_PTR(IORT_NODE, iort, iort->header.Length);
  /* Create iovirt block for each IORT node*/
  for (i = 0; i < iort->node_count; i++) {
    if (iort_node >= iort_end) {
      sbsa_print(AVS_PRINT_ERR, L"Bad IORT table \n");
      return;
    }
    iort_add_block(iort, iort_node, IoVirtTable, &next_block);
    iort_node = ADD_PTR(IORT_NODE, iort_node, iort_node->length);
  }
  dump_iort_table(IoVirtTable);
  check_mapping_overlap(IoVirtTable);
}

/**
  @brief  Check if given SMMU node has unique context bank interrupt ids

  @param  smmu_block smmu IOVIRT block base address

  @return 0 if test fails, 1 if test passes
**/
UINT32
pal_iovirt_check_unique_ctx_intid(UINT64 smmu_block)
{
  IOVIRT_BLOCK *block = (IOVIRT_BLOCK *)smmu_block;
  /* This test has already been done while parsing IORT */
  /* Check the flags to get the test result */
  if(block->flags & (1 << IOVIRT_FLAG_SMMU_CTX_INT_SHIFT))
    return 0;
  return 1;
}

/**
  @brief  Check if given root complex node has unique requestor id to stream id mapping

  @param  rc_block root complex IOVIRT block base address

  @return 0 if test fails, 1 if test passes
**/

UINT32
pal_iovirt_unique_rid_strid_map(UINT64 rc_block)
{
  IOVIRT_BLOCK *block = (IOVIRT_BLOCK *)rc_block;
  if(block->flags & (1 << IOVIRT_FLAG_STRID_OVERLAP_SHIFT))
    return 0;
  return 1;
}
