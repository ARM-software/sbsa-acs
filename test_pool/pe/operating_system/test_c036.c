/** @file
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (ACS_PE_TEST_NUM_BASE + 36)
#define TEST_RULE  "S_L7PE_09"
#define TEST_DESC  "Check WFE Fine tune delay feature     "

static void payload(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t primary_pe_idx = val_pe_get_primary_index();

    if (g_sbsa_level < 7) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    /* ID_AA64MMFR1_EL1.TWED [35:32] = 0b0001 indicates support for configurable delayed
       trapping for WFE instruction */
    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64MMFR1_EL1), 32, 35);
    if (index == primary_pe_idx)
        val_print(ACS_PRINT_DEBUG, "\n       ID_AA64MMFR1_EL1.TWED = %llx", data);

    if (data == 1)
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
    else {
        if (index == primary_pe_idx)
            val_print(ACS_PRINT_WARN,
                "\n       Recommened WFE fine-tuning delay feature not implemented", 0);
        val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
    }
}

uint32_t c036_entry(uint32_t num_pe)
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
