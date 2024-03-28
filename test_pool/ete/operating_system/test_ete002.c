/** @file
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
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

#include "val/common/include/acs_val.h"
#include "val/common/include/acs_pe.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_val_interface.h"

#define TEST_NUM   (ACS_ETE_TEST_NUM_BASE + 2)
#define TEST_RULE  "ETE_03"
#define TEST_DESC  "Check trace unit ETE supports     "

static void payload(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t resource_selector_pair = 0;
    uint32_t test_fail = 0;
    uint64_t reg_trcidr;

    if (g_sbsa_level < 8) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    /* ID_AA64DFR0_EL1.TraceVer, bits [7:4] non-zero value indicate FEAT_ETE support */
    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64DFR0_EL1), 4, 7);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       ID_AA64DFR0_EL1.TraceVer = %llx",
                                                                             data, index);

    if (data == 0) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    /* The trace unit must support following 9 ETE features
     * Check 1: Cycle counting with a cycle counter that is at least 12-bits in size
     * TRCIDR0.TRCCCI[7] == 1 indicates atleast 12 bit cycle counting implemented for trace unit */

    reg_trcidr = val_pe_reg_read(TRCIDR0);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       TRCIDR0 = %llx", reg_trcidr, index);

    data = VAL_EXTRACT_BITS(reg_trcidr, 7, 7);
    if (data != 1) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       Cycle counting not implemented", 0, index);
        test_fail++;
    }

    /* Check 2: At least one address comparator pair
     * TRCIDR4.NUMACPAIRS, bits [3:0] non-zero indicates at least one address comparator pair */

    reg_trcidr = val_pe_reg_read(TRCIDR4);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       TRCIDR4 = %llx", reg_trcidr, index);

    data = VAL_EXTRACT_BITS(reg_trcidr, 0, 3);
    if (data == 0) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       Address comparator pair not present",
                             0, index);
        test_fail++;
    }

    /* Check 3: At least one Context ID comparator */
    /* TRCIDR4.NUMCIDC, bits [27:24] non-zero value indicates atleast one Context ID comparator */

    data = VAL_EXTRACT_BITS(reg_trcidr, 24, 27);
    if (data == 0) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       Context ID comparator not present",
                             0, index);
        test_fail++;
    }

    /* Check 4: At least one Virtual context identifier comparator, if the PE implements EL2
     * If data value is non - zero then pe implements EL2 */
    data = val_is_el2_enabled();

    if (data != 0) {
        /* TRCIDR4.NUMVMIDC, bits [31:28] non-zero value indicates atleast one Virtual Context ID */
        data = VAL_EXTRACT_BITS(reg_trcidr, 28, 31);
        if (data == 0) {
            val_print_primary_pe(ACS_PRINT_ERR, "\n       Virtual Context ID not present",
                                 0, index);
            test_fail++;
        }
    }

    /* Check 5: At least one single-shot comparator control.
     * TRCIDR4.NUMSSCC, bits [23:20] non-zero value indicates atleast one single-shot comparator */

    data = VAL_EXTRACT_BITS(reg_trcidr, 20, 23);
    if (data == 0) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       one single-shot comparator ctrl not present",
                             0, index);
        test_fail++;
    }

    /* Check 6: At least one event in the trace.
     * TRCIDR4.NUMRSPAIR, bits [19:16]  non-zero indicates atleast one ETE event implemented
     * (Non-zero value + 1) Indicates number of resource selection pairs for trace*/
    data = VAL_EXTRACT_BITS(reg_trcidr, 16, 19);

    if (data == 0) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       ETE Event not present in trace", 0, index);
        test_fail++;
    }
    resource_selector_pair = data;

    /* Check 7: At least two counters.
     * TRCIDR5.NUMCNTR, bits [30:28] indicates no of Counters for trace */

    reg_trcidr = val_pe_reg_read(TRCIDR5);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       TRCIDR5 = %llx", reg_trcidr, index);

    data = VAL_EXTRACT_BITS(reg_trcidr, 28, 30);
    if (data < 2) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       Counters Expected >= 2 Actual = %d",
                             data, index);
        test_fail++;
    }

    /* Check 8: The sequencer state machine
     * TRCIDR5.NUMSEQSTATE, bits [27:25] non zero value indicates sequencer is implemented */

    data = VAL_EXTRACT_BITS(reg_trcidr, 25, 27);
    if (data == 0) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       Sequencer not Implemented", 0, index);
        test_fail++;
    }

    /* Check 9: At least four resource selection pairs */
    if (resource_selector_pair < 3) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       Selection Pair Expected > 3 Actual = %d",
                             resource_selector_pair + 1, index);
        test_fail++;
    }

    if (test_fail)
        val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
     else
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t ete002_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}

