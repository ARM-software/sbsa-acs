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

#define TEST_NUM   (ACS_PE_TEST_NUM_BASE + 42)
#define TEST_RULE  "S_L8PE_07"
#define TEST_DESC  "Check for unsupported PBHA bits       "

static void payload(void)
{
    uint64_t el, e2h, data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    if (g_sbsa_level < 8) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    /* ID_AA64MMFR1_EL1.HPDS[12:15] == 2 indicates FEAT_HPDS2 support
     * FEAT_HPDS2 support indicates VMSAv8-64 block and page descriptors
     * bits[62:59] can individually enabled as PBHA bits for both Stage-1 and Stage-2
     */
    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64MMFR1_EL1), 12, 15);
    val_print_primary_pe(ACS_PRINT_INFO, "\n       ID_AA64MMFR1_EL1.HPDS = %llx",
                                                                            data, index);

    /* If FEAT_HPDS2 is not supported then PBHA bits cannot be enabled */
    if (data != 2) {
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
        return;
    } else {
        /* Read CurrentEL which indicates the current exception level */
        el = val_pe_reg_read(CurrentEL);

        if (el != AARCH64_EL1 && el != AARCH64_EL2) {
            val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
            val_print_primary_pe(ACS_PRINT_DEBUG, "\n     Current EL = %llx", el, index);
            return;
        }

        /* If current exception level is EL2 check HCR_EL2.E2H[34] == 1 indicates Lower and Upper
         * VA range control using TCR_EL2 for EL2 else TCR_EL2 for EL2 controls single VA range
         * TCR_EL1 always controls both lower and upper VA range
         */

        if (el == AARCH64_EL2) {
            e2h = VAL_EXTRACT_BITS(val_pe_reg_read(HCR_EL2), 34, 34);
            val_print_primary_pe(ACS_PRINT_INFO, "\n       HCR_EL2.E2H = %llx",
                                                                            e2h, index);
        }


        /* If FEAT_HPDS2 is implemented then TCR_ELx register is used for enabling
         * PBHA bits for Stage-1
         */
        data = val_pe_reg_read(TCR_ELx);

        if (el == AARCH64_EL1 || ((el == AARCH64_EL2) && e2h))
        {
           /* TCR_ELx.HWU0nn[46:43] can enable descriptor bits as PBHA only if
            * TCR_ELx.HPD0[41] !=0 for lower VA range in Stage-1
            *
            * Eg: TCR_ELx.HWU059[43] != 0 indicates 59th bit of VMSAv8-64 descriptor in
            * Lower VA range can be enabled as PBHA bit in Stage-1 only if TCR_ELx.HPD0[41] != 0
            */

            if (VAL_EXTRACT_BITS(data, 41, 41) != 0) {
                if (VAL_EXTRACT_BITS(data, 43, 46) != 0) {
                    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       TCR_ELx.HPD0 = %llx",
                                         VAL_EXTRACT_BITS(data, 41, 41), index);
                    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       TCR_ELx.HWU0nn = %llx",
                                         VAL_EXTRACT_BITS(data, 43, 46), index);
                    val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
                    return;
                }
            }

            /* Upper VA range PBHA bits can be enabled by TCR_ELx.HPD1[42] and
             * TCR_ELx.HWU1nn[50:47] in Stage-1 for both Stage-1 and Stage-2 */

            if (VAL_EXTRACT_BITS(data, 42, 42) != 0) {
                if (VAL_EXTRACT_BITS(data, 47, 50) != 0) {
                    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       TCR_ELx.HPD1 = %llx",
                                         VAL_EXTRACT_BITS(data, 42, 42), index);
                    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       TCR_ELx.HWU1nn = %llx",
                                         VAL_EXTRACT_BITS(data, 47, 50), index);
                    val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
                    return;
                }
            }
        } else {

           /* TCR_EL2.HWUnn[28:25] != 0 indicates corresponding PBHA bits can be
            * enabled for single VA range of EL2 only if TCR_EL2.HPD[24] != 0 and HCR_El2.E2H == 0
            */

            if (VAL_EXTRACT_BITS(data, 24, 24) != 0) {
                if (VAL_EXTRACT_BITS(data, 25, 28) != 0) {
                    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       TCR_EL2.HPD = %llx",
                                         VAL_EXTRACT_BITS(data, 24, 24), index);
                    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       TCR_EL2.HWUnn = %llx",
                                         VAL_EXTRACT_BITS(data, 25, 28), index);
                    val_set_status(index, RESULT_FAIL(TEST_NUM, 04));
                    return;
                }
             }
        }

        if (el != AARCH64_EL2) {
             val_print_primary_pe(ACS_PRINT_WARN, "\n       Current EL needs to be in EL2",
                                                             0, index);
             val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
             return;
        }

        /* VTCR_EL2.HWUnn[28:25] !=0 indicates corresponding PBHA bit can be enabled in Stage-2 */
        data = val_pe_reg_read(VTCR_EL2);
        if (VAL_EXTRACT_BITS(data, 25, 28) != 0) {
            val_print_primary_pe(ACS_PRINT_DEBUG, "\n       VTCR_EL2.HWUnn = %llx",
                                     VAL_EXTRACT_BITS(data, 25, 28), index);
            val_set_status(index, RESULT_FAIL(TEST_NUM, 05));
         } else
            val_set_status(index, RESULT_PASS(TEST_NUM, 02));
    }

}

uint32_t c042_entry(uint32_t num_pe)
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
