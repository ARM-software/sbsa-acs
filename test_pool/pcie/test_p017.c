/** @file
 * Copyright (c) 2018, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 17)
#define TEST_DESC  "Root port must implement minimal ACS features if P2P supported"

static void payload(void)
{
    uint32_t index;
    uint32_t count;
    uint32_t valid_cnt;
    uint8_t data;
    uint16_t acs_data;
    uint32_t pcie_type;
    uint64_t dev_bdf;

    index = val_pe_get_index_mpid(val_pe_get_mpid());

    valid_cnt = 0;
    count = val_peripheral_get_info(NUM_ALL, 0);
    if (!count) {
        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 3));
        return;
    }

    while (count > 0) {
        count--;
        dev_bdf = val_peripheral_get_info(ANY_BDF, count);

        /* get the PCIe device/port type */
        pcie_type = val_pcie_get_pcie_type(dev_bdf);
        if (pcie_type == PCIE_TYPE_ROOT_PORT) {
            /* check if root port supports peer to peer */
            if (val_pcie_p2p_support(dev_bdf))
                continue;

            valid_cnt++;
            val_pcie_read_ext_cap_word(dev_bdf, PCI_EXT_CAPID_ACS, PCI_CAPID_ACS, &acs_data);
            if (!acs_data)
                goto test_fail;
            /* Extract ACS source validation bit */
            data = VAL_EXTRACT_BITS(acs_data, 0, 0);
            if (!data) {
                val_print(AVS_PRINT_ERR, "\n       Source validation not supported", 0);
                goto test_fail;
            }
            /* Extract ACS translation blocking bit */
            data = VAL_EXTRACT_BITS(acs_data, 1, 1);
            if (!data) {
                val_print(AVS_PRINT_ERR, "\n       Translation blocking not supported", 0);
                goto test_fail;
            }
            /* Extract ACS P2P request redirect bit */
            data = VAL_EXTRACT_BITS(acs_data, 2, 2);
            if (!data) {
                val_print(AVS_PRINT_ERR, "\n       P2P request redirect not supported", 0);
                goto test_fail;
            }
            /* Extract ACS P2P completion redirect bit */
            data = VAL_EXTRACT_BITS(acs_data, 3, 3);
            if (!data) {
                val_print(AVS_PRINT_ERR, "\n       P2P completion redirect not supported", 0);
                goto test_fail;
            }
            /* Extract ACS upstream forwarding bit */
            data = VAL_EXTRACT_BITS(acs_data, 4, 4);
            if (!data) {
                val_print(AVS_PRINT_ERR, "\n       Upstream forwarding not supported", 0);
                goto test_fail;
            }
        } else {
            continue;
        }
    }

    if (!valid_cnt) {
        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 3));
        return;
    }

    val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
    return;

test_fail:
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
}

uint32_t p017_entry(uint32_t num_pe)
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
