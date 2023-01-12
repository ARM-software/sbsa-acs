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

#define TEST_NUM   (AVS_RAS_TEST_NUM_BASE + 11)
#define TEST_RULE  "SYS_RAS_2,SYS_RAS_3"
#define TEST_DESC  "Check Poison Storage & Forwarding "

static uint32_t esr_pending = 1;
static uint64_t int_id;

static
void
intr_handler(void)
{
  /* Clear the interrupt pending state */

  val_print(AVS_PRINT_INFO, "\n       Received interrupt %x       ", 0);
  val_gic_end_of_interrupt(int_id);
  return;
}

static void *branch_to_test;

static
void
esr(uint64_t interrupt_type, void *context)
{
  esr_pending = 0;

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(AVS_PRINT_ERR, "\n       Received exception of type: %d", interrupt_type);
}

static
void
payload()
{

  uint32_t status;
  uint32_t fail_cnt = 0, test_skip = 1;
  uint64_t num_node;
  uint64_t value;
  uint64_t mc_status;
  uint64_t pe_status;
  uint32_t node_index;
  uint32_t poison_check;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t mpidr = val_pe_get_mpid();
  uint64_t rec_index;
  RAS_ERR_IN_t err_in_params;
  RAS_ERR_OUT_t err_out_params;
  uint64_t pe_node_index;

  /* Get Number of nodes with RAS Functionality */
  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);
  if (status || (num_node == 0)) {
    val_print(AVS_PRINT_DEBUG, "\n       RAS Nodes not found. Skipping...", 0);
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  /* Run this test only if this pe node has ras Functionality */
  /* Check current PE RAS Support with mpidr */
  status = val_ras_get_info(RAS_INFO_NODE_INDEX_FOR_AFF, mpidr, &pe_node_index);
  if (status) {
    val_print(AVS_PRINT_DEBUG, "\n       RAS Node not found for PE", 0);
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
    return;
  }

  /* Check if Platform Supports Poison Storage & Forwarding */
  poison_check = val_ras_check_plat_poison_support();

  for (node_index = 0; node_index < num_node; node_index++) {

    /* Get Current Node Type */
    status = val_ras_get_info(RAS_INFO_NODE_TYPE, node_index, &value);
    if (status) {
      val_print(AVS_PRINT_DEBUG, "\n       Node Type not found index %d", node_index);
      fail_cnt++;
      break;
    }

    /* Check if Node is Memory Controller */
    if (value != NODE_TYPE_MC)
        continue;


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

    /* Get Interrupt details for this node */
    status = val_ras_get_info(RAS_INFO_ERI_ID, node_index, &int_id);
    if (status) {
      /* Interrupt details not found, Failing for this node */
      val_print(AVS_PRINT_DEBUG, "\n       No Intr found, Failed for node %d", node_index);
      fail_cnt++;
      continue;
    }

    test_skip = 0;

    /* Install sync and async handlers to handle exceptions.*/
    status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
    status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
    if (status)
    {
      val_print(AVS_PRINT_ERR, "\n      Failed in installing the exception handler", 0);
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
      return;
    }
    branch_to_test = &&exception_return;

    /* Install handler for interrupt */
    val_gic_install_isr(int_id, intr_handler);

    /* Setup an error in an implementation defined way */
    status = val_ras_setup_error(err_in_params, &err_out_params);
    if (status) {
      val_print(AVS_PRINT_ERR, "\n       val_ras_setup_error failed, node %d", node_index);
      fail_cnt++;
      break;
    }

    /* Inject error in an implementation defined way */
    status = val_ras_inject_error(err_in_params, &err_out_params);
    if (status) {
      val_print(AVS_PRINT_ERR, "\n       val_ras_inject_error failed, node %d", node_index);
      fail_cnt++;
      break;
    }

exception_return:
    /* Read Status Register for Memory Controller RAS Node */
    status = val_ras_check_err_record(node_index, err_in_params.ras_error_type);
    if (status) {
      val_print(AVS_PRINT_ERR, "\n       MC Err Status Check Failed, for node %d", node_index);
      fail_cnt++;
      continue;
    }

    /* Read Status Register for PE RAS Node */
    status = val_ras_check_err_record(pe_node_index, err_in_params.ras_error_type);
    if (status) {
      val_print(AVS_PRINT_ERR, "\n       PE Err Status Check Failed, for node %d", node_index);
      fail_cnt++;
      continue;
    }

    if (poison_check) {
      /* Poison Check only if Poison Storage & Forwarding Supported */
      /* Read Status Register for RAS Nodes */
      mc_status = val_ras_reg_read(node_index, RAS_ERR_STATUS, rec_index);
      if (mc_status == INVALID_RAS_REG_VAL) {
          val_print(AVS_PRINT_ERR,
                    "\n       Couldn't read ERR<%d>STATUS register for ",
                    rec_index);
          val_print(AVS_PRINT_ERR,
                    "RAS node index: 0x%lx",
                    node_index);
          fail_cnt++;
          continue;
      }
      pe_status = val_ras_reg_read(pe_node_index, RAS_ERR_STATUS, rec_index);
      if (pe_status == INVALID_RAS_REG_VAL) {
          val_print(AVS_PRINT_ERR,
                    "\n       Couldn't read ERR<%d>STATUS register for ",
                    rec_index);
          val_print(AVS_PRINT_ERR,
                    "RAS node index: 0x%lx",
                    pe_node_index);
          fail_cnt++;
          continue;
      }

      /* Check Poison Information Storage/Forwarding in MC/PE Ras Node */
      if (!(mc_status & ERR_STATUS_PN_MASK)) {
        val_print(AVS_PRINT_DEBUG, "\n       Poison Storage Fail, for node %d", node_index);
        fail_cnt++;
        continue;
      }
      if (!(pe_status & ERR_STATUS_PN_MASK)) {
        val_print(AVS_PRINT_DEBUG, "\n       Poison Frwding Fail, for node %d", pe_node_index);
        fail_cnt++;
        continue;
      }
    } else {
      /* Check for External Abort */
      if (esr_pending) {
        val_print(AVS_PRINT_DEBUG, "\n       EA Check Fail, for node %d", pe_node_index);
        fail_cnt++;
        continue;
      }
    }
  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 04));
    return;
  } else if (test_skip) {
    val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
ras011_entry(uint32_t num_pe)
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
