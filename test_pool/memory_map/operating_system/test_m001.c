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
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_peripherals.h"
#include "val/include/sbsa_avs_memory.h"

#define TEST_NUM (AVS_MEM_MAP_TEST_NUM_BASE + 1)
#define TEST_RULE "S_L3MM_01, S_L3MM_02"
#define TEST_DESC "Check peripherals addr 64Kb apart "

static void payload(void)
{
    uint32_t pe_index;
    uint32_t peri_index, peri_index1;
    uint64_t peri_count, addr_diff;
    uint64_t peri_addr1, peri_addr2;
    uint32_t fail_cnt = 0;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    peri_count = val_peripheral_get_info(NUM_ALL, 0);

    /* check whether all peripheral base addresses are 64KB apart from each other */
    for (peri_index = 0 ; peri_index < peri_count; peri_index++) {
        for (peri_index1 = peri_index + 1; peri_index1 < peri_count; peri_index1++) {

            peri_addr1 = val_peripheral_get_info(ANY_BASE0, peri_index);
            peri_addr2 = val_peripheral_get_info(ANY_BASE0, peri_index1);
            val_print(AVS_PRINT_INFO, "\n   addr of Peripheral 1 is  %llx", peri_addr1);
            val_print(AVS_PRINT_INFO, "\n   addr of Peripheral 2 is  %llx", peri_addr2);

           if ((peri_addr1 == 0) || (peri_addr2 == 0))
                continue;

            addr_diff = (peri_addr1 > peri_addr2) ?
                         peri_addr1 - peri_addr2 : peri_addr2 - peri_addr1;

            if (addr_diff < MEM_SIZE_64KB) {
                val_print(AVS_PRINT_ERR,
                         "\n  Peripheral base addresses isn't atleast 64Kb apart %llx", addr_diff);
                fail_cnt++;
            }
        }
    }

    if (fail_cnt)
    {
        val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
        return;
    }

    val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t m001_entry(uint32_t num_pe)
{
    uint32_t status = AVS_STATUS_FAIL;

    /* run on single PE */
    num_pe = 1;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
    /* This check is when user is forcing us to skip this test */
    if (status != AVS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

    return status;
}
