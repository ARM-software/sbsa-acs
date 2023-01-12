/** @file
 * Copyright (c) 2022 Arm Limited or its affiliates. All rights reserved.
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
#include "Include/IndustryStandard/ArmErrorSourceTable.h"

#include "include/pal_uefi.h"
#include "include/pal_ras.h"
#include "include/platform_override.h"

#define ADD_PTR(t, p, l) ((t*)((UINT8*)p + l))

UINT64 pal_get_aest_ptr();

/**
  @brief  API to check support for Poison

  @param  None

  @return  0 - Poison storage & forwarding not supported
           1 - Poison storage & forwarding supported
**/
UINT32
pal_ras_check_plat_poison_support()
{
  return 0;
}

/**
  @brief  Platform Defined way of setting up the Error Environment

  @param  in_param  - Error Input Parameters.
  @param  *out_param  - Parameters returned from platform to be used in the test.

  @return  0 - Success, NOT_IMPLEMENTED - API not implemented, Other values - Failure
**/
UINT32
pal_ras_setup_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param)
{
  /* Platform Defined way of setting up the Error Environment */
  return NOT_IMPLEMENTED;
}

/**
  @brief  Platform Defined way of injecting up the Error Environment

  @param  in_param  - Error Input Parameters.
  @param  *out_param  - Parameters returned from platform to be used in the test.

  @return  0 - Success, NOT_IMPLEMENTED - API not implemented, Other values - Failure
**/
UINT32
pal_ras_inject_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param)
{
  return NOT_IMPLEMENTED;
}

/**
  @brief  Platform Defined way of Timeout/Wait loop

  @param  count  - Timeout/Wait Multiplier.

  @return None
**/
VOID
pal_ras_wait_timeout(UINT32 count)
{
  UINT32 timeout = count * PLATFORM_TIMEOUT_MEDIUM;

  /* Wait Loop */
  while (timeout > 0)
    timeout--;
}

/**
  @brief  Display RAS info table details

  @param  RasInfoTable  - Address to the RAS information table.

  @return  None
**/
VOID
pal_ras_dump_info_table(RAS_INFO_TABLE *RasInfoTable)
{
  UINT32 i, j;
  RAS_NODE_INFO *curr;

  if (RasInfoTable == NULL) {
      return;
  }

  sbsa_print(AVS_PRINT_INFO, L"\nRAS Info :");
  sbsa_print(AVS_PRINT_INFO, L"\nRAS Num Nodes : %d ", RasInfoTable->num_nodes);

  curr = &(RasInfoTable->node[0]);

  for (i = 0; i < RasInfoTable->num_nodes; i++) {
      sbsa_print(AVS_PRINT_INFO, L"\n Index    : %d ", i);
      sbsa_print(AVS_PRINT_INFO, L"\n Type     : 0x%x ", curr->type);
      sbsa_print(AVS_PRINT_INFO, L"\n Num Intr : 0x%x ", curr->num_intr_entries);

      switch (curr->type) {
       case NODE_TYPE_PE:
           /* Print Processor Node Details */
           sbsa_print(AVS_PRINT_INFO, L"\n ProcessorID : 0x%x ", curr->node_data.pe.processor_id);
           sbsa_print(AVS_PRINT_INFO, L"\n resource_type : 0x%x ",
                                      curr->node_data.pe.resource_type);
           sbsa_print(AVS_PRINT_INFO, L"\n flags : 0x%x ", curr->node_data.pe.flags);
           sbsa_print(AVS_PRINT_INFO, L"\n affinity : 0x%x ", curr->node_data.pe.affinity);
           break;
       case NODE_TYPE_MC:
           /* Print Memory Controller Node Details */
           sbsa_print(AVS_PRINT_INFO, L"\n proximity_domain : 0x%x ",
                                      curr->node_data.mc.proximity_domain);
           break;
       default:
           break;
      }

      sbsa_print(AVS_PRINT_INFO, L"\n Interface Info :");
      sbsa_print(AVS_PRINT_INFO, L"\n  type    : 0x%x ", curr->intf_info.intf_type);
      sbsa_print(AVS_PRINT_INFO, L"\n  base    : 0x%x ", curr->intf_info.base_addr);
      sbsa_print(AVS_PRINT_INFO, L"\n  num_err : 0x%x ", curr->intf_info.num_err_rec);

      sbsa_print(AVS_PRINT_INFO, L"\n Interrupt Info :");
      for (j = 0; j < curr->num_intr_entries; j++) {
        sbsa_print(AVS_PRINT_INFO, L"\n  type    : 0x%x ", curr->intr_info[j].type);
        sbsa_print(AVS_PRINT_INFO, L"\n  gsiv    : 0x%x ", curr->intr_info[j].gsiv);
      }

      curr++;
  }
  sbsa_print(AVS_PRINT_INFO, L"\n");
}

void fill_node_specific_data (
     RAS_INFO_TABLE *RasInfoTable,
     AEST_NODE *aest_node,
     RAS_NODE_INFO *curr_node,
     EFI_ACPI_AEST_NODE_STRUCT *node_header
     )
{
     EFI_ACPI_AEST_PROCESSOR_STRUCT *pe_node;
     EFI_ACPI_AEST_MEMORY_CONTROLLER_STRUCT *mc_node;

     switch (node_header->Type) {
       case EFI_ACPI_AEST_NODE_TYPE_PROCESSOR:
           /* Fill Processor Node Details */
           pe_node = (EFI_ACPI_AEST_PROCESSOR_STRUCT *)aest_node;
           curr_node->type = NODE_TYPE_PE;
           curr_node->node_data.pe.processor_id = pe_node->AcpiProcessorId;
           curr_node->node_data.pe.resource_type = pe_node->ResourceType;
           curr_node->node_data.pe.flags = pe_node->Flags;
           curr_node->node_data.pe.affinity = pe_node->ProcessorAffinityLevelIndicator;
           RasInfoTable->num_pe_node++;
           break;
       case EFI_ACPI_AEST_NODE_TYPE_MEMORY:
           /* Fill Memory Controller Node Details */
           mc_node = (EFI_ACPI_AEST_MEMORY_CONTROLLER_STRUCT *)aest_node;
           curr_node->type = NODE_TYPE_MC;
           curr_node->node_data.mc.proximity_domain = mc_node->ProximityDomain;
           RasInfoTable->num_mc_node++;
           break;
       default:
           break;
     }
}

void fill_node_interface_data (
     RAS_INFO_TABLE *RasInfoTable,
     AEST_NODE *aest_node,
     RAS_NODE_INFO *curr_node,
     EFI_ACPI_AEST_NODE_STRUCT *node_header
     )
{
    EFI_ACPI_AEST_INTERFACE_STRUCT *node_intf;

    node_intf = ADD_PTR(EFI_ACPI_AEST_INTERFACE_STRUCT, aest_node, node_header->InterfaceOffset);

    curr_node->intf_info.intf_type = node_intf->Type;
    curr_node->intf_info.flags = node_intf->Flags;
    curr_node->intf_info.base_addr = node_intf->BaseAddress;
    curr_node->intf_info.start_rec_index = node_intf->StartErrorRecordIndex;
    curr_node->intf_info.num_err_rec = node_intf->NumberErrorRecords;
    curr_node->intf_info.err_rec_implement = node_intf->ErrorRecordImplemented;
    curr_node->intf_info.err_status_reporting = node_intf->ErrorRecordStatusReportingSupported;
    curr_node->intf_info.addressing_mode = node_intf->AddressingMode;
}

void fill_node_interrupt_data (
     RAS_INFO_TABLE *RasInfoTable,
     AEST_NODE *aest_node,
     RAS_NODE_INFO *curr_node,
     EFI_ACPI_AEST_NODE_STRUCT *node_header
     )
{
  EFI_ACPI_AEST_INTERRUPT_STRUCT *node_intr_data;
  UINT32 count = 0;

  node_intr_data = ADD_PTR(EFI_ACPI_AEST_INTERRUPT_STRUCT, aest_node,
                           node_header->InterruptArrayOffset);

  for (count = 0; count < curr_node->num_intr_entries; count++)
  {
    curr_node->intr_info[count].type = node_intr_data->InterruptType;
    curr_node->intr_info[count].gsiv = node_intr_data->InterruptGsiv;
    curr_node->intr_info[count].flag = node_intr_data->InterruptFlags;
    curr_node->intr_info[count].its_grp_id = node_intr_data->ItsGroupRefId;

    node_intr_data = ADD_PTR(EFI_ACPI_AEST_INTERRUPT_STRUCT, node_intr_data,
                             sizeof(EFI_ACPI_AEST_INTERRUPT_STRUCT));
  }

}
/**
  @brief  This API fills in the RAS_INFO_TABLE with information about local and system
          timers in the system. This is achieved by parsing the ACPI - AEST table.

  @param  RasInfoTable  - Address where the RAS information needs to be filled.

  @return  None
**/
VOID
pal_ras_create_info_table(RAS_INFO_TABLE *RasInfoTable)
{
  EFI_ACPI_ARM_ERROR_SOURCE_TABLE *aest;
  AEST_NODE *aest_node, *aest_end;
  EFI_ACPI_AEST_NODE_STRUCT *node_header;
  RAS_NODE_INFO *curr_node;

  if (RasInfoTable == NULL) {
      sbsa_print(AVS_PRINT_ERR, L"\n Input RAS Table Pointer is NULL");
      return;
  }

  /* Initialize RasInfoTable */
  RasInfoTable->num_nodes = 0;
  RasInfoTable->num_pe_node = 0;
  RasInfoTable->num_mc_node = 0;

  aest = (EFI_ACPI_ARM_ERROR_SOURCE_TABLE *)pal_get_aest_ptr();
  if (aest == NULL) {
      sbsa_print(AVS_PRINT_DEBUG, L" AEST table not found\n");
      return;
  }

  /* Pointer to the first AEST node */
  aest_node = ADD_PTR(AEST_NODE, aest, sizeof(EFI_ACPI_DESCRIPTION_HEADER));
  aest_end = ADD_PTR(AEST_NODE, aest, aest->Header.Length);

  curr_node = &(RasInfoTable->node[0]);

  /* Create RAS info block  for each AEST node*/
  while (aest_node < aest_end) {
      /* Add common data for each node */
      node_header = (EFI_ACPI_AEST_NODE_STRUCT *)(aest_node);

      curr_node->type = node_header->Type;
      curr_node->length = node_header->Length;
      curr_node->num_intr_entries = node_header->InterruptArrayCount;

      /* Fill Node Specific data */
      fill_node_specific_data(RasInfoTable, aest_node, curr_node, node_header);

      /* Fill Node Interface data */
      fill_node_interface_data(RasInfoTable, aest_node, curr_node, node_header);

      /* Fill Node Interrupt data */
      fill_node_interrupt_data(RasInfoTable, aest_node, curr_node, node_header);

      RasInfoTable->num_nodes++;

      if (RasInfoTable->num_nodes >= MAX_NUM_OF_RAS_SUPPORTED) {
          sbsa_print(AVS_PRINT_WARN, L"\n Number of RAS nodes greater than %d",
                     MAX_NUM_OF_RAS_SUPPORTED);
          break;
      }

      /* Move to the next AEST node */
      aest_node = ADD_PTR(AEST_NODE, aest_node, curr_node->length);
      curr_node++;
  }

  pal_ras_dump_info_table(RasInfoTable);
}

/**
  @brief  Display RAS info table details

  @param  RasInfoTable  - Address to the RAS information table.

  @return  None
**/
VOID
pal_ras2_dump_info_table(RAS2_INFO_TABLE *RasFeatInfoTable)
{
  RAS2_BLOCK *curr_block;

  if (RasFeatInfoTable == NULL) {
      sbsa_print(AVS_PRINT_ERR, L"\n Input RAS Table Pointer is NULL");
      return;
  }

  curr_block = RasFeatInfoTable->blocks;
  UINT32 i;

  sbsa_print(AVS_PRINT_INFO, L"\nRAS2 Feature Info :");
  sbsa_print(AVS_PRINT_INFO,
             L"\n Total number of RAS2 feature info blocks  : %d",
             RasFeatInfoTable->num_all_block);
  sbsa_print(AVS_PRINT_INFO,
             L"\n Number of RAS2 memory feature info blocks : %d\n",
             RasFeatInfoTable->num_of_mem_block);

  /*Iterate RAS2 feature info table and print info fields */
  for (i = 0 ; i < RasFeatInfoTable->num_all_block ; i++) {
    sbsa_print(AVS_PRINT_INFO, L"\n RAS2 feature info * Index %d *", i);
    switch(curr_block->type) {
    case RAS2_FEATURE_TYPE_MEMORY:
        sbsa_print(AVS_PRINT_INFO, L"\n  Type                            : 0x%x",
                   curr_block->type);
        sbsa_print(AVS_PRINT_INFO, L"\n  Proximity Domain                : 0x%llx",
                   curr_block->block_info.mem_feat_info.proximity_domain);
        sbsa_print(AVS_PRINT_INFO, L"\n  Patrol scrub support            : 0x%lx\n",
                   curr_block->block_info.mem_feat_info.patrol_scrub_support);
        break;
    default:
        sbsa_print(AVS_PRINT_INFO,
             L"\n  Invalid RAS feature type : 0x%x",
                   curr_block->type);
    }
    curr_block++;
  }
}



/**
  @brief  This is a helper function to update RAS2_INFO_TABLE block
          with required PCCT subspace information for memory instance.
          Caller : pal_ras2_create_info_table()
  @param  pcct            - Pointer to PCCT ACPI table
  @param  curr_block      - Pointer to RAS2_BLOCK block.
  @param  pcct_array_idx  - Index of PCCT entry to parse.

  @return  None
**/
VOID pal_ras2_fill_mem_pcct_info(
     EFI_ACPI_6_4_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER *pcct,
     RAS2_BLOCK *curr_block,
     UINT8 pcct_array_idx
     )
{

  EFI_ACPI_6_4_PCCT_SUBSPACE_GENERIC *pcct_subspace, *pcct_end;
  RAS2_PCC_SHARED_MEMORY_REGION *ras2_pcc_shared_mem = NULL;
  UINT32 index = 0;

  /* initialise RAS2_MEM_INFO PCCT info fields */
  curr_block->block_info.mem_feat_info.patrol_scrub_support = 0;

  /* pointer to start of PCC subspace structure entries */
  pcct_subspace = ADD_PTR(EFI_ACPI_6_4_PCCT_SUBSPACE_GENERIC, pcct,
                          sizeof(EFI_ACPI_6_4_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER));
  pcct_end =  ADD_PTR(EFI_ACPI_6_4_PCCT_SUBSPACE_GENERIC, pcct,
                          pcct->Header.Length);

  while (pcct_subspace < pcct_end) {
    if (index == pcct_array_idx) {
    /* get base address of Generic Communications Channel Shared Memory Region
       i.e., RAS2 Platform Communication Channel Shared Memory Region in RAS
       context */
      ras2_pcc_shared_mem = (RAS2_PCC_SHARED_MEMORY_REGION * )pcct_subspace->BaseAddress;
      /* check if memory instance supports PATROL_SCRUB RAS feature */
      /* RasFeatures[1] is lower 64bits in 128bits member */
      if (ras2_pcc_shared_mem->RasFeatures[1] & RAS2_PLATFORM_FEATURE_PATROL_SCRUB_BITMASK)
          curr_block->block_info.mem_feat_info.patrol_scrub_support = 1;
      return;
    }
    /* point to next PCC subspace entry */
    pcct_subspace = ADD_PTR(EFI_ACPI_6_4_PCCT_SUBSPACE_GENERIC, pcct_subspace,
                          pcct_subspace->Length);
    index++;
  }

  if (ras2_pcc_shared_mem == NULL) {
      sbsa_print(AVS_PRINT_ERR, L"\n No PCC subspace found for PCCT index : 0x%x", pcct_array_idx);
      return;
  }

  return;
}

/**
  @brief  This API fills in the RAS_INFO_TABLE with memory patrol scrub info
          from parsing RAS2 ACPI table.

  @return  None
**/
VOID
pal_ras2_create_info_table(RAS2_INFO_TABLE *RasFeatInfoTable)
{
  RAS_FEATURE_2_TABLE_HEADER *ras2;
  RAS2_PCC_DESCRIPTOR *ras2_pcc_descp;
  RAS2_BLOCK *curr_block;
  EFI_ACPI_6_4_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER *pcct;


  UINT32 i;
  UINT8 pcct_array_idx;

  if (RasFeatInfoTable == NULL) {
      sbsa_print(AVS_PRINT_ERR, L"\n Input RAS Table Pointer is NULL");
      return;
  }

  /* initialise RAS2_INFO_TABLE fields */
  RasFeatInfoTable->num_all_block = 0;
  RasFeatInfoTable->num_of_mem_block = 0;

  ras2 = (RAS_FEATURE_2_TABLE_HEADER *)pal_get_acpi_table_ptr(EFI_ACPI_6_5_RAS2_FEATURE_TABLE_SIGNATURE);
  if (ras2 == NULL) {
      sbsa_print(AVS_PRINT_DEBUG, L" RAS2 ACPI table not found\n");
      return;
  }

  pcct = (EFI_ACPI_6_4_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER * )
          pal_get_acpi_table_ptr(EFI_ACPI_6_4_PLATFORM_COMMUNICATIONS_CHANNEL_TABLE_SIGNATURE);
  if (pcct == NULL) {
      sbsa_print(AVS_PRINT_DEBUG, L" PCCT ACPI table not found\n");
      return;
  }
  /* pointer to the start of RAS2 PCC descriptor array */
  ras2_pcc_descp = ADD_PTR(RAS2_PCC_DESCRIPTOR, ras2, sizeof(RAS_FEATURE_2_TABLE_HEADER));

  /* point to first info block */
  curr_block = RasFeatInfoTable->blocks;

  /* Iterate PCC descriptors and parse corresponding PCC info for each proxomity domain */
  for (i = 0; i < ras2->NumOfPccDescriptors; i++, ras2_pcc_descp++) {
      if (ras2_pcc_descp->FeatureType == RAS2_FEATURE_TYPE_MEMORY) {
          /* Parse required fields from RAS2 ACPI table*/
          curr_block->type = ras2_pcc_descp->FeatureType;
          curr_block->block_info.mem_feat_info.proximity_domain = ras2_pcc_descp->Instance;
          pcct_array_idx = ras2_pcc_descp->PccIdentifier;
          pal_ras2_fill_mem_pcct_info(pcct, curr_block, pcct_array_idx);

          /* increment block count and move the pointer to next info block*/
          RasFeatInfoTable->num_all_block++;
          RasFeatInfoTable->num_of_mem_block++;
          curr_block++;
      }
  }
  pal_ras2_dump_info_table(RasFeatInfoTable);
}
