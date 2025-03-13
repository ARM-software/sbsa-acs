/** @file
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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

#include "val/sbsa/include/sbsa_val_interface.h"
#include "val/common/include/acs_val.h"
#include "val/common/include/acs_pe.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_pmu.h"
#include "val/sbsa/include/sbsa_acs_memory.h"
#include "val/sbsa/include/sbsa_acs_mpam.h"
#include "val/common/include/acs_common.h"

#define TEST_NUM  (ACS_PMU_TEST_NUM_BASE + 4)
#define TEST_RULE "PMU_BM_1, PMU_SYS_1, PMU_SYS_2"
#define TEST_DESC "Check memory bandwidth monitors        "

#define BUFFER_SIZE 4194304 /* 4 Megabytes*/
#define NUM_PMU_MON 3       /* Minimum required monitors */

static PMU_EVENT_TYPE_e config_events[NUM_PMU_MON] = {PMU_EVENT_IB_TOTAL_BW,
                                               PMU_EVENT_IB_READ_BW,
                                               PMU_EVENT_IB_WRITE_BW};

/* Generates Inbound read/write traffic at memory interface */
static uint32_t generate_inbound_traffic(uint32_t node_index, uint64_t base_addr, uint32_t size,
                             uint64_t *value)
{
    uint32_t  i;
    void *src_buf = 0;
    void *dest_buf = 0;
    /* Generate inbound traffic for given size*/

    /* Allocate memory for 4 Megabytes */
    src_buf = (void *)val_mem_alloc_at_address(base_addr, BUFFER_SIZE);
    dest_buf = (void *)val_mem_alloc_at_address(base_addr + BUFFER_SIZE, BUFFER_SIZE);

    if ((src_buf == NULL) || (dest_buf == NULL))
        return 1;

    /* Perform memory copy for given size */
    val_memcpy(src_buf, dest_buf, size);

    /* Read the configured monitors for bandwidth values */
    for (i = 0; i < NUM_PMU_MON ; i++) {
        value[i] = val_pmu_read_count(node_index, i);
        val_print(ACS_PRINT_DEBUG, "\n       MON %d", i);
        val_print(ACS_PRINT_DEBUG, " value = %x", value[i]);
    }

    /*Free the buffers */
    val_mem_free_at_address((uint64_t)src_buf, BUFFER_SIZE);
    val_mem_free_at_address((uint64_t)dest_buf, BUFFER_SIZE);

    return 0;
}

static void payload(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t fail_cnt = 0;
    uint32_t node_count;
    uint32_t node_index;
    uint64_t bandwidth1[NUM_PMU_MON];
    uint64_t bandwidth2[NUM_PMU_MON];
    uint32_t status;
    uint64_t mem_range_index;
    uint64_t num_mem_range;
    uint64_t mc_prox_domain;
    uint64_t prox_base_addr, addr_len;
    uint32_t i, cs_com = 0;

    if (g_sbsa_level < 7) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    node_count = val_pmu_get_info(PMU_NODE_COUNT, 0);
    val_print(ACS_PRINT_DEBUG, "\n       PMU NODES = %d", node_count);

    if (node_count == 0) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
        val_print(ACS_PRINT_ERR, "\n       No PMU nodes found", 0);
        return;
    }

    /* The test uses PMU CoreSight arch register map, skip if pmu node is not cs */
    for (node_index = 0; node_index < node_count; node_index++) {
        cs_com |= val_pmu_get_info(PMU_NODE_CS_COM, node_index);
    }
    if (cs_com != 0x1) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
        val_print(ACS_PRINT_DEBUG, "\n       No CS PMU nodes found", 0);
        return;
    }

    /*Get number of memory ranges from SRAT table */
    num_mem_range = val_srat_get_info(SRAT_MEM_NUM_MEM_RANGE, 0);
    if (num_mem_range == 0 || num_mem_range == SRAT_INVALID_INFO) {
        val_print(ACS_PRINT_ERR, "\n       No Proximity domains in the system", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
        return;
    }

    /* Loop through the memory ranges listed on SRAT table */
    for (mem_range_index = 0 ; mem_range_index < num_mem_range ; mem_range_index++) {

        /* Get proximity domain mapped to the memory range */
        mc_prox_domain = val_srat_get_prox_domain(mem_range_index);
        if (mc_prox_domain == SRAT_INVALID_INFO) {
            val_print(ACS_PRINT_ERR, "\n       Proximity domain not found", 0);
            fail_cnt++;
            continue;
        }

        /* Get PMU node index corresponding to the proximity domain */
        node_index = val_pmu_get_node_index(mc_prox_domain);
        if (node_index == PMU_INVALID_INDEX) {
            val_print(ACS_PRINT_ERR,
                    "\n       Proximity domain %d has no PMU associated with it", mc_prox_domain);
            fail_cnt++;
            continue;
        }

        /* Check if the PMU supports atleast 3 counters */
        data = val_pmu_get_monitor_count(node_index);
        if (data < 3) {
            val_print(ACS_PRINT_ERR, "\n       PMU node must support atleast 3 counter", 0);
            fail_cnt++;
            continue;
        }

        /* Get base address of the proximity domain */
        prox_base_addr = val_srat_get_info(SRAT_MEM_BASE_ADDR, mc_prox_domain);
        addr_len = val_srat_get_info(SRAT_MEM_ADDR_LEN, mc_prox_domain);
        if ((prox_base_addr == SRAT_INVALID_INFO) || (addr_len == SRAT_INVALID_INFO) ||
            (addr_len <= 2 * BUFFER_SIZE)) {
            val_print(ACS_PRINT_ERR,
                        "\n       Invalid base address for proximity domain : 0x%lx",
                        mc_prox_domain);
            fail_cnt++;
            continue;
        }

        /* Configure PMEVTYPER to monitor Bandwidth value */
        for (i = 0; i < NUM_PMU_MON; i++) {
            status = val_pmu_configure_monitor(node_index, config_events[i], i);
            if (status) {
                val_print(ACS_PRINT_ERR,
                            "\n       Required PMU Event 0x%x not supported", config_events[i]);
                val_print(ACS_PRINT_ERR, " at node %d", node_index);
                fail_cnt++;
                break;
            }
        }

        if (status)
            continue;

        /* Enable specific monitors */
        for (i = 0; i < NUM_PMU_MON ; i++)
            val_pmu_enable_monitor(node_index, i);

        /* Generate first memory traffic for 2 MB */
        status = generate_inbound_traffic(node_index, prox_base_addr, BUFFER_SIZE / 2, bandwidth1);

        if (status) {
            val_print(ACS_PRINT_ERR, "\n       Memory allocation failed", node_index);
            fail_cnt++;
            continue;
        }

        /* Reset the monitors */
        for (i = 0; i < NUM_PMU_MON ; i++) {
            val_pmu_disable_monitor(node_index, i);
            val_pmu_enable_monitor(node_index, i);
        }

        /* Generate second memory traffic for 4 MB */
        status = generate_inbound_traffic(node_index, prox_base_addr, BUFFER_SIZE, bandwidth2);

        if (status) {
            val_print(ACS_PRINT_ERR, "\n       Memory allocation failed", node_index);
            fail_cnt++;
            continue;
        }

        /* Consider delta to check if counter is moving in proportion */
        for (i = 0; i < NUM_PMU_MON ; i++) {
            if (bandwidth1[i] > bandwidth2[i]) {
                fail_cnt++;
                break;
            }
        }

        /* Disable PMU monitors */
        val_pmu_disable_all_monitors(node_index);
    }

    if (fail_cnt) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
        return;
    }

    val_set_status(index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t pmu004_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1; /* This test is run on a single PE */

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
