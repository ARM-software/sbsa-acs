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
#include "include/platform_override.h"
#include "include/pal_pmu.h"

#define ADD_PTR(t, p, l) ((t*)((UINT8*)p + l))
#define MAX_NUM_OF_PMU_SUPPORTED  512

UINT64 pal_get_apmt_ptr();

/**
  @brief  Display PMU info table details

  @param  PmuTable  - Address to the PMU information table.

  @return  None
**/
VOID
pal_pmu_dump_info_table(PMU_INFO_TABLE *PmuTable)
{
  UINT32 i;

  if (PmuTable == NULL) {
      return;
  }

  for (i = 0; i < PmuTable->pmu_count; i++) {
      sbsa_print(AVS_PRINT_INFO, L"\nPMU info Index      :%d ", i);
      sbsa_print(AVS_PRINT_INFO, L"\nPMU node type       :%02X ", PmuTable->info[i].type);
      sbsa_print(AVS_PRINT_INFO, L"\nDual page extension :%d ",
                 PmuTable->info[i].dual_page_extension);
      sbsa_print(AVS_PRINT_INFO, L"\nBase Address 0      :%llX ", PmuTable->info[i].base0);
      if(PmuTable->info[i].dual_page_extension)
          sbsa_print(AVS_PRINT_INFO, L"\nBase Address 1      :%llX ", PmuTable->info[i].base1);
      sbsa_print(AVS_PRINT_INFO, L"\nPrimary Instance    :%llX ",
                 PmuTable->info[i].primary_instance);
      sbsa_print(AVS_PRINT_INFO, L"\nSecondary Instance  :%08X ",
                 PmuTable->info[i].secondary_instance);

  }
}

/**
  @brief  This API fills in the PMU_INFO_TABLE with information about local and system
          timers in the system. This is achieved by parsing the ACPI - APMT table.

  @param  PmuTable  - Address where the PMU information needs to be filled.

  @return  None
**/
VOID
pal_pmu_create_info_table(PMU_INFO_TABLE *PmuTable)
{
  APMT_TABLE *apmt;
  APMT_NODE *apmt_node, *apmt_end;

  if (PmuTable == NULL) {
      sbsa_print(AVS_PRINT_ERR, L"\n Input PMU Table Pointer is NULL");
      return;
  }

  /* Initialize PmuTable */
  PmuTable->pmu_count = 0;

  apmt = (APMT_TABLE *)pal_get_apmt_ptr();
  if (apmt == NULL) {
      sbsa_print(AVS_PRINT_DEBUG, L" APMT table not found\n");
      return;
  }

  /* Pointer to the first APMT node */
  apmt_node = ADD_PTR(APMT_NODE, apmt, sizeof(EFI_ACPI_DESCRIPTION_HEADER));
  apmt_end = ADD_PTR(APMT_NODE, apmt, apmt->header.Length);

  /* Create PMU info block  for each APMT node*/
  while (apmt_node < apmt_end) {
      PmuTable->info[PmuTable->pmu_count].type = apmt_node->type;
      PmuTable->info[PmuTable->pmu_count].dual_page_extension = apmt_node->flags & 1;
      PmuTable->info[PmuTable->pmu_count].base0 = apmt_node->base_addr0;
      PmuTable->info[PmuTable->pmu_count].base1 = apmt_node->base_addr1;
      PmuTable->info[PmuTable->pmu_count].primary_instance = apmt_node->primary_instance;
      PmuTable->info[PmuTable->pmu_count].secondary_instance = apmt_node->secondary_instance;
      PmuTable->pmu_count++;
      if (PmuTable->pmu_count >= MAX_NUM_OF_PMU_SUPPORTED) {
          sbsa_print(AVS_PRINT_WARN, L"\n Number of PMUs greater than %d",
                     MAX_NUM_OF_PMU_SUPPORTED);
          break;
      }
      apmt_node = ADD_PTR(APMT_NODE, apmt_node, apmt_node->length);
  }
  pal_pmu_dump_info_table(PmuTable);
}

typedef struct{
    PMU_NODE_INFO_TYPE node_type;
    PMU_EVENT_TYPE_e event_desc;
    UINT32 event_id;
}event_details;

/* Array containing the details of implementation defined system PMU events */
event_details event_list[] = {
  {PMU_NODE_MEM_CNTR, PMU_EVENT_IB_TOTAL_BW,  PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_OB_TOTAL_BW,  PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_IB_READ_BW,   PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_IB_WRITE_BW,  PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_OB_READ_BW,   PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_OB_WRITE_BW,  PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_IB_OPEN_TXN,  PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_IB_TOTAL_TXN, PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_OB_OPEN_TXN,  PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_OB_TOTAL_TXN, PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_LOCAL_BW,     PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_REMOTE_BW,    PMU_EVENT_INVALID},
  {PMU_NODE_MEM_CNTR, PMU_EVENT_ALL_BW,       PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_IB_TOTAL_BW,  PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_OB_TOTAL_BW,  PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_IB_READ_BW,   PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_IB_WRITE_BW,  PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_OB_READ_BW,   PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_OB_WRITE_BW,  PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_IB_OPEN_TXN,  PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_IB_TOTAL_TXN, PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_OB_OPEN_TXN,  PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_OB_TOTAL_TXN, PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_LOCAL_BW,     PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_REMOTE_BW,    PMU_EVENT_INVALID},
  {PMU_NODE_PCIE_RC,  PMU_EVENT_ALL_BW,       PMU_EVENT_INVALID},
  {PMU_NODE_ACPI_DEVICE, PMU_EVENT_TRAFFIC_1, PMU_EVENT_INVALID},
  {PMU_NODE_ACPI_DEVICE, PMU_EVENT_TRAFFIC_2, PMU_EVENT_INVALID}
};

/**
  @brief  This API returns the event ID to be filled into PMEVTYPER register.
          Prerequisite - event_list array. This API should be called after
          filling the required event IDs into event_list array.

  @param  event_type  -  Type of the event.
  @param  node_type   -  PMU Node type

  @return  Event ID

**/
UINT32
pal_pmu_get_event_info(PMU_EVENT_TYPE_e event_type, PMU_NODE_INFO_TYPE node_type)
{
  UINT32 i=0;
  while (event_list[i].node_type != node_type || event_list[i].event_desc != event_type) {
    i++;
  }
  return event_list[i].event_id;
}

/**
  @brief   This API checks if pmu monitor count value is valid
  @param   interface_acpiid - acpiid of interface
  @param   count_value - monitor count value
  @param   eventid - eventid  
  @return  0 - monitor count value is valid
           non-zero - error or invallid count value
**/
UINT32
pal_pmu_check_monitor_count_value(UINT64 interface_acpiid, UINT32 count_value, UINT32 eventid)
{
    return NOT_IMPLEMENTED;
}

/**
  @brief   This API generates required workload for given pmu node and event id
  @param   interface_acpiid - acpiid of interface
  @param   pmu_node_index - pmu node index
  @param   mon_index - monitor index
  @param   eventid - eventid
  @return  0 - success status
           non-zero - error status
**/
UINT32
pal_generate_traffic(UINT64 interface_acpiid, UINT32 pmu_node_index,
                                     UINT32 mon_index, UINT32 eventid)
{
    return NOT_IMPLEMENTED;
}

/**
  @brief   This API checks if PMU node is secure only.
  @param   interface_acpiid - acpiid of interface
  @param   num_traffic_type_support - num of traffic type supported.
  @return  0 - success status
           non-zero - error status
**/
UINT32
pal_pmu_get_multi_traffic_support_interface(UINT64 *interface_acpiid,
                                                       UINT32 *num_traffic_type_support)
{
    return NOT_IMPLEMENTED;
}
