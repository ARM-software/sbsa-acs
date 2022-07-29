/** @file
 * Copyright (c) 2016-2018, 2020-2022 Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_val.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/val_interface.h"

#define TEST_NUM   (AVS_PE_TEST_NUM_BASE  +  22)
#define TEST_DESC  "Check for pointer signing         "

static void check_pointer_signing_algorithm(uint32_t index, uint64_t data)
{
    /* Read ID_AA64ISAR1_EL1[7:4] for pointer signing using standard algorithm
     * defined by Arm architecture
     */
    if (VAL_EXTRACT_BITS(data, 4, 7) == 0)
        val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
    else
        val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}
static void payload(void)
{
    uint64_t data = val_pe_reg_read(ID_AA64ISAR1_EL1);
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    if (g_sbsa_level < 4) {
        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
        return;

    } else if (g_sbsa_level < 5) {

        /* Pointer signing is optional, Check if Pointer signing is implemented */
        if ((VAL_EXTRACT_BITS(data, 4, 7) == 0) && (VAL_EXTRACT_BITS(data, 8, 11) == 0) &&
            (VAL_EXTRACT_BITS(data, 24, 27) == 0) && (VAL_EXTRACT_BITS(data, 28, 31) == 0)) {
            /* Pointer signing not implemented, Skip the test */
            val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
            return;
        }

        /* Implemented, Check for pointer signing using standard algorithm */
        check_pointer_signing_algorithm(index, data);

    } else {
        /* Pointer signing is mandatory, Check for pointer signing using standard algorithm */
        check_pointer_signing_algorithm(index, data);
    }
}

uint32_t c022_entry(uint32_t num_pe)
{
    uint32_t status = AVS_STATUS_FAIL;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
    /* This check is when user is forcing us to skip this test */
    if (status != AVS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe);
    val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

    return status;
}
