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
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_pmu.h"
#include "val/sbsa/include/sbsa_acs_pcie.h"
#include "val/sbsa/include/sbsa_acs_mpam.h"
#include "val/common/include/acs_common.h"

#define TEST_NUM  (ACS_PMU_TEST_NUM_BASE + 8)
#define TEST_RULE "PMU_SYS_5"
#define TEST_DESC "Check System PMU for NUMA systems      "

#define BUFFER_SIZE 4194304 /* 4 Megabytes*/

#define NUM_PMU_MON 3       /* Minimum required monitors */

static PMU_EVENT_TYPE_e config_events[NUM_PMU_MON] = {PMU_EVENT_LOCAL_BW,
                                               PMU_EVENT_REMOTE_BW,
                                               PMU_EVENT_ALL_BW};

static uint32_t remote_pe_index;
static void *src_buf;
static void *dest_buf;

/* This payload generates remote PE traffic for 2 MB*/
static void payload1(void)
{
    val_memcpy(src_buf, dest_buf, BUFFER_SIZE / 2);

    val_set_status(remote_pe_index, RESULT_PASS(TEST_NUM, 01));
}

/* This payload generates remote PE traffic for 4 MB*/
static void payload2(void)
{
    val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

    val_set_status(remote_pe_index, RESULT_PASS(TEST_NUM, 02));
}

static uint32_t generate_traffic(uint64_t prox_domain, uint32_t size, void (*remote_traffic)(void))
{
    uint64_t prox_base_addr, addr_len;
    uint32_t timeout = TIMEOUT_LARGE;

    prox_base_addr = val_srat_get_info(SRAT_MEM_BASE_ADDR, prox_domain);
    addr_len = val_srat_get_info(SRAT_MEM_ADDR_LEN, prox_domain);
    if ((prox_base_addr == SRAT_INVALID_INFO) || (addr_len == SRAT_INVALID_INFO) ||
        (addr_len <= 2 * BUFFER_SIZE)) {
        val_print(ACS_PRINT_ERR,
                    "\n       Invalid base address for proximity domain : 0x%lx",
                    prox_domain);
        return 1;
    }

    /* Allocate memory for 4 Megabytes */
    src_buf = (void *)val_mem_alloc_at_address(prox_base_addr, BUFFER_SIZE);
    dest_buf = (void *)val_mem_alloc_at_address(prox_base_addr + BUFFER_SIZE, BUFFER_SIZE);

    if ((src_buf == NULL) || (dest_buf == NULL))
        return 1;

    /* Perform memory copy for given size */
    val_memcpy(src_buf, dest_buf, size);

    /* Perform memory copy from remote PE*/
    val_execute_on_pe(remote_pe_index, remote_traffic, 0);

    /* Wait for execution to complete*/
    while (--timeout)
    {
        if (!(IS_RESULT_PENDING(val_get_status(remote_pe_index)))) {
            break;
        }
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
    uint32_t node_count;
    uint32_t mc_node_index;
    uint32_t status;
    uint32_t i;
    uint64_t num_mem_range;
    uint32_t pe_uid, pe_prox_domain;
    uint32_t remote_pe_uid, remote_pe_prox_domain;
    uint64_t value1[NUM_PMU_MON];
    uint64_t value2[NUM_PMU_MON];

    if (g_sbsa_level < 7) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    node_count = val_pmu_get_info(PMU_NODE_COUNT, 0);
    val_print(ACS_PRINT_DEBUG, "\n       PMU NODES = %d", node_count);

    if (node_count == 0) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
        val_print(ACS_PRINT_ERR, "\n       No PMU nodes found", 0);
        return;
    }

    /*Get number of memory ranges from SRAT table */
    num_mem_range = val_srat_get_info(SRAT_MEM_NUM_MEM_RANGE, 0);
    if (num_mem_range == 0 || num_mem_range == SRAT_INVALID_INFO) {
        val_print(ACS_PRINT_ERR, "\n       No Proximity domains in the system", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
        return;
    }

    /*Get local PE details */
    pe_uid = val_pe_get_uid(index);
    pe_prox_domain = val_srat_get_info(SRAT_GICC_PROX_DOMAIN, pe_uid);
    if (pe_prox_domain == SRAT_INVALID_INFO) {
        val_print(ACS_PRINT_ERR, "\n       Could not get proximity domain info for given PE", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 3));
        return;
    }
    /* Get memory controller local to the primary PE */
    mc_node_index = val_pmu_get_node_index(pe_prox_domain);
    if (mc_node_index == PMU_INVALID_INDEX) {
        val_print(ACS_PRINT_ERR, "\n       PMU node not found", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 4));
        return;
    }

    /* Check if the PMU supports atleast 3 counters */
    data = val_pmu_get_monitor_count(mc_node_index);
    if (data < 3) {
        val_print(ACS_PRINT_ERR, "\n       PMU node must support atleast 3 counter", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 5));
        return;
    }

    /* Configure PMEVTYPER to monitor Bandwidth value */
    for (i = 0; i < NUM_PMU_MON; i++) {
        status = val_pmu_configure_monitor(mc_node_index, config_events[i], i);
        if (status) {
            val_print(ACS_PRINT_ERR,
                        "\n       Required PMU Event 0x%x not supported", config_events[i]);
            val_print(ACS_PRINT_ERR, " at node %d", mc_node_index);
            val_set_status(index, RESULT_FAIL(TEST_NUM, 6));
            return;
        }
    }

    /* Enable specific monitors */
    for (i = 0; i < NUM_PMU_MON ; i++)
        val_pmu_enable_monitor(mc_node_index, i);

    /* Get remote PE details */
    remote_pe_prox_domain = val_srat_get_info(SRAT_GICC_REMOTE_PROX_DOMAIN, pe_prox_domain);
    if (remote_pe_prox_domain == SRAT_INVALID_INFO) {
        val_print(ACS_PRINT_ERR, "\n       Could not get remote PE proximity domain", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 7));
        return;
    }
    remote_pe_uid = val_srat_get_info(SRAT_GICC_PROC_UID, remote_pe_prox_domain);
    remote_pe_index = val_pe_get_index_uid(remote_pe_uid);

    /* Generate traffic to the memory controller with varied scale. First with
        size 2 MB and second with 4 MB. Consider delta for results */

    /* Generate remote and local traffic for 2 MB */
    status = generate_traffic(pe_prox_domain, BUFFER_SIZE / 2, payload1);
    if (status) {
        val_print(ACS_PRINT_ERR, "\n       Memory allocation failed", 0);
            val_set_status(index, RESULT_FAIL(TEST_NUM, 8));
            return;
    }

    /* Read the configured monitors for bandwidth values */
    for (i = 0; i < NUM_PMU_MON ; i++)
        value1[i] = val_pmu_read_count(mc_node_index, i);

    /* Reset the monitors */
    for (i = 0; i < NUM_PMU_MON ; i++) {
        val_pmu_disable_monitor(mc_node_index, i);
        val_pmu_enable_monitor(mc_node_index, i);
    }

    /* Generate remote and local traffic for 4 MB */
    status = generate_traffic(pe_prox_domain, BUFFER_SIZE, payload2);

    if (status) {
    val_print(ACS_PRINT_ERR, "\n       Memory allocation failed", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 9));
        return;
    }

    /* Read the configured monitors for bandwidth values */
    for (i = 0; i < NUM_PMU_MON ; i++)
        value2[i] = val_pmu_read_count(mc_node_index, i);

    /*Consider delta for results*/
    for (i = 0 ; i < NUM_PMU_MON ; i++) {
        if (value2[i] <= value1[i]) {
            val_set_status(index, RESULT_FAIL(TEST_NUM, 10));
            return;
        }
    }

    /* Disable PMU monitors */
    val_pmu_disable_all_monitors(mc_node_index);

    val_set_status(index, RESULT_PASS(TEST_NUM, 03));
}

uint32_t
pmu008_entry(uint32_t num_pe)
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
