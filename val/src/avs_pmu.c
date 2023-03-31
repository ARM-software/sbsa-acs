/** @file
 * Copyright (c) 2022-2023, Arm Limited or its affiliates. All rights reserved.
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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_common.h"
#include "include/sbsa_avs_pmu.h"
#include "include/sbsa_avs_pmu_reg.h"

PMU_INFO_TABLE  *g_pmu_info_table;

/**
  @brief   This API executes all the PMU tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_pmu_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pmu_execute_tests(uint32_t level, uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;
  uint32_t i, pmu_node_count;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == AVS_PMU_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "      USER Override - Skipping all PMU tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  if (g_single_module != SINGLE_MODULE_SENTINEL && g_single_module != AVS_PMU_TEST_NUM_BASE &&
    (g_single_test == SINGLE_MODULE_SENTINEL ||
      (g_single_test - AVS_PMU_TEST_NUM_BASE > 100 ||
        g_single_test - AVS_PMU_TEST_NUM_BASE < 0))) {
      val_print(AVS_PRINT_TEST, " USER Override - Skipping all PMU tests \n", 0);
      val_print(AVS_PRINT_TEST, " (Running only a single module)\n", 0);
      return AVS_STATUS_SKIP;
  }

  /* check if PE supports PMU extension, else skip all PMU tests */
  if (val_pe_feat_check(PE_FEAT_PMU)) {
      val_print(AVS_PRINT_TEST,
                "\n       PE PMU extension unimplemented. Skipping all PMU tests\n", 0);
      return AVS_STATUS_SKIP;
  }

  g_curr_module = 1 << PMU_MODULE;

  /* run tests which don't check PMU nodes */
  if (g_sbsa_level > 6) {
      status  = pmu001_entry(num_pe);
      status |= pmu002_entry(num_pe);
      status |= pmu003_entry(num_pe);
      status |= pmu006_entry(num_pe);
  }

  pmu_node_count = val_pmu_get_info(PMU_NODE_COUNT, 0);
  if (pmu_node_count == 0) {
      val_print(AVS_PRINT_TEST,
                "\n       PMU nodes not found. Skipping remaining PMU tests\n", 0);
      return AVS_STATUS_SKIP;
  }

  if (g_sbsa_level > 6) {
      status |= pmu004_entry(num_pe);
      status |= pmu005_entry(num_pe);
      status |= pmu007_entry(num_pe);
      status |= pmu008_entry(num_pe);
      status |= pmu009_entry(num_pe);
  }

  val_print_test_end(status, "PMU");

  return status;
}

/**
  @brief   This API will call PAL layer to fill in the PMU information
           into the address pointed by g_pmu_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   pmu_info_table  pre-allocated memory pointer for PMU info
  @return  Error if Input param is NULL
**/
void
val_pmu_create_info_table(uint64_t *pmu_info_table)
{
  if (pmu_info_table == NULL) {
    val_print(AVS_PRINT_ERR, "\nInput for Create PMU Info table cannot be NULL", 0);
    return;
  }

  g_pmu_info_table = (PMU_INFO_TABLE *)pmu_info_table;

  pal_pmu_create_info_table(g_pmu_info_table);

  val_print(AVS_PRINT_TEST, " PMU_INFO: Number of PMU units        : %4d\n",
            g_pmu_info_table->pmu_count);

  return;
}

/**
  @brief  Free the memory allocated for the PMU information table
**/
void
val_pmu_free_info_table()
{
  pal_mem_free((void *)g_pmu_info_table);
  g_pmu_info_table = NULL;
}

/**
  @brief   This API returns requested PMU node info.

  @param   type       - the type of information being requested.
  @param   node_index  - index of the node in the APMT info table..

  @return  requested data if found, otherwise PMU_INVALID_INFO.
**/

uint64_t
val_pmu_get_info(PMU_INFO_e type, uint32_t node_index)
{
  PMU_INFO_BLOCK *entry;

  if (g_pmu_info_table == NULL) {
      val_print(AVS_PRINT_WARN, "\n   APMT info table not found", 0);
      return 0;
  }

  if (node_index > g_pmu_info_table->pmu_count) {
      val_print(AVS_PRINT_WARN, "\n   Invalid Node index ", 0);
      return 0;
  }

  entry = &g_pmu_info_table->info[node_index];

  switch (type) {
  case PMU_NODE_TYPE:
      return entry->type;
  case PMU_NODE_BASE0:
      return entry->base0;
  case PMU_NODE_BASE1:
      return entry->base1;
  case PMU_NODE_PRI_INST:
      return entry->primary_instance;
  case PMU_NODE_SEC_INST:
      return entry->secondary_instance;
  case PMU_NODE_COUNT:
      return g_pmu_info_table->pmu_count;
  case PMU_NODE_DP_EXTN:
      return entry->dual_page_extension;
  default:
      val_print(AVS_PRINT_ERR, "\n   This PMU info option is not supported : %d ", type);
      return 0;
  }
}

/**
  @brief   This API checks if the PMU implements dedicated cycle counter.
  @param   node_index - index of the PMU node in the APMT info table.
  @return  1 if supported 0 otherwise.
**/
uint8_t
val_pmu_supports_dedicated_cycle_counter(uint32_t node_index)
{
    addr_t base;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    return BITFIELD_READ(PMCFGR_CC, val_mmio_read(base + REG_PMCFGR));
}

/**
  @brief   This API gets the number of monitors supported by the PMU node.
  @param   node_index - index of the PMU node in the APMT info table.
  @return  monitor count.
**/
uint32_t
val_pmu_get_monitor_count(uint32_t node_index)
{
    addr_t base;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

   /* If PMCFGR.CC == 1 PMEVCNTR31 is implemented and is a dedicated cycle counter
      else If PMEVCNTR31 is implemented, it is a normal monitor */
    if (val_pmu_supports_dedicated_cycle_counter(node_index))
        return BITFIELD_READ(PMCFGR_N, val_mmio_read(base + REG_PMCFGR));
    else
        return BITFIELD_READ(PMCFGR_N, val_mmio_read(base + REG_PMCFGR)) + 1;
}

/**
  @brief   This API disbles all PMU monitors.
  @param   node_index - index of the PMU node in the APMT info table.
  @return  none.
**/
void
val_pmu_disable_all_monitors(uint32_t node_index)
{
    addr_t base;
    uint32_t data;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    data = BITFIELD_WRITE(val_mmio_read(base + REG_PMCR), PMCR_E, 0);

    val_mmio_write(base + REG_PMCR, data);
}

/**
  @brief   This API enable all PMU monitors count. Monitors are enabled by PMCNTENSET
  @param   node_index - index of the PMU node in the APMT info table.
  @return  none.
**/
void
val_pmu_enable_all_monitors(uint32_t node_index)
{
    addr_t base;
    uint32_t data;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    data = BITFIELD_WRITE(val_mmio_read(base + REG_PMCR), PMCR_E, 1);

    val_mmio_write(base + REG_PMCR, data);
}

/**
  @brief   This API resets all PMU monitors.
  @param   node_index - index of the PMU node in the APMT info table.
  @return  none.
**/
void
val_pmu_reset_all_monitors(uint32_t node_index)
{
    addr_t base;
    uint32_t data;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    data = BITFIELD_WRITE(val_mmio_read(base + REG_PMCR), PMCR_P, 1);

    val_mmio_write(base + REG_PMCR, data);
}

/**
  @brief   This API returns the number of monitor groups implemented by the PMU node.
  @param   node_index - index of the PMU node in the APMT info table.
  @return  monitor group count.
**/
uint32_t
val_pmu_get_monitor_group_count(uint32_t node_index)
{
    addr_t base;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    /* PMCFGR.NCG gives number of monitor groups implemented minus one */
    return BITFIELD_READ(PMCFGR_NCG, val_mmio_read(base + REG_PMCFGR)) + 1;
}

/**
  @brief   This API returns the size of the largest implemented monitor.
  @param   node_index - index of the PMU node in the APMT info table.
  @return  The size of the largest implemented monitor.
**/
uint32_t
val_pmu_get_max_monitor_size(uint32_t node_index)
{
    addr_t base;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    return BITFIELD_READ(PMCFGR_SIZE, val_mmio_read(base + REG_PMCFGR));
}

/**
  @brief   This API Configures the requested monitor instance.
  @param   node_index - index of the PMU node in the APMT info table.
  @param   event_type - PMU event type.
  @param   mon_inst - instance of the monitor to enable.
  @return  status   1 - Configure FAIL due to unsupported event
                    0 - PASS
**/
uint32_t
val_pmu_configure_monitor(uint32_t node_index, PMU_EVENT_TYPE_e event_type, uint32_t mon_inst)
{
    addr_t base;
    uint32_t offset;
    uint32_t data;
    uint32_t node_type;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    /* Calculate the register offset based on selected monitor */
    offset = 4 * mon_inst;

    /* Disable all monitors before configuring the monitor */
    val_pmu_disable_all_monitors(node_index);

    node_type = val_pmu_get_info(PMU_NODE_TYPE, node_index);
    /* Get event id details based on Implementation */
    data = pal_pmu_get_event_info(event_type, node_type);

    if (data == PMU_EVENT_INVALID) {
        return 1;
    }

    /* Write the received details into PMEVTYPER */
    val_mmio_write(base + offset + REG_PMEVTYPER, data);

    /* Enable all the monitors  using PMCR.E */
    val_pmu_enable_all_monitors(node_index);

    /* Reset the all monitor counts before enabling required monitor */
    val_pmu_reset_all_monitors(node_index);

    return 0;
}

/**
  @brief   This API Enables the requested monitor instance.
  @param   node_index - index of the PMU node in the APMT info table.
  @param   mon_inst - instance of the monitor to enable.
  @return  The size of the largest implemented monitor.
**/
void
val_pmu_enable_monitor(uint32_t node_index, uint32_t mon_inst)
{
    addr_t base;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    uint32_t reg_offset, bit_offset;

    /* Register offset and bit offset is calculated based on the selected
    monitor */
    reg_offset = mon_inst / 32 * 4;
    bit_offset = mon_inst % 32;

    val_mmio_write(base + reg_offset + REG_PMCNTENSET, 1 << bit_offset);
}

/**
  @brief   This API disables the requested monitor instance.
  @param   node_index - index of the PMU node in the APMT info table.
  @param   mon_inst - instance of the monitor to disable.
  @return  None.
**/
void
val_pmu_disable_monitor(uint32_t node_index, uint32_t mon_inst)
{
    addr_t base;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    uint32_t reg_offset, bit_offset;

    /* Register offset and bit offset is calculated based on the selected
    monitor */
    reg_offset = mon_inst / 32 * 4;
    bit_offset = mon_inst % 32;

    val_mmio_write(base + reg_offset + REG_PMCNTENCLR, 1 << bit_offset);

    /* Reset all the monitors */
    val_pmu_reset_all_monitors(node_index);
}

/**
  @brief   This API reads PMU counter value.
  @param   node_index - index of the PMU node in the APMT info table.
  @param   mon_inst - instance of the monitor to read.
  @return  Counter value.
**/
uint64_t
val_pmu_read_count(uint32_t node_index, uint32_t mon_inst)
{
    addr_t base;
    uint64_t count;
    uint32_t offset;

    /* PMEVCNTR is a page 1 register when dual page extension is implemented */
    if (val_pmu_get_info(PMU_NODE_DP_EXTN, node_index))
        base = val_pmu_get_info(PMU_NODE_BASE1, node_index);
    else
        base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    /* The width of the register PMEVCNTR depends on the size of the largest
    implemented counter. if size if PMCFGR.SIZE <= 0b011111, all monitors are
    32 bits or smaller else at least one monitor is larger than 32 bits. The
    offset is then calculated accordingly. */

    if (val_pmu_get_max_monitor_size(node_index) > 0b011111) {
        offset = 8 * mon_inst;
        count = (uint64_t)val_mmio_read(base + offset + REG_PMEVCNTR_H) << 32 |
                val_mmio_read(base + offset + REG_PMEVCNTR_L);
    }
    else {
        offset = 4 * mon_inst;
        count = val_mmio_read(base + offset + REG_PMEVCNTR);
    }

    return count;
}

/**
 @brief   This API returns PMU node index for given proximity domain
 @param   prox_domain Proximity domain from SRAT ACPI table.
 @return  PMU node index from APMT ACPI table.
**/
uint32_t
val_pmu_get_node_index(uint64_t prox_domain)
{
    uint32_t node_index;
    PMU_INFO_BLOCK *entry;

    for (node_index = 0 ; node_index < g_pmu_info_table->pmu_count ; node_index++)
    {
        entry = &g_pmu_info_table->info[node_index];
        if (entry->type == PMU_NODE_MEM_CNTR) {
            if (prox_domain == entry->primary_instance) {
                return node_index;
            }
        }
    }
    val_print(AVS_PRINT_DEBUG, "\n   PMU node for given proximity domain not found ", 0);
    return PMU_INVALID_INDEX;
}

/* the below API are needed for test update to Coresight PMU EAC release specification,
   The tests are currently based on beta release of spec and below API are not called */
/**
  @brief   This API checks if PMU node implements PMSCR register.
  @param   node_index - index of the PMU node in the APMT info table.
  @return  1 - PMSCR is present.
           0 - PMSCR is absent.
**/
uint32_t
val_pmu_implements_pmscr(uint32_t node_index)
{
    addr_t base;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    return BITFIELD_READ(PMSCR_IMPL, val_mmio_read(base + REG_PMSCR_L));
}

/**
  @brief   This API checks if PMU node is secure only.
  @param   node_index - index of the PMU node in the APMT info table.
  @return  0 - Non-secure and Realm Register Access is disabled
           1 - Non-secure and Realm Register Access is enabled
**/
uint32_t
val_pmu_is_secure(uint32_t node_index)
{
    addr_t base;

    base = val_pmu_get_info(PMU_NODE_BASE0, node_index);

    if (val_pmu_implements_pmscr(node_index)) {
        return BITFIELD_READ(PMSCR_NSRA, val_mmio_read(base + REG_PMSCR_L));
    }

    return 0;
}

/**
  @brief   This API checks if pmu monitor count value is valid
  @param   interface_acpiid - acpiid of interface
  @param   count_value - monitor count value
  @param   eventid - eventid
  @return  0 - monitor count value is valid
           non-zero - error or invallid count value
**/
uint32_t
val_pmu_check_monitor_count_value(uint64_t interface_acpiid, uint32_t count_value,
                                                                       uint32_t eventid)
{
    return pal_pmu_check_monitor_count_value(interface_acpiid, count_value, eventid);
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
uint32_t
val_generate_traffic(uint64_t interface_acpiid, uint32_t pmu_node_index,
                                     uint32_t mon_index, uint32_t eventid)
{
    return pal_generate_traffic(interface_acpiid, pmu_node_index, mon_index, eventid);
}
/**
  @brief   This API generates required workload for given pmu node and event id
  @param   interface_acpiid - acpiid of interface
  @param   index_id - index id of interface
  @return  0 - success status
           non-zero - error status
**/
uint32_t
val_pmu_get_index_acpiid(uint64_t interface_acpiid)
{
    uint32_t node_index;
    PMU_INFO_BLOCK *entry;

    for (node_index = 0 ; node_index < g_pmu_info_table->pmu_count ; node_index++)
    {
        entry = &g_pmu_info_table->info[node_index];
        if ((entry->type == PMU_NODE_ACPI_DEVICE) &&
                                                (entry->primary_instance == interface_acpiid)) {
            return node_index;
        }
    }
    val_print(AVS_PRINT_DEBUG, "\n   PMU node for given acpi id not found ", 0);
    return PMU_INVALID_INDEX;

}
/**
  @brief   This API checks if PMU node is secure only.
  @param   interface_acpiid - acpiid of interface
  @param   num_traffic_type_support - num of traffic type supported.
  @return  0 - success status
           non-zero - error status
**/
uint32_t
val_pmu_get_multi_traffic_support_interface(uint64_t *interface_acpiid,
                                                       uint32_t *num_traffic_type_support)
{
    return pal_pmu_get_multi_traffic_support_interface(interface_acpiid, num_traffic_type_support);
}
