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

#define TEST_NUM   (AVS_RAS_TEST_NUM_BASE + 5)
#define TEST_RULE  "RAS_06"
#define TEST_DESC  "Check ERI/FHI Connected to GIC    "

#define IS_NOT_SPI_PPI(int_id) ((int_id < 16) || (int_id > 1019))

static uint64_t int_id;
static uint32_t intr_pending = 1;

static
void
intr_handler(void)
{
  intr_pending = 0;

  /* Clear the interrupt pending state */

  val_print(AVS_PRINT_INFO, "\n       Received interrupt %x       ", 0);
  val_gic_end_of_interrupt(int_id);
  return;
}

static
void
payload()
{

  uint32_t status;
  uint32_t fail_cnt = 0, test_skip = 1;
  uint64_t num_node;
  uint32_t node_index;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t eri_id = 0, fhi_id = 0;
  uint64_t rec_index;
  RAS_ERR_IN_t err_in_params;
  RAS_ERR_OUT_t err_out_params;

  /* Get Number of nodes with RAS Functionality */
  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);
  if (status) {
    val_print(AVS_PRINT_ERR, "\n       RAS Nodes not present", 0);
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  for (node_index = 0; node_index < num_node; node_index++) {

    /* Get ERI number for Node, If Not Skip the Node */
    status = val_ras_get_info(RAS_INFO_ERI_ID, node_index, &eri_id);
    if (status) {
      /* No ERI Support for this node */
      val_print(AVS_PRINT_DEBUG, "\n       ERI Not supported for node %d", node_index);
    } else {
      test_skip = 0;
      /* ERI Support, Check for SPI/PPI */
      if (IS_NOT_SPI_PPI(eri_id)) {
        val_print(AVS_PRINT_ERR, "\n       ERI Not SPI/PPI for node %d", node_index);
        fail_cnt++;
        continue;
      }
    }

    /* Get FHI number for Node, If Not Skip the Node */
    status = val_ras_get_info(RAS_INFO_FHI_ID, node_index, &fhi_id);
    if (status) {
      /* No FHI Support for this node */
      val_print(AVS_PRINT_DEBUG, "\n       FHI Not supported for node %d", node_index);
    } else {
      test_skip = 0;
      /* FHI Support, Check for SPI/PPI */
      if (IS_NOT_SPI_PPI(fhi_id)) {
        val_print(AVS_PRINT_ERR, "\n       FHI Not SPI/PPI for node %d", node_index);
        fail_cnt++;
        continue;
      }
    }

    /* Check with Interrupt Generation if platform supports */
    if (test_skip == 0) {

      int_id = (fhi_id) ? fhi_id : eri_id;

      /* Get Error Record number for this Node */
      status = val_ras_get_info(RAS_INFO_START_INDEX, node_index, &rec_index);
      if (status) {
        val_print(AVS_PRINT_DEBUG, "\n       Could not get Start Index for index %d", node_index);
        fail_cnt++;
        continue;
      }

      err_in_params.rec_index = rec_index;
      err_in_params.node_index = node_index;
      err_in_params.ras_error_type = ERR_CE;
      err_in_params.is_pfg_check = 0;

      /* Install handler for interrupt */
      val_gic_install_isr(int_id, intr_handler);

      /* Setup an error in an implementation defined way */
      status = val_ras_setup_error(err_in_params, &err_out_params);
      if (status == NOT_IMPLEMENTED) {
        val_print(AVS_PRINT_DEBUG, "\n       Skipping Functional Check, node %d", node_index);
        continue;
      } else if (status) {
        val_print(AVS_PRINT_ERR, "\n       val_ras_setup_error failed, node %d", node_index);
        fail_cnt++;
        break;
      }

      /* Inject error in an implementation defined way */
      status = val_ras_inject_error(err_in_params, &err_out_params);
      if (status == NOT_IMPLEMENTED) {
        val_print(AVS_PRINT_DEBUG, "\n       Skipping Functional Check, node %d", node_index);
        continue;
      } else if (status) {
        val_print(AVS_PRINT_ERR, "\n       val_ras_inject_error failed, node %d", node_index);
        fail_cnt++;
        break;
      }

      /* Wait loop */
      val_ras_wait_timeout(1);

      if (intr_pending) {
        val_print(AVS_PRINT_ERR, "\n       Not Connected to GIC for node %d", node_index);
        fail_cnt++;
        continue;
      }
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
ras005_entry(uint32_t num_pe)
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
