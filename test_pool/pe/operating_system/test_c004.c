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
#include "val/common/include/acs_peripherals.h"

#define TEST_NUM   (ACS_PE_TEST_NUM_BASE + 4)
#define TEST_RULE  "S_L3PE_04"
#define TEST_DESC  "Check FEAT_LPA Requirements           "

#define FEAT_LPA_IMPL 0x6
/* Retuns true if addr is greater than 48 bits */
#define IS_ADDR_EXCEEDS_48BITS(addr) (addr & (0xfULL << 48))

static void payload(void)
{
    uint64_t data1 = 0, data2 = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint64_t peri_count;
    uint64_t peri_base;

    /* ID_AA64MMFR0_EL1.PARange [3:0] = 0b0110 indicates FEAT_LPA is implemented */
    data1 = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64MMFR0_EL1), 0, 3);
    peri_count = val_peripheral_get_info(NUM_ALL, 0);
    data2 = val_pe_reg_read(ID_AA64MMFR0_EL1);


    if (data1 == FEAT_LPA_IMPL)
    {
        /* The following fields indicates FEAT_LPA2 is implemented:
               ID_AA64MMFR0_EL1.TGran16 [23:20] == 0b0010 and
               ID_AA64MMFR0_EL1.TGran4  [31:28] == 0b0001
         */

        if (VAL_EXTRACT_BITS(data2, 20, 23) == 0x2 || VAL_EXTRACT_BITS(data2, 28, 31) == 0x1)
        {
            val_print(ACS_PRINT_INFO, "\n System supports both FEAT_LPA & FEAT_LPA2 ", 0);
            val_set_status(index, RESULT_PASS(TEST_NUM, 01));
            return;
        }

        /* If the PE implements FEAT_LPA Index through Peripheral info table and
                                                              get base addresses */
        while (peri_count)
        {
            peri_base = val_peripheral_get_info(ANY_BASE0, peri_count - 1);

            /* If the base address is greater than 48 bits it is outside 256TB memory map */
            if (IS_ADDR_EXCEEDS_48BITS(peri_base))
            {
                val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
                return;
            }
            peri_count--;
        }

        val_set_status(index, RESULT_PASS(TEST_NUM, 02));
    }
    else
    {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
    }
}

uint32_t c004_entry(uint32_t num_pe)
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
