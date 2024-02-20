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

#include "val/include/sbsa_avs_val.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/val_interface.h"

#define TEST_NUM   (AVS_PE_TEST_NUM_BASE + 33)
#define TEST_RULE  "S_L7PE_06"
#define TEST_DESC  "Check PAuth2, FPAC & FPACCOMBINE  "

static void payload(void)
{
    /* Read ID_AA64ISAR1_EL1 and ID_AA64ISAR2_EL1 for EnhancedPAC2 and FPAC support */
    uint64_t data1 = val_pe_reg_read(ID_AA64ISAR1_EL1);
    uint32_t data2 = val_pe_reg_read(ID_AA64ISAR2_EL1);
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t primary_pe_idx = val_pe_get_primary_index();

    if (g_sbsa_level < 7) {
        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
        return;
    }

    if (index == primary_pe_idx) {
        val_print(AVS_PRINT_DEBUG, "\n       ID_AA64ISAR1_EL1.APA[7:4]    = %llx",
                 VAL_EXTRACT_BITS(data1, 4, 7));
        val_print(AVS_PRINT_DEBUG, "\n       ID_AA64ISAR2_EL1.APA3[15:12] = %llx",
                 VAL_EXTRACT_BITS(data2, 12, 15));
     }

    /* Read ID_AA64ISAR1_EL1.APA[7:4] and ID_AA64ISAR2_EL1.APA3[15:12] == 0b0101 indicates
     * PAuth2, EnhancedPAC2 and FPAC support of standard QARMA3 and QARMA5 algorithms
     */
    if ((VAL_EXTRACT_BITS(data1, 4, 7) == 5) || (VAL_EXTRACT_BITS(data2, 12, 15) == 5))
        val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
    else
        val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
}

uint32_t c033_entry(uint32_t num_pe)
{
    uint32_t status = AVS_STATUS_FAIL;

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
