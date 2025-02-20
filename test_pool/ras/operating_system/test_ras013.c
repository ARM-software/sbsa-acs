/** @file
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (ACS_RAS_TEST_NUM_BASE + 13)
#define TEST_RULE  "SYS_RAS_4"
#define TEST_DESC  "Check RAS memory mapped view supp     "

#define RESOURCE_FLAG(flag) ((flag >> 1) & 1)

static void payload(void)
{

  uint32_t status;
  uint32_t fail_cnt = 0;
  uint64_t num_node;
  uint64_t value;
  uint32_t node_index;
  uint64_t num_pc_node;
  uint32_t test_skip = 1;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  if (g_sbsa_level < 8) {
      val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  /* get number of PE nodes with RAS functionality */
  status = val_ras_get_info(RAS_INFO_NUM_PE, 0, &num_pc_node);
  if (status || (num_pc_node == 0)) {
      val_print(ACS_PRINT_ERR, "\n       RAS PE nodes not found. Skipping...", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
      return;
  }
  /* Get Number of RAS nodes */
  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);

  for (node_index = 0; node_index < num_node; node_index++) {

      status = val_ras_get_info(RAS_INFO_NODE_TYPE, node_index, &value);

      /* Check if Node is PE  */
      if (!(value == NODE_TYPE_PE))
          continue;

      /* Check for Resource Shared in case of Processor Node */
      status = val_ras_get_info(RAS_INFO_PE_FLAG, node_index, &value);
      if (status) {
          val_print(ACS_PRINT_DEBUG, "\n       PE Resource flag not found index %d", node_index);
          fail_cnt++;
          break;
      }

     /* Resources that are shared by two or more PEs, and are implementing Armv8 RAS Extensions,
      * must minimally support the memory-mapped view of the error nodes. */

      /* Check for Resource flag value must  be 1 (shared)*/
      if (RESOURCE_FLAG(value) != 1)
          continue;

      test_skip = 0;

      /* Check for interface type is memory-mapped */
      status = val_ras_get_info(RAS_INFO_INTF_TYPE, node_index, &value);
      if (status) {
          val_print(ACS_PRINT_DEBUG, "\n       Interface Type not found index %d", node_index);
          fail_cnt++;
          break;
      }
      if (value != 1) {
          val_print(ACS_PRINT_ERR, "\n       Interface Type must be MMIO for index %d", node_index);
          fail_cnt++;
          continue;
      }


  }

  if (fail_cnt) {
      val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
      return;
  } else if (test_skip) {
      val_print(ACS_PRINT_ERR, "\n       No Resource are Shared between two or more PE. "
                               "Skipping... ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 03));
      return;
    }

  val_set_status(index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t ras013_entry(uint32_t num_pe)
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
