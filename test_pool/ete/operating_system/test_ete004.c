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
#include "val/sbsa/include/sbsa_acs_ete.h"
#include "val/common/include/acs_timer_support.h"
#include "val/sbsa/include/sbsa_val_interface.h"
#include "val/sbsa/include/sbsa_acs_memory.h"

#define TEST_NUM   (ACS_ETE_TEST_NUM_BASE + 4)
#define TEST_RULE  "ETE_05"
#define TEST_DESC  "Check Trace Same Timestamp Source "

volatile uint64_t trace_buffer_addr;

static void payload(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint64_t dfr0_value = 0;
    uint64_t traced_timestamp_1 = 0;
    uint64_t traced_timestamp_2 = 0;
    uint64_t traced_timestamp_3 = 0;

    if (g_sbsa_level < 8) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    dfr0_value = val_pe_reg_read(ID_AA64DFR0_EL1);

    /* ID_AA64DFR0_EL1.TraceBuffer, bits [47:44] non-zero value indicate FEAT_TRBE support */
    data = VAL_EXTRACT_BITS(dfr0_value, 44, 47);
    if (data == 0) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       FEAT_TRBE not supported", 0, index);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    /* ID_AA64DFR0_EL1.TraceFilt, bits [43:40] non-zero value indicate FEAT_TRF support */
    data = VAL_EXTRACT_BITS(dfr0_value, 40, 43);
    if (data == 0) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       FEAT_TRF not supported", 0, index);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
        return;
    }

    /* ID_AA64DFR0_EL1.ExtTrcBuff, bits [59:56] non-zero value indicate FEAT_TRBE_EXT support */
    data = VAL_EXTRACT_BITS(dfr0_value, 46, 49);
    if (data == 0) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       FEAT_TRBE_EXT not supported", 0, index);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
        return;
    }

    /* Run Test only if FEAT_TRBE & FEAT_TRF is supported, FEAT_TRF is required
     * for accessing TRFCR_* Registers */

    /* Enable Timer */
    ArmWriteCntpCtl((ArmReadCntpCtl() | ARM_ARCH_TIMER_ENABLE) & (~ARM_ARCH_TIMER_IMASK));

    /* Generate Trace when SelfHostedTraceEnabled = TRUE */
    traced_timestamp_1 = val_ete_generate_trace(trace_buffer_addr, SH_TRACE_ENABLE_TRUE);

    val_print_primary_pe(ACS_PRINT_INFO,
                         "\n       traced_timestamp_1 : 0x%llx", traced_timestamp_1, index);

    /* Generate Trace when SelfHostedTraceEnabled = FALSE */
    traced_timestamp_2 = val_ete_generate_trace(trace_buffer_addr, SH_TRACE_ENABLE_FALSE);

    val_print_primary_pe(ACS_PRINT_INFO,
                         "\n       traced_timestamp_2 : 0x%llx", traced_timestamp_2, index);

    /* Generate Trace when SelfHostedTraceEnabled = TRUE */
    traced_timestamp_3 = val_ete_generate_trace(trace_buffer_addr, SH_TRACE_ENABLE_TRUE);

    val_print_primary_pe(ACS_PRINT_INFO,
                         "\n       traced_timestamp_3 : 0x%llx", traced_timestamp_3, index);

    /* Disable Timer */
    ArmWriteCntpCtl((ArmReadCntpCtl() | ARM_ARCH_TIMER_IMASK) & (~ARM_ARCH_TIMER_ENABLE));

    if ((traced_timestamp_1 == ACS_STATUS_FAIL) ||
        (traced_timestamp_2 == ACS_STATUS_FAIL) ||
        (traced_timestamp_3 == ACS_STATUS_FAIL)) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       Trace Generation Failed", 0, index);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 04));
        return;
    }

    if ((traced_timestamp_1 == 0) || (traced_timestamp_2 == 0) || (traced_timestamp_3 == 0)) {
        val_print_primary_pe(ACS_PRINT_ERR, "\n       Traced Timestamp is 0", 0, index);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 05));
        return;
    }

    /* Check Traced Timestamp is increasing */
    if ((traced_timestamp_1 < traced_timestamp_2) && (traced_timestamp_2 < traced_timestamp_3)) {
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
        return;
    }

    val_set_status(index, RESULT_FAIL(TEST_NUM, 06));
}

uint32_t ete004_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP) {
        trace_buffer_addr = (uint64_t)val_aligned_alloc(MEM_ALIGN_4K, num_pe * MEM_ALIGN_4K);

        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

        val_memory_free_aligned((void *)trace_buffer_addr);
    }

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
