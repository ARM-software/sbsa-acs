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
#include "val/sbsa/include/sbsa_acs_mpam.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_ras.h"
#include "val/common/include/acs_common.h"

#define TEST_NUM   (ACS_RAS_TEST_NUM_BASE + 9)
#define TEST_RULE  "S_L7RAS_1"
#define TEST_DESC  "Data abort on Containable err         "

#define ONE_BYTE_BUFFER 0x1

static uint32_t esr_pending = 1;
static void *branch_to_test;

static
void
esr(uint64_t interrupt_type, void *context)
{
  esr_pending = 0;

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(ACS_PRINT_ERR, "\n       Received exception of type: %d", interrupt_type);
}

static
void
payload()
{
  uint64_t num_node;
  uint64_t num_mc_node;
  uint64_t node_type;
  uint64_t err_inj_addr;
  uint64_t prox_base_addr;

  uint32_t status;
  uint32_t fail_cnt = 0, test_skip = 0;
  uint32_t node_index;
  uint64_t mc_prox_domain;
  uint32_t err_inj_addr_data = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  RAS_ERR_IN_t err_in_params;
  RAS_ERR_OUT_t err_out_params;

  /* get number of nodes with RAS functionality */
  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_node);
  if (status || (num_node == 0)) {
    val_print(ACS_PRINT_ERR, "\n       RAS nodes not found. Skipping...", 0);
    val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
    return;
  }

  /* get number of MC nodes with RAS functionality */
  status = val_ras_get_info(RAS_INFO_NUM_MC, 0, &num_mc_node);
  if (status || (num_mc_node == 0)) {
    val_print(ACS_PRINT_ERR, "\n       RAS MC nodes not found. Skipping...", 0);
    val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
    return;
  }

  for (node_index = 0; node_index < num_node; node_index++) {

    /* check whether current node is memory controller node */
    status = val_ras_get_info(RAS_INFO_NODE_TYPE, node_index, &node_type);
    if (status) {
      val_print(ACS_PRINT_ERR, "\n       Couldn't get node type for node : 0x%lx", node_index);
      fail_cnt++;
      continue;
    }

    if (node_type != NODE_TYPE_MC)
      continue;

    /* Get proximity domain of RAS memory controller node */
    status = val_ras_get_info(RAS_INFO_MC_RES_PROX_DOMAIN, node_index, &mc_prox_domain);
    if (status) {
      val_print(ACS_PRINT_ERR, "\n       Couldn't get MC prox domain for node : 0x%lx", node_index);
      fail_cnt++;
      continue;
    }

    /* Get base addr for proximity domain to inject error in platform defined method */
    prox_base_addr = val_srat_get_info(SRAT_MEM_BASE_ADDR, mc_prox_domain);
    if (prox_base_addr == SRAT_INVALID_INFO) {
      val_print(ACS_PRINT_ERR, "\n       Invalid base for prox domain : 0x%lx", mc_prox_domain);
      fail_cnt++;
      continue;
    }

    /* check if the address accessible to PE by trying to allocate the address */
    err_inj_addr = (uint64_t)val_mem_alloc_at_address(prox_base_addr, ONE_BYTE_BUFFER);
    val_print(ACS_PRINT_ERR, "\n       err_inj_addr : 0x%lx", err_inj_addr);
    if (err_inj_addr == 0) {
      val_print(ACS_PRINT_ERR, "\n       Unable to allocate address in prox domain : 0x%lx",
                mc_prox_domain);
      /* test not applicable if memory isn't accessible by PE */
      test_skip++;
      continue;
    }

    /* Install sync and async handlers to handle exceptions.*/
    status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
    status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
    if (status)
    {
      val_print(ACS_PRINT_ERR, "\n      Failed in installing the exception handler", 0);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
      return;
    }
    branch_to_test = &&exception_return;

    /* Inject error with following parameters */
    err_in_params.rec_index = 0;           /* not applicable for scenario*/
    err_in_params.node_index = node_index;
    err_in_params.ras_error_type = ERR_CONTAINABLE; /* containable error */
    err_in_params.error_pa = err_inj_addr; /* address of the location where error
                                              needs to be injected */
    err_in_params.is_pfg_check = 0;        /* not a pseudo fault check */

    /* Setup error in an implementation defined way */
    status = val_ras_setup_error(err_in_params, &err_out_params);
    if (status) {
      val_print(ACS_PRINT_ERR, "\n       val_ras_setup_error failed, node %d", node_index);
      fail_cnt++;
      break;
    }

    /* Inject error in an implementation defined way.
       Inject error at an address, which will cause system to
       record the error on reading with address syndrome in one of
       the error records present for the current RAS node */
    status = val_ras_inject_error(err_in_params, &err_out_params);
    if (status) {
      val_print(ACS_PRINT_ERR, "\n       val_ras_inject_error failed, node %d", node_index);
      fail_cnt++;
      break;
    }

    /* wait loop to allow system to inject the error */
    val_ras_wait_timeout(10);

    /* Perform a read to error-injected address, which will cause
     * system to record the error with address syndrome in one of
     * the error records present for the current RAS node */
    err_inj_addr_data = (*(volatile addr_t *)err_inj_addr);
    val_print(ACS_PRINT_DEBUG, "\n       Error injected address: 0x%llx", err_inj_addr);
    val_print(ACS_PRINT_DEBUG, "  Data read: 0x%lx", err_inj_addr_data);


exception_return:
    val_print(ACS_PRINT_INFO, "\n       value esr_pending, %d", esr_pending);
    /* Check for External Abort */
    if (esr_pending) {
      val_print(ACS_PRINT_DEBUG, "\n       Data abort Check Fail, for node %d", node_index);
      fail_cnt++;
      continue;
    }
  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
    return;
  } else if (test_skip) {
    val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
    return;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
ras009_entry(uint32_t num_pe)
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
