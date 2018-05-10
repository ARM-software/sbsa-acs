/** @file
 * Copyright (c) 2016-2018, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_secure.h"
#include "val/include/val_interface.h"

#define TEST_NUM   (AVS_PE_TEST_NUM_BASE  +  28)
#define TEST_DESC  "Check SVE if implemented          "
#define VEC_LEN_MAX 0x100000000

uint32_t update_cptr_zcr()
{
    SBSA_SMC_t smc;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    /* SMC Call to Set CPTR_EL3.EZ[8] = 1, ZCR_EL3.LEN[3:0] = 0b1111, */
    /* CPTR_EL2.ZEN[17:16] = 1, ZCR_EL2.LEN[3:0] = 0b1111 */
    smc.test_index = SBSA_SECURE_UPDATE_SVE_REG;
    val_secure_call_smc(&smc);

    val_secure_get_result(&smc, 2);
    if(smc.test_arg02 != SBSA_SMC_INIT_SIGN){
        val_print(AVS_PRINT_WARN, "\n   ARM-TF firmware not ported, skipping this test", 0);
        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
        return 0;
    }

    return 1;
}
static void payload()
{
    uint64_t data = 0;
    uint64_t rdvl_value = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    if (g_sbsa_level < 3) {
        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
        return;
    }

    /* Read ID_AA64PFR0_EL1[35:32] for Scalar vector extension */
    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 32, 35);

    if (data == 0) {
        /* SVE Not Implemented Skip the test */
        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
        return;
    }

    /* Update CPTR_EL3.EZ, ZCR_EL3.ZEN, CPTR_EL2.ZEN, ZCR_EL2.LEN */
    if (!update_cptr_zcr()) {
        /* ARM TF Not Ported, Skip the test */
        return;
    }

    /* SVE Implemented Check for vector length maximum >= 256 Bits */
    rdvl_value = val_pe_reg_read(RDVL);

    if (rdvl_value >= VEC_LEN_MAX)
        val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
    else
        val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));

}

uint32_t c028_entry(uint32_t num_pe)
{
    uint32_t status = AVS_STATUS_FAIL;

    num_pe = 1;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
    /* This check is when user is forcing us to skip this test */
    if (status != AVS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe);
    val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

    return status;
}
