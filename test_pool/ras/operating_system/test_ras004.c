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

#include "val/common/include/acs_val.h"
#include "val/sbsa/include/sbsa_val_interface.h"
#include "val/sbsa/include/sbsa_acs_memory.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_ras.h"
#include "val/common/include/acs_common.h"

#define TEST_NUM   (ACS_RAS_TEST_NUM_BASE + 4)
#define TEST_RULE  "RAS_04"
#define TEST_DESC  "Check ERI in Error Record Group   "

static
void
payload()
{

  uint32_t status;
  uint32_t fail_cnt = 0, test_skip = 1;
  uint64_t num_node;
  uint32_t node_index, sec_node;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t base_addr, base_addr_sec;
  uint64_t eri_id, eri_id_sec;

  /* Get Number of nodes with RAS Functionality */
  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);
  if (status || (num_node < 2)) {
    val_print(ACS_PRINT_DEBUG, "\n       RAS Nodes should be more than 1. Skipping...", 0);
    val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
    return;
  }

  for (node_index = 0; node_index < (num_node - 1); node_index++) {

    /* Get Current Node Base Address */
    status = val_ras_get_info(RAS_INFO_BASE_ADDR, node_index, &base_addr);
    if (status) {
      /* Interface is System Register based, Skipping this node */
      val_print(ACS_PRINT_DEBUG, "\n       Interface is SR, Skipping node %d", node_index);
      continue;
    }

    /* Get ERI number for Node, If Not Skip the Node */
    status = val_ras_get_info(RAS_INFO_ERI_ID, node_index, &eri_id);
    if (status) {
      val_print(ACS_PRINT_DEBUG, "\n       ERI Not supported for index %d", node_index);
      continue;
    }

    test_skip = 0;

    /* Compare with Rest of the node having same Base Address */
    for (sec_node = node_index + 1; sec_node < num_node; sec_node++) {

      /* Get Second Node Base Address */
      status = val_ras_get_info(RAS_INFO_BASE_ADDR, sec_node, &base_addr_sec);
      if (status) {
        /* Interface is System Register based, Skipping this node */
        val_print(ACS_PRINT_DEBUG, "\n       Interface is SR, Skipping sec_node %d", node_index);
        continue;
      }

      /* If not same base address then skip this node */
      if (base_addr != base_addr_sec)
        continue;

      /* Get ERI number for this Node */
      status = val_ras_get_info(RAS_INFO_ERI_ID, sec_node, &eri_id_sec);
      if (status) {
        val_print(ACS_PRINT_DEBUG, "\n       ERI Not supported for index %d", sec_node);
        continue;
      }

      /* Check if ERI is same otherwise fail the test */
      if (eri_id != eri_id_sec) {
        val_print(ACS_PRINT_ERR, "\n       ERI Diff for Same Group Nodes. Index %d", node_index);
        val_print(ACS_PRINT_ERR, " : %d", sec_node);
        fail_cnt++;
        continue;
      }
    }
  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
    return;
  } else if (test_skip) {
    val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
    return;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
ras004_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
