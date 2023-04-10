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
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_mpam.h"


#define TEST_NUM  (AVS_MPAM_TEST_NUM_BASE + 3)
#define TEST_RULE "S_L7MP_05, S_L7MP_06"
#define TEST_DESC "Check for MPAM MBWUs Monitor func "

#define BUFFER_SIZE 65536 /* 64 Kilobytes*/

static void payload(void)
{
    uint32_t pe_index;
    uint32_t msc_node_cnt, msc_index;
    uint32_t rsrc_node_cnt, rsrc_index;
    uint64_t mpam2_el2, mpam2_el2_temp;
    uint64_t byte_count;
    uint64_t addr_base, addr_len;
    uint64_t nrdy_timeout;
    uint32_t test_fails = 0;
    uint32_t test_skip = 1;
    void *src_buf = 0;
    void *dest_buf = 0;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

    if (g_sbsa_level < 7) {
        val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
        return;
    }

   /* Check if PE implements FEAT_MPAM */
    if (!((VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 40, 43) > 0) ||
        (VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR1_EL1), 16, 19) > 0))) {
            val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
            return;
    }


    /* get total number of MSCs reported by MPAM ACPI table */
    msc_node_cnt = val_mpam_get_msc_count();
    val_print(AVS_PRINT_DEBUG, "\n       MSC count = %d", msc_node_cnt);

    if (!msc_node_cnt) {
        val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 03));
        return;
    }

    /* MSC must implement MPAM v1.1 version */
    for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {
        if (val_mpam_msc_get_version(msc_index) != MPAM_VERSION_1_1) {
            val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
            return;
        }
    }

    /* read MPAM2_EL2 and store the value for restoring later */
    mpam2_el2 = val_mpam_reg_read(MPAM2_EL2);
    mpam2_el2_temp = mpam2_el2;

    /* Write DEFAULT_PARTID & DEFAULT PMG to mpam2_el2 to generate PE traffic */
    mpam2_el2 = (mpam2_el2 & ~(MPAMn_ELx_PARTID_D_MASK << MPAMn_ELx_PARTID_D_SHIFT)) |
                                                       DEFAULT_PARTID << MPAMn_ELx_PARTID_D_SHIFT;
    mpam2_el2 = (mpam2_el2 & ~(MPAMn_ELx_PMG_D_MASK << MPAMn_ELx_PMG_D_SHIFT)) |
                                                             DEFAULT_PMG << MPAMn_ELx_PMG_D_SHIFT;

    val_print(AVS_PRINT_DEBUG, "\n       Value written to MPAM2_EL2 = 0x%llx", mpam2_el2);
    val_mpam_reg_write(MPAM2_EL2, mpam2_el2);

    /* visit each MSC node and check for memory resources */
    for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {
        rsrc_node_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);

        val_print(AVS_PRINT_DEBUG, "\n       msc index  = %d", msc_index);
        val_print(AVS_PRINT_DEBUG, "\n       Resource count %d = ", rsrc_node_cnt);

        for (rsrc_index = 0; rsrc_index < rsrc_node_cnt; rsrc_index++) {

            /* check whether the resource location is memory */
            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index) ==
                                                                         MPAM_RSRC_TYPE_MEMORY) {

                /* As per S_L7MP_05, MBWU monitoring must be supported for general purpose mem */
                if (!val_mpam_msc_supports_mbwumon(msc_index)) {
                    val_print(AVS_PRINT_ERR, "\n       MBWU MON unsupported by MSC %d", msc_index);
                    test_fails++;
                    break;
                }

                test_skip = 0;
                /* select resource instance if RIS feature implemented */
                if (val_mpam_msc_supports_ris(msc_index))
                    val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

                val_print(AVS_PRINT_DEBUG, "\n       rsrc index = %d", rsrc_index);

                /* Allocate source and destination memory buffers*/
                addr_base = val_mpam_memory_get_base(msc_index, rsrc_index);
                addr_len  = val_mpam_memory_get_size(msc_index, rsrc_index);

                if ((addr_base == SRAT_INVALID_INFO) || (addr_len == SRAT_INVALID_INFO) ||
                    (addr_len <= 2 * BUFFER_SIZE)) { /* src and dst buffer size */
                    val_print(AVS_PRINT_ERR, "\n       No SRAT mem range info found", 0);
                    val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));

                    /* Restore MPAM2_EL2 settings */
                    val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);
                    return;
                }


                src_buf = (void *)val_mem_alloc_at_address(addr_base, BUFFER_SIZE);
                dest_buf = (void *)val_mem_alloc_at_address(addr_base + BUFFER_SIZE, BUFFER_SIZE);

                if ((src_buf == NULL) || (dest_buf == NULL)) {
                    val_print(AVS_PRINT_ERR, "\n       Memory allocation of buffers failed", 0);
                    val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));

                    /* Restore MPAM2_EL2 settings */
                    val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);
                    return;
                }

                /* configure MBWU Monitor for this memory resource node */
                val_mpam_memory_configure_mbwumon(msc_index);

                /* enable MBWU monitoring */
                val_mpam_memory_mbwumon_enable(msc_index);


                /* wait for MAX_NRDY_USEC after msc config change */
                nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
                while (nrdy_timeout) {
                    --nrdy_timeout;
                };

                /* perform memory operation */
                val_memcpy(src_buf, dest_buf, BUFFER_SIZE);

                /* read the memory bandwidth usage monitor */
                byte_count = val_mpam_memory_mbwumon_read_count(msc_index);

                /* disable and reset the MBWU monitor */
                val_mpam_memory_mbwumon_disable(msc_index);
                val_mpam_memory_mbwumon_reset(msc_index);

                val_print(AVS_PRINT_DEBUG, "\n       byte_count = 0x%llx bytes", byte_count);

                /* the monitor must count both read and write bandwidth,
                   hence count must be twice of the buffer size */
                if ((byte_count != 2 * BUFFER_SIZE)) {
                    val_print(AVS_PRINT_ERR, "\n       Monitor count incorrect for MSC %d",
                                                                                       msc_index);
                    val_print(AVS_PRINT_ERR, "       rsrc node %d", rsrc_index);
                    test_fails++;
                }

                /* free the buffers */
                val_mem_free_at_address((uint64_t)src_buf, BUFFER_SIZE);
                val_mem_free_at_address((uint64_t)dest_buf, BUFFER_SIZE);
            }
        }
    }
    /* Restore MPAM2_EL2 settings */
    val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);

    if (test_fails)
        val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 04));
    else if (test_skip)
        val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 04));
    else
        val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

    return;
}

uint32_t mpam003_entry(uint32_t num_pe)
{
    uint32_t status = AVS_STATUS_FAIL;

    num_pe = 1;
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
