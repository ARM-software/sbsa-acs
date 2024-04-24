/** @file
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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
#include "val/common/include/acs_common.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_mpam.h"
#include "val/common/include/acs_peripherals.h"

#define TEST_NUM   (ACS_MPAM_TEST_NUM_BASE + 5)
#define TEST_RULE  "S_L7MP_08"
#define TEST_DESC  "Check for MPAM MSC address overlap    "

static void payload(void)
{
    uint32_t msc_node_cnt;
    uint32_t msc_index;
    uint32_t msc_index1;
    uint64_t msc_addr;
    uint32_t msc_len;
    uint64_t msc_addr1;
    uint32_t test_fails = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint64_t peri_count;
    uint64_t peripheral_base;

   if (g_sbsa_level < 7) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    msc_node_cnt = val_mpam_get_msc_count();
    val_print(ACS_PRINT_DEBUG, "\n       MSC count = %d", msc_node_cnt);

    if (msc_node_cnt == 0) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    /* Check MSC memory node size are not overlapping with other MSC */
    for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {
        msc_addr = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
        msc_len = val_mpam_get_info(MPAM_MSC_ADDR_LEN, msc_index, 0);

        for (msc_index1 = msc_index + 1; msc_index1 < msc_node_cnt; msc_index1++) {
            msc_addr1 = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index1, 0);

            if (msc_addr1 >= msc_addr && msc_addr1 <= (msc_addr + msc_len)) {
                val_print(ACS_PRINT_ERR, "\n       MSC %d and", msc_index);
                val_print(ACS_PRINT_ERR, " %d memory layout overlapping", msc_index1);
                test_fails++;
            }
        }

        /* Check with peripherals - USB */
        peri_count = val_peripheral_get_info(NUM_USB, 0);
        while (peri_count) {
            --peri_count;  //array index starts from 0, so subtract 1 from count
            val_print(ACS_PRINT_DEBUG, "\n       USB index %d", peri_count);
            peripheral_base = val_peripheral_get_info(USB_BASE0, peri_count);
            if (peripheral_base >= msc_addr && peripheral_base <= (msc_addr + msc_len)) {
                val_print(ACS_PRINT_ERR, "\n       MSC %d and", msc_index);
                val_print(ACS_PRINT_ERR, " USB %d memory layout overlapping", peri_count);
                test_fails++;
            }
        }


        /* Check with peripherals - UART */
        peri_count = val_peripheral_get_info(NUM_UART, 0);
        while (peri_count) {
            --peri_count;  //array index starts from 0, so subtract 1 from count
            val_print(ACS_PRINT_DEBUG, "\n       UART index %d", peri_count);
            peripheral_base = val_peripheral_get_info(UART_BASE0, peri_count);
            if (peripheral_base >= msc_addr && peripheral_base <= (msc_addr + msc_len)) {
                val_print(ACS_PRINT_ERR, "\n       MSC %d and", msc_index);
                val_print(ACS_PRINT_ERR, " UART %d memory layout overlapping", peri_count);
                test_fails++;
            }
        }

        /* Check with peripherals - SATA */
        peri_count = val_peripheral_get_info(NUM_SATA, 0);
        while (peri_count) {
            --peri_count;  //array index starts from 0, so subtract 1 from count
            val_print(ACS_PRINT_DEBUG, "\n       SATA index %d", peri_count);
            peripheral_base = val_peripheral_get_info(SATA_BASE0, peri_count);
            if (peripheral_base >= msc_addr && peripheral_base <= (msc_addr + msc_len)) {
                val_print(ACS_PRINT_ERR, "\n       MSC %d and", msc_index);
                val_print(ACS_PRINT_ERR, " SATA %d memory layout overlapping", peri_count);
                test_fails++;
            }
        }
    }

    if (test_fails)
        val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
    else
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t mpam005_entry(uint32_t num_pe)
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
