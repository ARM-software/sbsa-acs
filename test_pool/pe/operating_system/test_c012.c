/** @file
 * Copyright (c) 2016-2018, 2022-2024, Arm Limited or its affiliates. All rights reserved.
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
#include "val/sbsa/include/sbsa_val_interface.h"

#define TEST_NUM   (ACS_PE_TEST_NUM_BASE  +  12)
#define TEST_RULE  "S_L5PE_05"
#define TEST_DESC  "Check for SHA3 and SHA512 support "

static void payload(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    if (g_sbsa_level < 5) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    /* Read ID_AA64ISAR0_EL1.SHA3[35:32] for cryptography support for SHA3 */
    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64ISAR0_EL1), 32, 35);
    if (data == 0x1)
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
    else {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    /* Read ID_AA64ISAR0_EL1.SHA3[15:12] for cryptography support for SHA512 */
    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64ISAR0_EL1), 12, 15);
    if (data == 0x2)
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
    else
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
}

uint32_t c012_entry(uint32_t num_pe)
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
