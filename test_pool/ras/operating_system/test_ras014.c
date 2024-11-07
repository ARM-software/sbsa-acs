/** @file
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
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
#include "val/common/include/acs_pe.h"
#include "val/sbsa/include/sbsa_val_interface.h"
#include "val/sbsa/include/sbsa_acs_memory.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_ras.h"
#include "val/common/include/acs_common.h"

#define TEST_NUM   (ACS_RAS_TEST_NUM_BASE + 14)
#define TEST_RULE  "S_RAS_01"
#define TEST_DESC  "Check RAS SR Interface ERI/FHI are PPI"

static void payload(void)
{

  uint32_t status;
  uint32_t fail_cnt = 0;
  uint64_t num_node;
  uint64_t value;
  uint32_t node_index;
  uint64_t eri_id = 0;
  uint64_t fhi_id = 0;
  uint32_t test_skip = 1;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  if (g_sbsa_level < 6) {
    val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
    return;
  }

  /* Get Number of RAS nodes */
  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);

  for (node_index = 0; node_index < num_node; node_index++) {

    /* Check for interface type is System Register Based */
    status = val_ras_get_info(RAS_INFO_INTF_TYPE, node_index, &value);
    if (status) {
      val_print(ACS_PRINT_DEBUG, "\n       Interface Type not found index %d", node_index);
      fail_cnt++;
      break;
    }
    if (value != 0)
      continue;

    /* Get ERI number for Node, If Not Skip the Node */
    status = val_ras_get_info(RAS_INFO_ERI_ID, node_index, &eri_id);
    if (status) {
      /* No ERI Support for this node */
      val_print(ACS_PRINT_DEBUG, "\n       ERI Not supported for node %d", node_index);
    } else {
      test_skip = 0;
      /* ERI Support, Check for PPI */
      if ((eri_id < 16) || (eri_id > 31)) {
        val_print(ACS_PRINT_ERR, "\n       ERI Not PPI for node %d", node_index);
        fail_cnt++;
        continue;
      }
    }

    /* Get FHI number for Node, If Not Skip the Node */
    status = val_ras_get_info(RAS_INFO_FHI_ID, node_index, &fhi_id);
    if (status) {
      /* No FHI Support for this node */
      val_print(ACS_PRINT_DEBUG, "\n       FHI Not supported for node %d", node_index);
    } else {
      test_skip = 0;
      /* FHI Support, Check for PPI */
      if ((fhi_id < 16) || (fhi_id > 31)) {
        val_print(ACS_PRINT_ERR, "\n       FHI Not PPI for node %d", node_index);
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

uint32_t ras014_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
