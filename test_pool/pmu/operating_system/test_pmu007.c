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
#include "val/common/include/acs_common.h"

#define TEST_NUM  (ACS_PMU_TEST_NUM_BASE + 7)
#define TEST_RULE "PMU_BM_2, PMU_SYS_1, PMU_SYS_2"
#define TEST_DESC "Check PCIe bandwidth monitors     "

#define NUM_TOTAL_PMU_MON 6 /* Minimum required bandwidth monitors */

PMU_EVENT_TYPE_e bandwidth_events[NUM_TOTAL_PMU_MON] = {PMU_EVENT_IB_TOTAL_BW,
                                                     PMU_EVENT_IB_READ_BW,
                                                     PMU_EVENT_IB_WRITE_BW,
                                                     PMU_EVENT_OB_TOTAL_BW,
                                                     PMU_EVENT_OB_READ_BW,
                                                     PMU_EVENT_OB_WRITE_BW};

static void generate_inbound_traffic(uint32_t num_ecam, uint32_t size)
{
    uint32_t ecam_index;
    uint32_t start_bus, seg_num;
    uint32_t dev_index, func_index;
    uint32_t bdf, reg_value;

    for (ecam_index = 0; ecam_index < num_ecam; ecam_index++)
    {
        /* Derive ecam specific information */
        seg_num = val_pcie_get_info(PCIE_INFO_SEGMENT, ecam_index);
        start_bus = val_pcie_get_info(PCIE_INFO_START_BUS, ecam_index);

        /* Iterate over all buses, devices and functions in this ecam */
        for (dev_index = 0; dev_index < size; dev_index++) {
            for (func_index = 0; func_index < PCIE_MAX_FUNC; func_index++) {
                /* Form bdf using seg, bus, device, function numbers */
                bdf = PCIE_CREATE_BDF(seg_num, start_bus, dev_index, func_index);
                val_pcie_read_cfg(bdf, TYPE01_VIDR, &reg_value);
                val_pcie_write_cfg(bdf, TYPE01_VIDR, 0xFFFFFFFF);
            }
        }
    }
}

static void generate_traffic(uint32_t node_index, uint32_t num_ecam, uint32_t size, uint64_t *value)
{
    uint32_t i;

    /* Generate inbound/outbound traffic for given size*/
    generate_inbound_traffic(num_ecam, size);

    /* Read the configured monitors for bandwidth values */
    for (i = 0; i < NUM_TOTAL_PMU_MON ; i++)
        value[i] = val_pmu_read_count(node_index, i);
}

static void payload(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t node_count;
    uint32_t node_index;
    uint64_t bandwidth1[NUM_TOTAL_PMU_MON], bandwidth2[NUM_TOTAL_PMU_MON];
    uint32_t fail_cnt = 0, test_skip = 0;
    uint32_t run_flag = 0;
    uint32_t status;
    uint32_t num_ecam;
    uint32_t i;

    if (g_sbsa_level < 7) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);
    if (num_ecam == 0)
    {
      val_print(ACS_PRINT_ERR, "\n       No ECAMs present              ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
      return;
    }

    node_count = val_pmu_get_info(PMU_NODE_COUNT, 0);
    val_print(ACS_PRINT_DEBUG, "\n       PMU NODES = %d", node_count);

    if (node_count == 0) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
        val_print(ACS_PRINT_ERR, "\n       No PMU nodes found", 0);
        return;
    }

    /* Loop through all the PMU nodes */
    for (node_index = 0; node_index < node_count; node_index++) {

        /* Check the PMU nodes which are associated with PCIe RC */
        if (val_pmu_get_info(PMU_NODE_TYPE, node_index) == PMU_NODE_PCIE_RC) {
            run_flag = 1;

            /* Check if the PMU supports atleast 6 counter */
            data = val_pmu_get_monitor_count(node_index);
            if (data < NUM_TOTAL_PMU_MON) {
                val_print(ACS_PRINT_ERR, "\n       PMU node must support atleast 6 counters", 0);
                fail_cnt++;
                continue;
            }

            /* Configure PMEVTYPER to monitor Bandwidth value */
            for (i = 0; i < NUM_TOTAL_PMU_MON; i++) {
                status = val_pmu_configure_monitor(node_index, bandwidth_events[i], i);
                if (status) {
                    val_print(ACS_PRINT_ERR,
                            "\n       Required PMU Event 0x%x not supported", bandwidth_events[i]);
                    val_print(ACS_PRINT_ERR, " at node %d", node_index);
                    fail_cnt++;
                    break;
                }
            }

            if (status)
                continue;

            /* Enable specific monitors */
            for (i = 0; i < NUM_TOTAL_PMU_MON ; i++)
                val_pmu_enable_monitor(node_index, i);

            /* Generate first pcie traffic */
            generate_traffic(node_index, num_ecam, 10, bandwidth1);

            /* Reset the monitors */
            for (i = 0; i < NUM_TOTAL_PMU_MON ; i++) {
                val_pmu_disable_monitor(node_index, i);
                val_pmu_enable_monitor(node_index, i);
            }

            /* Generate second pcie traffic */
            generate_traffic(node_index, num_ecam, 20, bandwidth2);

            /* Consider delta to check if counter is moving in proportion */
            for (i = 0; i < NUM_TOTAL_PMU_MON ; i++) {
                if (bandwidth1[i] > bandwidth2[i]) {
                    fail_cnt++;
                    break;
                }
            }

            /* Disable PMU monitors */
            val_pmu_disable_all_monitors(node_index);
        }
    }

    if (!run_flag) {
        val_print(ACS_PRINT_ERR, "\n       No PMU associated with PCIe interface", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 04));
        return;
    }

    if (fail_cnt) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 05));
        return;
    } else if (test_skip) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 05));
        return;
    }

    val_set_status(index, RESULT_PASS(TEST_NUM, 06));
}

uint32_t
pmu007_entry(uint32_t num_pe)
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
