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
#include "val/common/include/val_interface.h"
#include "val/sbsa/include/sbsa_acs_ete.h"
#include "val/common/include/acs_memory.h"
#include "val/sbsa/include/sbsa_acs_memory.h"
#include "val/common/include/acs_timer_support.h"

#define TEST_NUM   (ACS_ETE_TEST_NUM_BASE + 3)
#define TEST_RULE  "ETE_04, ETE_06"
#define TEST_DESC  "Check ETE Trace Timestamp Source      "

volatile uint64_t buffer_addr;
volatile uint64_t *start_timestamp;
volatile uint64_t *end_timestamp;
volatile uint32_t test_fail;

void check_timestamp(uint32_t num_pe)
{
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t curr = 0;
    uint32_t pe_timestamp_failed = 0;
    uint32_t check_failed = 0;

    if (test_fail)
        return;

    for (curr = 0; curr < num_pe; curr++)
    {
        pe_timestamp_failed = 0;

        /* Check 1 : For every PE start_timestamp < end_timestamp */
        if (start_timestamp[curr] >= end_timestamp[curr]) {
            /* Fail the test */
            pe_timestamp_failed = 1;
            check_failed = 1;
            val_print(ACS_PRINT_ERR, "\n       Timestamp Fail for PE Index : %d", curr);
        }

        if (curr == index)
            continue;

        /* Check 2 : Since Primary PE Trace will be generated first *
         * timestamp of primary pe will be the least */
        if ((start_timestamp[curr] <= start_timestamp[index]) ||
            (end_timestamp[curr] <= end_timestamp[index]))
        {
            /* Fail the test and continue */
            pe_timestamp_failed = 1;
            val_print(ACS_PRINT_ERR, "\n       Timestamp Mismatch for PE : 0x%x", index);
            val_print(ACS_PRINT_ERR, " and 0x%x", curr);
        }

        /* Debug Prints in case of failures */
        if (pe_timestamp_failed) {
            check_failed = 1;
            val_print(ACS_PRINT_INFO, "\n       start_timestamp : %llx", start_timestamp[curr]);
            val_print(ACS_PRINT_INFO, ",  end_timestamp : %llx", end_timestamp[curr]);
        }
    }

    if (check_failed) {
        val_print(ACS_PRINT_INFO,
                  "\n       Primary PE start_timestamp : 0x%llx", start_timestamp[index]);
        val_print(ACS_PRINT_INFO,
                  "\n       Primary PE end_timestamp   : 0x%llx", end_timestamp[index]);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 04));
    } else
        val_set_status(index, RESULT_PASS(TEST_NUM, 02));
}

static void payload(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint64_t dfr0_value = 0;
    uint64_t traced_timestamp = 0;

    if (g_sbsa_level < 8) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    dfr0_value = val_pe_reg_read(ID_AA64DFR0_EL1);

    /* ID_AA64DFR0_EL1.TraceBuffer, bits [47:44] non-zero value indicate FEAT_TRBE support */
    data = VAL_EXTRACT_BITS(dfr0_value, 44, 47);
    if (data == 0) {
        test_fail = 1;
        val_print_primary_pe(ACS_PRINT_ERR, "\n       FEAT_TRBE not supported", 0, index);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    /* ID_AA64DFR0_EL1.TraceFilt, bits [43:40] non-zero value indicate FEAT_TRF support */
    data = VAL_EXTRACT_BITS(dfr0_value, 40, 43);
    if (data == 0) {
        test_fail = 1;
        val_print_primary_pe(ACS_PRINT_ERR, "\n       FEAT_TRF not supported", 0, index);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
        return;
    }

    /* Run Test only if FEAT_TRBE & FEAT_TRF is supported, FEAT_TRF is required
     * for accessing TRFCR_* Registers */

    /* Enable Timer */
    ArmWriteCntpCtl((ArmReadCntpCtl() | ARM_ARCH_TIMER_ENABLE) & (~ARM_ARCH_TIMER_IMASK));

    /* Store Start Timestamp values which will be used later */
    start_timestamp[index] = ArmReadCntPct();
    val_data_cache_ops_by_va((addr_t)(start_timestamp + index), CLEAN_AND_INVALIDATE);
    val_print_primary_pe(ACS_PRINT_INFO,
                         "\n       Start Timestamp : 0x%llx", start_timestamp[index], index);

    /* Generate Trace when SelfHostedTraceEnabled = TRUE */
    traced_timestamp = val_ete_generate_trace(buffer_addr, SH_TRACE_ENABLE_TRUE);
    val_print_primary_pe(ACS_PRINT_INFO,
                         "\n       Traced Timestamp   : 0x%llx", traced_timestamp, index);


    /* Disable Timer */
    ArmWriteCntpCtl((ArmReadCntpCtl() | ARM_ARCH_TIMER_IMASK) & (~ARM_ARCH_TIMER_ENABLE));

    /* Read Current Counter Value */
    end_timestamp[index] = ArmReadCntPct();
    val_data_cache_ops_by_va((addr_t)(end_timestamp + index), CLEAN_AND_INVALIDATE);
    val_print_primary_pe(ACS_PRINT_INFO,
                         "\n       End Timestamp   : 0x%llx", end_timestamp[index], index);

    /* If Trace is not generated or timestamp for current PE not updated */
    if (traced_timestamp == ACS_STATUS_FAIL) {
        test_fail = 1;
        val_data_cache_ops_by_va((addr_t)(&test_fail), CLEAN_AND_INVALIDATE);
        val_print_primary_pe(ACS_PRINT_ERR, "\n       Trace Generation Failed", 0, index);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
        return;
    }

    if (traced_timestamp == 0) {
        /* Timestamp traced 0 or parsing failed*/
        test_fail = 1;
        val_data_cache_ops_by_va((addr_t)(&test_fail), CLEAN_AND_INVALIDATE);
        val_print_primary_pe(ACS_PRINT_ERR, "\n       Traced Timestamp is 0", 0, index);
        val_set_status(index, RESULT_FAIL(TEST_NUM, 05));
        return;
    }

    if ((start_timestamp[index] <= traced_timestamp) &&
        (traced_timestamp <= end_timestamp[index])) {
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
        return;
    }
    val_set_status(index, RESULT_FAIL(TEST_NUM, 05));
}

uint32_t ete003_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP) {
        buffer_addr     = (uint64_t)val_aligned_alloc(MEM_ALIGN_4K, num_pe * MEM_ALIGN_4K);
        start_timestamp = (uint64_t *)val_aligned_alloc(MEM_ALIGN_4K, num_pe * sizeof(uint64_t));
        end_timestamp   = (uint64_t *)val_aligned_alloc(MEM_ALIGN_4K, num_pe * sizeof(uint64_t));

        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

        /* Compare timestamps across PE's */
        if (test_fail != 1)
            check_timestamp(num_pe);

        val_memory_free_aligned((void *)buffer_addr);
        val_memory_free_aligned((void *)start_timestamp);
        val_memory_free_aligned((void *)end_timestamp);
    }

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
