/** @file
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM  (ACS_PMU_TEST_NUM_BASE + 9)
#define TEST_RULE "PMU_SYS_6"
#define TEST_DESC "Check multiple type traffic measurement"

#define NUM_TRAFFIC_TYPE 2 /* To atleast check 2 different types of traffic */

static PMU_EVENT_TYPE_e config_events[NUM_TRAFFIC_TYPE] = {PMU_EVENT_TRAFFIC_1,
                                                           PMU_EVENT_TRAFFIC_2};

static void payload(void)
{
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t pmu_node_count, num_mon, i;
    uint32_t mon_index, pmu_node_index;
    uint32_t mon_count_value;
    uint32_t ret_status, status;
    uint64_t interface_acpiid;
    uint32_t num_traffic_support;
    uint32_t cs_com = 0, node_index;

    if (g_sbsa_level < 7) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
        return;
    }

    pmu_node_count = val_pmu_get_info(PMU_NODE_COUNT, 0);
    val_print(ACS_PRINT_DEBUG, "\n       PMU NODES = %d", pmu_node_count);

    if (pmu_node_count == 0) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
        val_print(ACS_PRINT_TEST, "\n       No PMU nodes found in APMT table", 0);
        val_print(ACS_PRINT_TEST, "\n       The test must be considered fail"
                                   " if system has CoreSight PMU", 0);
        val_print(ACS_PRINT_TEST, "\n       For non CoreSight PMU, manually verify A.4 PMU rules "
                                   "in the SBSA specification", 0);
        return;
    }

    /* The test uses PMU CoreSight arch register map, skip if pmu node is not cs */
    for (node_index = 0; node_index < pmu_node_count; node_index++) {
        cs_com |= val_pmu_get_info(PMU_NODE_CS_COM, node_index);
    }
    if (cs_com != 0x1) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 03));
        val_print(ACS_PRINT_TEST, "\n       No CoreSight PMU nodes found", 0);
        val_print(ACS_PRINT_TEST, "\n       For non CoreSight PMU, manually verify A.4 PMU rules "
                                   "in the SBSA specification", 0);
        return;
    }

    /* PAL API to know the interface id which supports multiple types of traffic
       and all the types of traffic supported */
    ret_status = val_pmu_get_multi_traffic_support_interface(&interface_acpiid,
                                                                          &num_traffic_support);
    if (ret_status == NOT_IMPLEMENTED) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 04));
        return;
    }

    /* PMU info table index for the interface */
    pmu_node_index = val_pmu_get_index_acpiid(interface_acpiid);
    if (pmu_node_index == PMU_INVALID_INDEX) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 05));
        return;
    }

    /* Get number of monitor to the interface pmu node */
    num_mon = val_pmu_get_monitor_count(pmu_node_index);
    if (num_mon == 0) {
        val_print(ACS_PRINT_ERR, "\n       PMU node must support atleast 1 counter", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 5));
        return;
    }

    /* Each monitor must be able to measure each supported traffic type */
    for (mon_index = 0; mon_index < num_mon; mon_index++)
    {
        /* Configure PMEVTYPER with required event type */
        for (i = 0; i < NUM_TRAFFIC_TYPE; i++) {

            status = val_pmu_configure_monitor(pmu_node_index, config_events[i], mon_index);
            if (status) {
                val_print(ACS_PRINT_ERR,
                        "\n       Required PMU Event 0x%x not supported", config_events[i]);
                val_print(ACS_PRINT_ERR, " at node %d", pmu_node_index);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 6));
                return;
            }
            val_pmu_enable_monitor(pmu_node_index, mon_index);

            /* generate workload */
            ret_status = val_generate_traffic(interface_acpiid, pmu_node_index, mon_index,
                                                                        config_events[i]);
            if (ret_status) {
                val_print(ACS_PRINT_ERR, "\n       workload generate function failed", 0);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 7));
                return;
            }
            mon_count_value = val_pmu_read_count(pmu_node_index, mon_index);

            /* check if the monitor count value is as expected */
            ret_status = val_pmu_check_monitor_count_value(interface_acpiid, mon_count_value,
                                                                             config_events[i]);
            if (ret_status) {
                val_print(ACS_PRINT_ERR, "\n       count value not as expected", 0);
                val_set_status(index, RESULT_FAIL(TEST_NUM, 8));
                return;
            }

            val_pmu_disable_monitor(pmu_node_index, mon_index);
        }
    }

    /* Disable PMU monitors */
    val_pmu_disable_all_monitors(pmu_node_index);

    val_set_status(index, RESULT_PASS(TEST_NUM, 9));
}

uint32_t
pmu009_entry(uint32_t num_pe)
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
