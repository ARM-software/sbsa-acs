/** @file
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
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
#include "val/common/include/acs_common.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_mpam.h"


#define TEST_NUM (ACS_MPAM_TEST_NUM_BASE + 2)
#define TEST_RULE "S_L7MP_03, S_L7MP_04"
#define TEST_DESC "Check for MPAM LLC CSU            "

static void payload(void)
{
    uint32_t llc_index;
    uint64_t cache_identifier;
    uint32_t msc_node_cnt;
    uint32_t rsrc_node_cnt;
    uint32_t msc_index, rsrc_index;
    uint32_t csumon_count;
    uint32_t test_fail = 0;
    uint32_t test_run = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

   if (g_sbsa_level < 7) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

   /* Check if PE implements FEAT_MPAM */
    if (!((VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 40, 43) > 0) ||
        (VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR1_EL1), 16, 19) > 0))) {
            val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
            return;
    }

    /* Find the LLC cache identifier */
    llc_index = val_cache_get_llc_index();
    if (llc_index == CACHE_TABLE_EMPTY) {
        val_print(ACS_PRINT_ERR, "\n       Cache info table empty", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    cache_identifier = val_cache_get_info(CACHE_ID, llc_index);
    if (cache_identifier == INVALID_CACHE_INFO) {
        val_print(ACS_PRINT_ERR, "\n       LLC invalid in PPTT", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
        return;
    }


    /* Check in the MPAM table which MSC is attached to the LLC */

    msc_node_cnt = val_mpam_get_msc_count();
    val_print(ACS_PRINT_DEBUG, "\n       MSC count = %d", msc_node_cnt);

    if (msc_node_cnt == 0) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
        return;
    }

    /* visit each MSC node and check for cache resources */
    for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {
        rsrc_node_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);

        val_print(ACS_PRINT_DEBUG, "\n       msc index  = %d", msc_index);
        val_print(ACS_PRINT_DEBUG, "\n       Resource count %d = ", rsrc_node_cnt);

        for (rsrc_index = 0; rsrc_index < rsrc_node_cnt; rsrc_index++) {

            /* check whether the resource location is cache */
            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index) ==
                                                                       MPAM_RSRC_TYPE_PE_CACHE) {
                if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index) ==
                                                                               cache_identifier) {
                    /* We have MSC which controls/monitors the LLC cache */
                    val_print(ACS_PRINT_DEBUG, "\n       rsrc index  = %d", rsrc_index);

                    test_run = 1;
                    /* Check CSU monitor are present */
                    if (!val_mpam_supports_csumon(msc_index)) {
                        val_print(ACS_PRINT_ERR, "\n       CSU MON unsupported by LLC", 0);
                        test_fail = 1;
                    }
                    /* Check min 16 CSU monitor are present */
                    csumon_count = val_mpam_get_csumon_count(msc_index);
                    if (csumon_count < 16) {
                        val_print(ACS_PRINT_ERR, "\n       CSU MON %d less than 16", csumon_count);
                        test_fail = 1;
                    }
                }
            }
        }
    }

    if (!test_run) {
        val_print(ACS_PRINT_ERR, "\n       No LLC MSC found", 0);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 04));
        return;
    }

    if (test_fail)
        val_set_status(index, RESULT_FAIL(TEST_NUM, 05));
    else
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));

}

uint32_t mpam002_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1;
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
