/** @file
 * Copyright (c) 2022 Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_ras.h"

#define TEST_NUM   (AVS_RAS_TEST_NUM_BASE + 10)
#define TEST_RULE  "SYS_RAS_1"
#define TEST_DESC  "Check for patrol scrubbing support"

static
void
payload()
{

  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_of_mem_blocks = 0;
  uint32_t i, scrub_support;
  uint32_t fail_cnt = 0;


  /* Note: RAS2 entry for a proximity domain imply that the domain supports error detection */
  /* get total number of RAS2 memory info blocks */
  num_of_mem_blocks = val_ras2_get_mem_info(RAS2_NUM_MEM_BLOCK, 0);
  if (num_of_mem_blocks == 0) {
    val_print(AVS_PRINT_DEBUG, "\n       No RAS2 memory nodes found. Skipping...", 0);
    val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  for (i = 0; i < num_of_mem_blocks; i++) {
      /* check whether current NUMA node (proximity domain) support patrol scrubbing */
      scrub_support = val_ras2_get_mem_info(RAS2_SCRUB_SUPPORT, i);
      if (scrub_support == 0) {
          val_print(AVS_PRINT_DEBUG,
                    "\n       Patrol scrubbing not supported by proximity domain: 0x%x",
                    val_ras2_get_mem_info(RAS2_PROX_DOMAIN, i));
          fail_cnt++;
      }
  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  return;
}

uint32_t
ras010_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);

  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
