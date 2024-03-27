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
#include "val/sbsa/include/sbsa_acs_mpam.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_ras.h"
#include "val/common/include/acs_common.h"

#define TEST_NUM   (ACS_RAS_TEST_NUM_BASE + 6)
#define TEST_RULE  "RAS_07"
#define TEST_DESC  "RAS ERR<n>ADDR.AI bit status check"

#define ONE_BYTE_BUFFER 0x1

static
void
payload()
{
  uint64_t num_node;
  uint64_t num_mc_node;
  uint64_t node_type;
  uint64_t err_inj_addr;
  uint64_t prox_base_addr;
  uint64_t num_err_recs;
  uint64_t err_rec_impl_bitmap;
  uint64_t err_rec_addrmode_bitmap;
  uint64_t data;

  uint32_t status;
  uint32_t fail_cnt = 0, test_skip = 0;
  uint32_t node_index;
  uint64_t mc_prox_domain;
  uint32_t err_rec_addrmode;
  uint32_t err_rec_addr_ai;
  uint32_t err_rec_index;
  uint32_t err_inj_addr_data;
  uint32_t err_recorded = 0;
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
    val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
    return;
  }

  for (node_index = 0; node_index < num_node; node_index++) {
      /* check whether current node is memory controller node */
      status = val_ras_get_info(RAS_INFO_NODE_TYPE, node_index, &node_type);
      if (status) {
          val_print(ACS_PRINT_ERR,
                    "\n       Couldn't get node type for RAS node index: 0x%lx",
                    node_index);
          fail_cnt++;
          continue;
      }

      if (node_type == NODE_TYPE_MC) {
          /* get number of error records for the current RAS node
            (both implemented and unimplemented) */
          status = val_ras_get_info(RAS_INFO_NUM_ERR_REC, node_index, &num_err_recs);
          if (status) {
              val_print(ACS_PRINT_ERR,
                        "\n       Couldn't get number of error records for RAS node index: 0x%lx",
                        node_index);
              fail_cnt++;
              continue;
          }
          /* skip if num of error records in zero */
          if (num_err_recs == 0) {
              val_print(ACS_PRINT_DEBUG,
                        "\n       Number of error records for RAS node index: 0x%lx is zero",
                        node_index);
              test_skip++;
              continue;
          }

          /* get proximity domain of RAS memory controller node */
          status = val_ras_get_info(RAS_INFO_MC_RES_PROX_DOMAIN, node_index, &mc_prox_domain);
          if (status) {
              val_print(ACS_PRINT_ERR,
                        "\n       Couldn't get MC proximity domain for RAS node index: 0x%lx",
                        node_index);
              fail_cnt++;
              continue;
          }

          /* fetch base addr of the proximity domain to inject an error in platform
             defined method */
          prox_base_addr = val_srat_get_info(SRAT_MEM_BASE_ADDR, mc_prox_domain);
          if (prox_base_addr == SRAT_INVALID_INFO) {
              val_print(ACS_PRINT_ERR,
                        "\n       Invalid base address for proximity domain : 0x%lx",
                        mc_prox_domain);
              fail_cnt++;
              continue;
          }

          /* check if the address accessible to PE by trying to allocate the address */
          err_inj_addr = (uint64_t)val_mem_alloc_at_address(prox_base_addr, ONE_BYTE_BUFFER);

          if (err_inj_addr == 0) {
              val_print(ACS_PRINT_ERR,
                        "\n       Unable to allocate address in proximity domain : 0x%lx failed.",
                        mc_prox_domain);
              /* test not applicable if memory isn't accessible by PE */
              test_skip++;
              continue;
          }

          /* inject error with following parameters */
          err_in_params.rec_index = 0;           /* not applicable for scenario*/
          err_in_params.node_index = node_index;
          err_in_params.ras_error_type = ERR_CE; /* correctable error */
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

          /* Inject error in an implementation defined way */
          status = val_ras_inject_error(err_in_params, &err_out_params);
          if (status) {
            val_print(ACS_PRINT_ERR, "\n       val_ras_inject_error failed, node %d", node_index);
            fail_cnt++;
            break;
          }

          /* wait loop to allow system to inject the error */
          val_ras_wait_timeout(1);

          if (status) {
              val_print(ACS_PRINT_ERR,
                        "\n       Memory error injection for RAS node index: 0x%lx failed.",
                        node_index);
              fail_cnt++;
              continue;
          }

          /* perform a read to error-injected address, which will cause system to record the error
             with address syndrome in one of the error records present for the current RAS node */
          err_inj_addr_data = val_mmio_read(err_inj_addr);
          val_print(ACS_PRINT_DEBUG, "\n       Error injected address: 0x%llx", err_inj_addr);
          val_print(ACS_PRINT_DEBUG, "  Data read: 0x%lx", err_inj_addr_data);

          /* wait loop to allow system to update RAS error records */
          val_ras_wait_timeout(1);

          /* get error record implemented bitmap from RAS info table */
          status = val_ras_get_info(RAS_INFO_ERR_REC_IMP, node_index, &err_rec_impl_bitmap);
          if (status) {
              val_print(ACS_PRINT_ERR,
                        "\n       Couldn't get implemented rec bitmap for RAS node index: 0x%lx",
                        node_index);
              fail_cnt++;
              continue;
          }

          /* get addressing mode bitmap by which current RAS node populates
             ERR<n>ADDR field of error records */
          status = val_ras_get_info(RAS_INFO_ADDR_MODE, node_index, &err_rec_addrmode_bitmap);
          if (status) {
              val_print(ACS_PRINT_ERR,
                    "\n       Couldn't get implemented addr mode bitmap for RAS node index: 0x%lx",
                    node_index);
              fail_cnt++;
              continue;
          }

          /* Iterate through each error record implemented and check if ERR<n>ADDR.AI bit is 0b0
             if the address is the same as System Physical Address for the location,
             and 0b1 otherwise.*/
          for (err_rec_index = 0; err_rec_index < num_err_recs; err_rec_index++) {
              /* check if error record is implemented for current node
                 Bit[n] = 0b: Error record at index n is implemented.
                 Bit[n] = 1b: Error record at index n is not implemented.
                 If not implemented skip the error record */
              if ((err_rec_impl_bitmap >> err_rec_index) & 0x1)
                  continue;
              /* since we have injected error in a memory location in current MC proximity domain
                 space, one of the error record must have recorded address syndrome and
                 ERR<n>STATUS AV, bit [31] & V, bit [30] must be valid for that error record */

              /* if ERR<n>STATUS AV, bit [31] & V, bit [30] is invalid, continue with next
                 error record */
              data = val_ras_reg_read(node_index, RAS_ERR_STATUS, err_rec_index);
              if (data == INVALID_RAS_REG_VAL) {
                  val_print(ACS_PRINT_ERR,
                              "\n       Couldn't read ERR<%d>STATUS register for ",
                              err_rec_index);
                  val_print(ACS_PRINT_ERR,
                              "RAS node index: 0x%lx",
                              node_index);
                  fail_cnt++;
                  continue;
              }
              if (!((data & ERR_STATUS_V_MASK) && (data & ERR_STATUS_AV_MASK)))
                  continue;

              /* valid error record with address syndrome found */
              err_recorded = 1;

              /* get addressing mode for ERR<n>ADDR field in current error record
                 Bit[n] = 0b: Error record at index n reports System Physical
                 Addresses (SPA) in the ERR<n>_ADDR register.
                 Bit[n] = 1b: otherwise */
              err_rec_addrmode = (err_rec_addrmode_bitmap >> err_rec_index) & 0x1;

              /* read ERR<n>ADDR.AI bit */
              data = val_ras_reg_read(node_index, RAS_ERR_ADDR, err_rec_index);
              if (data == INVALID_RAS_REG_VAL) {
                  val_print(ACS_PRINT_ERR,
                              "\n       Couldn't read ERR<%d>STATUS register for ",
                              err_rec_index);
                  val_print(ACS_PRINT_ERR,
                              "RAS node index: 0x%lx",
                              node_index);
                  fail_cnt++;
                  continue;
              }
              err_rec_addr_ai = (data >> ERR_ADDR_AI_SHIFT) & 0x1;

              if (err_rec_addrmode == 0 && err_rec_addr_ai == 0) {
                  val_print(ACS_PRINT_DEBUG,
                      "\n       RAS node index: 0x%lx PASSED",
                      node_index);
                  /* error record comply with RAS_07 requirements */
                  break;
              } else {
                  fail_cnt++;
                  break;
              }
          }

          /* check if system RAS recorded the error in memory with address syndrome
             if no, rule is not applicable for the node  */
          if (!err_recorded) {
              val_print(ACS_PRINT_ERR,
                    "\n       Memory error not recorded for RAS node index: 0x%lx",
                    node_index);
              test_skip++;
          }
      }
  }

  if (fail_cnt) {
    val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
    return;
  } else if (test_skip) {
    val_set_status(index, RESULT_SKIP(TEST_NUM, 03));
    return;
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
ras006_entry(uint32_t num_pe)
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
