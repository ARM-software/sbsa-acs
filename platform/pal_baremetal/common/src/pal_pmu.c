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

#include "platform_override_struct.h"
#include "pal_pmu.h"
#include "pal_common_support.h"

extern PLATFORM_OVERRIDE_PMU_INFO_TABLE platform_pmu_cfg;

/**
  @brief  Display PMU info table details

  @param  PmuTable  - Address to the PMU information table.

  @return  None
**/
void
pal_pmu_dump_info_table(PMU_INFO_TABLE *PmuTable)
{
  uint32_t i;

  if (PmuTable == NULL) {
      return;
  }

  for (i = 0; i < PmuTable->pmu_count; i++) {
      print(AVS_PRINT_INFO, "\nPMU info Index      :%d ", i);
      print(AVS_PRINT_INFO, "\nPMU node type       :%02X ", PmuTable->info[i].type);
      print(AVS_PRINT_INFO, "\nDual page extension :%d ",
                 PmuTable->info[i].dual_page_extension);
      print(AVS_PRINT_INFO, "\nBase Address 0      :%llX ", PmuTable->info[i].base0);
      if(PmuTable->info[i].dual_page_extension)
          print(AVS_PRINT_INFO, "\nBase Address 1      :%llX ", PmuTable->info[i].base1);
      print(AVS_PRINT_INFO, "\nPrimary Instance    :%llX ",
                 PmuTable->info[i].primary_instance);
      print(AVS_PRINT_INFO, "\nSecondary Instance  :%08X ",
                 PmuTable->info[i].secondary_instance);

  }
}

/**
  @brief  This API fills in the PMU_INFO_TABLE with information about local and system
          timers in the system. this employs baremetal platform specific data.

  @param  PmuTable  - Address where the PMU information needs to be filled.

  @return  None
**/
void
pal_pmu_create_info_table(PMU_INFO_TABLE *PmuTable)
{

  uint32_t i;

  /* Check if memory for PMU info table allocated */
  if (PmuTable == NULL) {
      print(AVS_PRINT_ERR, "\n Input PMU Table Pointer is NULL");
      return;
  }

  /* Initialize PMU info table */
  PmuTable->pmu_count = 0;

  /* Populate PMU information */
  for (i = 0; i < platform_pmu_cfg.pmu_count; i++) {
      PmuTable->info[i].type = platform_pmu_cfg.pmu_info[i].type;
      PmuTable->info[i].dual_page_extension = platform_pmu_cfg.pmu_info[i].dual_page_extension;
      PmuTable->info[i].base0 = platform_pmu_cfg.pmu_info[i].base0;
      PmuTable->info[i].base1 = platform_pmu_cfg.pmu_info[i].base1;
      PmuTable->info[i].primary_instance = platform_pmu_cfg.pmu_info[i].primary_instance;
      PmuTable->info[i].secondary_instance = platform_pmu_cfg.pmu_info[i].secondary_instance;

      PmuTable->pmu_count++;
      if (PmuTable->pmu_count >= MAX_NUM_OF_PMU_SUPPORTED) {
          print(AVS_PRINT_WARN, "\n Number of PMUs greater than %d",
                MAX_NUM_OF_PMU_SUPPORTED);
          break;
      }

      /* Dump PMU info table */
      pal_pmu_dump_info_table(PmuTable);
  }
}

typedef struct{
    PMU_NODE_INFO_TYPE node_type;
    PMU_EVENT_TYPE_e event_desc;
    uint32_t event_id;
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
uint32_t
pal_pmu_get_event_info(PMU_EVENT_TYPE_e event_type, PMU_NODE_INFO_TYPE node_type)
{
  uint32_t i=0;
  while (event_list[i].node_type != node_type || event_list[i].event_desc != event_type) {
    i++;
  }
  return event_list[i].event_id;
}
