/** @file
 * Copyright (c) 2023 Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_ras.h"

#define TEST_NUM   (AVS_RAS_TEST_NUM_BASE + 7)
#define TEST_RULE  "RAS_08"
#define TEST_DESC  "Check Error Group Status          "

static
void
payload()
{

  uint32_t status;
  uint32_t fail_cnt = 0, test_skip = 1;
  uint64_t num_node;
  uint64_t value;
  uint32_t node_index;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t start_index;
  uint64_t err_rec_implement, err_status, record_imp_bits;

  /* Get Number of nodes with RAS Functionality */
  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);
  if (status || (num_node == 0)) {
    val_print(AVS_PRINT_DEBUG, "\n       RAS Nodes not found. Skipping...", 0);
    val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  for (node_index = 0; node_index < num_node; node_index++) {

    /* Get Current Node Interface Type */
    status = val_ras_get_info(RAS_INFO_BASE_ADDR, node_index, &value);
    if (status) {
      /* Interface is System Register based, Skipping this node */
      val_print(AVS_PRINT_DEBUG, "\n       Interface is SR, Skipping node %d", node_index);
      continue;
    }

    test_skip = 0;

    /* Get Start error index number for this Node */
    status = val_ras_get_info(RAS_INFO_START_INDEX, node_index, &start_index);
    if (status) {
      val_print(AVS_PRINT_DEBUG, "\n       Could not get Start Index for index %d", node_index);
      fail_cnt++;
      continue;
    }

    /* Check which error records are implemented in this node */
    status = val_ras_get_info(RAS_INFO_ERR_REC_IMP, node_index, &err_rec_implement);
    if (status) {
      val_print(AVS_PRINT_DEBUG, "\n       Could not get err rec info for index %d", node_index);
      fail_cnt++;
      continue;
    }

    /* Get error status reporting value for this node */
    status = val_ras_get_info(RAS_INFO_STATUS_REPORT, node_index, &err_status);
    if (status) {
      val_print(AVS_PRINT_DEBUG, "\n       Could not get status for index %d", node_index);
      fail_cnt++;
      continue;
    }

    /* This will have bit value 1 for all the implemented error records */
    record_imp_bits = (err_rec_implement ^ AVS_ALL_1_64BIT);

    if (err_status & record_imp_bits) {
      /* Fail the test as one of the implemented ER has error reporting not supported */
      val_print(AVS_PRINT_ERR, "\n       ERRGSR not supported for index %d", node_index);
      fail_cnt++;
      continue;
    }

  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
    return;
  } else if (test_skip) {
    val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
    return;
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
ras007_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);

  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
