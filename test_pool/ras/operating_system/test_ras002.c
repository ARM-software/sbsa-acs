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
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_ras.h"

#define TEST_NUM   (AVS_RAS_TEST_NUM_BASE + 2)
#define TEST_RULE  "RAS_02"
#define TEST_DESC  "Check CFI, DUI, UI Controls       "

static
void
payload()
{

  uint32_t status;
  uint32_t fail_cnt = 0;
  uint64_t num_node;
  uint64_t value;
  uint32_t node_index;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Get Number of nodes with RAS Functionality */
  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);
  if (status || (num_node == 0)) {
    val_print(AVS_PRINT_DEBUG, "\n       RAS Nodes not found. Skipping...", 0);
    val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  for (node_index = 0; node_index < num_node; node_index++) {

    /* Get Current Node Type */
    status = val_ras_get_info(RAS_INFO_NODE_TYPE, node_index, &value);
    if (status) {
      val_print(AVS_PRINT_DEBUG, "\n       Node Type not found index %d", node_index);
      fail_cnt++;
      break;
    }

    /* Check if Node is Memory Controller/PE (Cache Resource) */
    if (!((value == NODE_TYPE_MC) || (value == NODE_TYPE_PE)))
        continue;

    /* Check for Cache Resource Type in case of Processor Node */
    if (value == NODE_TYPE_PE) {
      status = val_ras_get_info(RAS_INFO_PE_RES_TYPE, node_index, &value);
      if (status) {
        val_print(AVS_PRINT_DEBUG, "\n       PE Resource type not found index %d", node_index);
        fail_cnt++;
        break;
      }

      if (value != 0)
        continue;
    }

    /* Read FR register of the first error record */
    value = val_ras_reg_read(node_index, RAS_ERR_FR, 0);
    if (value == INVALID_RAS_REG_VAL) {
        val_print(AVS_PRINT_ERR,
                    "\n       Couldn't read ERR<0>FR register for RAS node index: 0x%lx",
                    node_index);
        fail_cnt++;
        continue;
    }

    /* Check DUI[17:16] != 0 of FR Register For DUI Control */
    if (!(value & ERR_FR_DUI_MASK)) {
      val_print(AVS_PRINT_ERR, "\n       DUI not implemented for node_index %d", node_index);
      fail_cnt++;
      continue;
    }

    /* Check CFI[11:10] != 0 of FR Register For CFI Control */
    if (!(value & ERR_FR_CFI_MASK)) {
      val_print(AVS_PRINT_ERR, "\n       CFI not implemented for node_index %d", node_index);
      fail_cnt++;
      continue;
    }

    /* Check UI[5:4] != 0 of FR Register For UI Control */
    if (!(value & ERR_FR_UI_MASK)) {
      val_print(AVS_PRINT_ERR, "\n       UI not implemented for node_index %d", node_index);
      fail_cnt++;
      continue;
    }
  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
ras002_entry(uint32_t num_pe)
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
