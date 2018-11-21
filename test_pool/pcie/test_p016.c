/** @file
 * Copyright (c) 2016-2018, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/val_interface.h"

#include "val/include/sbsa_avs_pcie.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 16)
#define TEST_DESC  "NP type-1 pcie only support 32-bit"

#define BAR0    0x10

static void payload(void)
{
    uint32_t index;
    uint32_t count;
    uint32_t status = 0;
    uint32_t ret;
    uint32_t bar_data;
    uint32_t data;
    uint32_t dev_type;
    uint64_t dev_bdf;

    index = val_pe_get_index_mpid(val_pe_get_mpid());
    if (g_sbsa_level < 4) {
        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
        return ;
    }

    count = val_peripheral_get_info (NUM_ALL, 0);
    if (!count) {
        val_set_status(index, RESULT_SKIP (g_sbsa_level, TEST_NUM, 3));
        return;
    }

    while (count > 0) {
        count--;
        dev_bdf = val_peripheral_get_info (ANY_BDF, count);

        dev_type = val_pcie_get_device_type(dev_bdf);
        /* Allow only type-1 headers and skip others */
        if (dev_type != 3)
            continue;
        ret = val_pcie_io_read_cfg(dev_bdf, BAR0, &bar_data);
        if (bar_data) {
            /* Extract pref type */
            data = VAL_EXTRACT_BITS(bar_data, 3, 3);
            if (data == 0) {
                status = 1;
                /* Extract mem type */
                data = VAL_EXTRACT_BITS(bar_data, 1, 2);
                if (data != 0) {
                    val_print(AVS_PRINT_ERR, "\n       NP type-1 pcie is not 32-bit mem type", 0);
                    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
                    status = 2;
                    break;
                }
                /* Scan the all PCIe bridge devices and check memory type */
                ret = val_pcie_scan_bridge_devices_and_check_memtype(dev_bdf);
                if (ret) {
                    val_print(AVS_PRINT_ERR, "\n       NP type-1 pcie bridge end device"
                                                                 "is not 32-bit mem type", 0);
                    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
                    status = 2;
                    break;
                }
            }
        }
    }

    if (!status)
        val_set_status(index, RESULT_SKIP (g_sbsa_level, TEST_NUM, 3));
    else if (status == 1)
        val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t p016_entry(uint32_t num_pe)
{
    uint32_t status = AVS_STATUS_FAIL;

    /* This test is run on single processor */
    num_pe = 1;
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
    if (status != AVS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe);
    val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

    return status;
}
