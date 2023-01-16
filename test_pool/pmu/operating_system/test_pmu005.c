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

#include "val/include/sbsa_avs_val.h"
#include "val/include/sbsa_avs_common.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_pmu.h"
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_mpam.h"


#define TEST_NUM  (AVS_PMU_TEST_NUM_BASE + 5)
#define TEST_RULE "PMU_MEM_1, PMU_SYS_1, PMU_SYS_2"
#define TEST_DESC "Check memory latency monitors     "

#define BUFFER_SIZE 4194304 /* 4 Megabytes*/

static void payload(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t fail_cnt = 0, test_skip = 1;
    uint32_t node_count;
    uint32_t node_index;
    uint64_t num_open_txn;
    uint64_t num_total_txn;
    uint64_t mem_range_index;
    uint64_t num_mem_range;
    uint64_t mc_prox_domain;
    uint64_t prox_base_addr, addr_len;
    uint32_t status1, status2;
    void *src_buf = 0;
    void *dest_buf = 0;

    if (g_sbsa_level < 7) {
        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
        return;
    }

    node_count = val_pmu_get_info(PMU_NODE_COUNT, 0);
    val_print(AVS_PRINT_DEBUG, "\n       PMU NODES = %d", node_count);

    if (node_count == 0) {
        val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
        val_print(AVS_PRINT_ERR, "\n       No PMU nodes found", 0);
        return;
    }

    /*Get number of memory ranges from SRAT table */
    num_mem_range = val_srat_get_info(SRAT_MEM_NUM_MEM_RANGE, 0);
    if (num_mem_range == 0 || num_mem_range == SRAT_INVALID_INFO) {
        val_print(AVS_PRINT_ERR, "\n       No Proximity domains in the system", 0);
        val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
        return;
    }


 /* Loop through the memory ranges listed on SRAT table */
    for (mem_range_index = 0 ; mem_range_index < num_mem_range ; mem_range_index++) {

        /* Get proximity domain mapped to the memory range */
        mc_prox_domain = val_srat_get_prox_domain(mem_range_index);
        if (mc_prox_domain == SRAT_INVALID_INFO) {
            val_print(AVS_PRINT_ERR, "\n       Proximity domain not found", 0);
            fail_cnt++;
            continue;
        }

        /* Get PMU node index corresponding to the proximity domain */
        node_index = val_pmu_get_node_index(mc_prox_domain);
        if (node_index == PMU_INVALID_INDEX) {
            val_print(AVS_PRINT_ERR,
                    "\n       Proximity domain %d has no PMU associated with it", mc_prox_domain);
            fail_cnt++;
            continue;
        }


        /* Check if the PMU supports atleast 2 counters */
        data = val_pmu_get_monitor_count(node_index);
        if (data < 2) {
            val_print(AVS_PRINT_ERR, "\n       Number of monitors supported = %d", data);
            fail_cnt++;
            continue;
        }

            /* Configure PMEVTYPER to monitor memory latency */
        status1 = val_pmu_configure_monitor(node_index, PMU_EVENT_IB_OPEN_TXN, 0);
        status2 = val_pmu_configure_monitor(node_index, PMU_EVENT_IB_TOTAL_TXN, 1);
        if (status1 || status2) {
            val_print(AVS_PRINT_ERR,
                      "\n       Required events are not supported at node %d", node_index);
            fail_cnt++;
            continue;
        }

        /* Get base address of the proximity domain */
        prox_base_addr = val_srat_get_info(SRAT_MEM_BASE_ADDR, mc_prox_domain);
        addr_len = val_srat_get_info(SRAT_MEM_ADDR_LEN, mc_prox_domain);
        if ((prox_base_addr == SRAT_INVALID_INFO) || (addr_len == SRAT_INVALID_INFO) ||
            (addr_len <= 2 * BUFFER_SIZE)) {
            val_print(AVS_PRINT_ERR,
                        "\n       Invalid base address for proximity domain : 0x%lx",
                        mc_prox_domain);
            fail_cnt++;
            continue;
        }

        //* Allocate memory for 4 Megabytes */
        src_buf = (void *)val_mem_alloc_at_address(prox_base_addr, BUFFER_SIZE);
        dest_buf = (void *)val_mem_alloc_at_address(prox_base_addr + BUFFER_SIZE, BUFFER_SIZE);

        if ((src_buf == NULL) || (dest_buf == NULL)) {
            val_print(AVS_PRINT_ERR, "\n       Memory allocation of buffers failed", 0);
            fail_cnt++;
            continue;
        }


        /* Enable the required counters to count programmed events */
        val_pmu_enable_monitor(node_index, 0);
        val_pmu_enable_monitor(node_index, 1);

        /* Perform read/write and vary scale */
        val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

        /* Read the monitors */
        num_open_txn = val_pmu_read_count(node_index, 0);
        num_total_txn = val_pmu_read_count(node_index, 1);

        /*Free the buffers */
        val_mem_free_at_address((uint64_t)src_buf, BUFFER_SIZE);
        val_mem_free_at_address((uint64_t)dest_buf, BUFFER_SIZE);

        /* Check if counters are moving */
        if (!num_open_txn || !num_total_txn) {
            fail_cnt++;
        }

        /* Disable PMU monitors */
        val_pmu_disable_all_monitors(node_index);
    }

    if (fail_cnt) {
        val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
        return;
    } else if (test_skip) {
        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
        return;
    }

    val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t pmu005_entry(uint32_t num_pe)
{
    uint32_t status = AVS_STATUS_FAIL;

    num_pe = 1; /* This test is run on a single PE */

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level,
                                                                TEST_RULE);
    /* This check is when user is forcing us to skip this test */
    if (status != AVS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

    return status;
}
