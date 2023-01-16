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
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_mpam.h"


#define TEST_NUM  (AVS_MPAM_TEST_NUM_BASE + 4)
#define TEST_RULE "S_L7MP_07"
#define TEST_DESC "Check for MBWU counter size       "

#define MBWU_COUNTER_44BIT 0
#define MAX_44BIT_COUNTER_BW 1677722 /* this is in MB (1.6 TB) */

static void payload(void)
{
    uint32_t pe_index;
    uint32_t msc_node_cnt, msc_index;
    uint32_t rsrc_node_cnt, rsrc_index;
    uint64_t mbwu_bw;
    uint32_t test_fails = 0;
    uint32_t test_skip = 1;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

    if (g_sbsa_level < 7) {
        val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
        return;
    }
   /* Check if PE implements FEAT_MPAM */
    if (!((VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 40, 43) > 0) ||
        (VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR1_EL1), 16, 19) > 0))) {
            val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
            return;
    }

    /* get total number of MSCs reported by MPAM ACPI table */
    msc_node_cnt = val_mpam_get_msc_count();
    val_print(AVS_PRINT_DEBUG, "\n       MSC count = %d", msc_node_cnt);

    if (!msc_node_cnt) {
        val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
        return;
    }

    /* visit each MSC node and check for memory resources */
    for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {
        rsrc_node_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);

        val_print(AVS_PRINT_DEBUG, "\n       msc index  = %d", msc_index);
        val_print(AVS_PRINT_DEBUG, "\n       Resource count %d = ", rsrc_node_cnt);

        for (rsrc_index = 0; rsrc_index < rsrc_node_cnt; rsrc_index++) {

            /* check whether the resource location is memory */
            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index) ==
                                                                         MPAM_RSRC_TYPE_MEMORY) {

                /* As per S_L7MP_05, MBWU monitoring must be supported for general purpose mem */
                if (!val_mpam_msc_supports_mbwumon(msc_index)) {
                    val_print(AVS_PRINT_ERR, "\n       MBWU MON unsupported by MSC %d", msc_index);
                    test_fails++;
                    break;
                }

                test_skip = 0;

                /* L7MP_07, MBWU counter size must be 44 bit for interface b/w upto 1.6 TB/s
                   else 66 bit.  Check MBWUMON_IDR HAS_LONG[30] and LWD[29] bits. The reg is
                   present only if mbwumon is supported */
                if (!val_mpam_msc_supports_mbwumon(msc_index)) {
                    val_print(AVS_PRINT_ERR, "\n       MBWU MON unsupported by MSC %d", msc_index);
                    test_fails++;
                    break;
                }
                if (!val_mpam_mbwu_supports_long(msc_index)) {
                    val_print(AVS_PRINT_ERR, "\n       MBWU long unsupported MSC %d", msc_index);
                    test_fails++;
                    break;
                }
                mbwu_bw = val_mpam_msc_get_mscbw(msc_index, rsrc_index);
                if (mbwu_bw == HMAT_INVALID_INFO)
                {
                    val_print(AVS_PRINT_ERR, "\n       No HMAT info ", 0);
                    val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
                    return;
                }
                if ((val_mpam_mbwu_supports_lwd(msc_index) == MBWU_COUNTER_44BIT)
                            && (mbwu_bw >= MAX_44BIT_COUNTER_BW)) {
                    val_print(AVS_PRINT_ERR, "\n       MBWU supported b/w %d", mbwu_bw);
                    test_fails++;
                    break;
                }
            }
        }
    }

    if (test_fails)
        val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
    else if (test_skip)
        val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 03));
    else
        val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

    return;
}

uint32_t mpam004_entry(uint32_t num_pe)
{
    uint32_t status = AVS_STATUS_FAIL;

    num_pe = 1;
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
