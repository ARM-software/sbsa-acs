/** @file
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
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
#include "val/sbsa/include/sbsa_acs_gic.h"

#define TEST_NUM   (ACS_ETE_TEST_NUM_BASE + 8)
#define TEST_RULE  "ETE_10"
#define TEST_DESC  "Check GICC TRBE Interrupt field   "

static void payload(void)
{
    uint64_t int_id = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    if (g_sbsa_level < 8) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    /* Check for GICC TRBE GISV Interrupt is PPI */
    int_id = val_pe_get_gicc_trbe_interrupt(index);

    if (int_id == 1) {
      val_print_primary_pe(ACS_PRINT_DEBUG,
                    "\n       GICC TRBE interrupt field needs at least 6.5 ACPI table", 0, index);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
      return;
    }

    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       GICC TRBE INTERRUPT GISV = %d", int_id,
                                                                                       index);
    if (val_gic_is_valid_ppi(int_id))
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
    else
        val_set_status(index, RESULT_FAIL(TEST_NUM, 02));

}

uint32_t ete008_entry(uint32_t num_pe)
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

