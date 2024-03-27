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
#include "val/common/include/acs_pe.h"
#include "val/sbsa/include/sbsa_val_interface.h"
#include "val/sbsa/include/sbsa_acs_memory.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_ras.h"
#include "val/common/include/acs_common.h"

#define TEST_NUM   (ACS_RAS_TEST_NUM_BASE + 12)
#define TEST_RULE  "SYS_RAS_2"
#define TEST_DESC  "Check Pseudo Fault Injection      "

static void *branch_to_test;

static
void
esr(uint64_t interrupt_type, void *context)
{
  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(ACS_PRINT_ERR, "\n       Received exception of type: %d", interrupt_type);
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
  uint64_t rec_index;
  RAS_ERR_IN_t err_in_params;
  RAS_ERR_OUT_t err_out_params;

  /* Get Number of nodes with RAS Functionality */
  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);
  if (status || (num_node == 0)) {
    val_print(ACS_PRINT_DEBUG, "\n       RAS Nodes not found. Skipping...", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
    return;
  }

  /* Run this test only if this pe node has ras Functionality */
  for (node_index = 0; node_index < num_node; node_index++) {

    /* Get Error Record number for this Node */
    status = val_ras_get_info(RAS_INFO_START_INDEX, node_index, &rec_index);
    if (status) {
      val_print(ACS_PRINT_DEBUG, "\n       Could not get Start Index for index %d", node_index);
      fail_cnt++;
      continue;
    }

    err_in_params.rec_index = rec_index;
    err_in_params.node_index = node_index;
    err_in_params.ras_error_type = ERR_CE;
    err_in_params.is_pfg_check = 0;

    test_skip = 0;

    /* Install sync and async handlers to handle exceptions.*/
    status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
    status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
    if (status)
    {
      val_print(ACS_PRINT_ERR, "\n      Failed in installing the exception handler", 0);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
      return;
    }
    branch_to_test = &&exception_return;

    /* Setup an error in an implementation defined way */
    status = val_ras_setup_error(err_in_params, &err_out_params);
    if (status == NOT_IMPLEMENTED) {
        val_print(ACS_PRINT_ERR, "\n       ras_setup_error API unimplemented", 0);
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }

    /* Inject error in an implementation defined way */
    status = val_ras_inject_error(err_in_params, &err_out_params);

exception_return:
    /* Read Status Register for RAS Nodes */
    status = val_ras_check_err_record(node_index, err_in_params.ras_error_type);
    if (status) {
      val_print(ACS_PRINT_ERR, "\n       Err Status Check Failed, for node %d", node_index);
      fail_cnt++;
      continue;
    }
  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
    return;
  } else if (test_skip) {
    val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
    return;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
ras012_entry(uint32_t num_pe)
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
