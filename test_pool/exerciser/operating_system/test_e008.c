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

#include "val/common/include/acs_pcie_enumeration.h"
#include "val/sbsa/include/sbsa_acs_pcie.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_smmu.h"
#include "val/sbsa/include/sbsa_acs_memory.h"
#include "val/sbsa/include/sbsa_acs_exerciser.h"
#include "val/sbsa/include/sbsa_acs_pcie.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 8)
#define TEST_DESC  "Check 2/4/8 Bytes targeted writes     "
#define TEST_RULE  "S_PCIe_04"


static
uint32_t
get_target_exer_bdf(uint32_t req_rp_bdf, uint32_t *tgt_e_bdf,
                    uint32_t *tgt_rp_bdf, uint64_t *bar_base, uint32_t *tgt_instance)
{

  uint32_t erp_bdf;
  uint32_t e_bdf;
  uint32_t instance;
  uint32_t req_rp_ecam_index;
  uint32_t erp_ecam_index;
  uint32_t status;

  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  while (instance-- != 0)
  {
      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

      e_bdf = val_exerciser_get_bdf(instance);

      /* Read e_bdf BAR Register to get the Address to perform P2P */
      /* If No BAR Space, continue */
      val_pcie_get_mmio_bar(e_bdf, bar_base);
      if (*bar_base == 0)
          continue;

      /* Get RP of the exerciser */
      if (val_pcie_get_rootport(e_bdf, &erp_bdf))
          continue;

      if (req_rp_bdf != erp_bdf)
      {
          status = val_pcie_get_ecam_index(req_rp_bdf, &req_rp_ecam_index);
          if (status)
          {
             val_print(ACS_PRINT_ERR,
                       "\n       Error Ecam index for req RP BDF: 0x%x", req_rp_bdf);
             goto clean_fail;
          }

          status = val_pcie_get_ecam_index(erp_bdf, &erp_ecam_index);
          if (status)
          {
             val_print(ACS_PRINT_ERR, "\n       Error Ecam index for tgt RP BDF: 0x%x", erp_bdf);
             goto clean_fail;
          }

          if (req_rp_ecam_index != erp_ecam_index)
              continue;

          *tgt_e_bdf = e_bdf;
          *tgt_rp_bdf = erp_bdf;

          /* Enable Bus Master Enable */
          val_pcie_enable_bme(e_bdf);
          /* Enable Memory Space Access */
          val_pcie_enable_msa(e_bdf);

          *tgt_instance = instance;

          return ACS_STATUS_PASS;
      }
  }

clean_fail:
  /* Return failure if No Such Exerciser Found */
  *tgt_e_bdf = 0;
  *tgt_rp_bdf = 0;
  *bar_base = 0;
  return ACS_STATUS_FAIL;
}

static
uint32_t
check_sequence(uint64_t dma_buffer, uint32_t tgt_instance, uint32_t req_instance,
               uint64_t bar_base, uint32_t size)
{
  uint32_t status;
  uint64_t transaction_data;
  uint64_t idx;

  /* Copy the contents of the memory to requestor exercise's memory */
  val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)dma_buffer, size, req_instance);
  val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, req_instance);

  /* Set the destination buffer as BAR base address of target exerciser */
  val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)bar_base, size, req_instance);

  /* Start the transaction monitoring in the target exerciser */
  status = val_exerciser_ops(START_TXN_MONITOR, CFG_READ, tgt_instance);
  if (status == PCIE_CAP_NOT_FOUND)
  {
      val_print(ACS_PRINT_ERR, "\n       Transaction Monitoring capability not found", 0);
      return 1;
  }

  /* Copy the contents from requestor exerciser to target exerciser's BAR address */
  val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, req_instance);

  /* Stop the transaction monitoring in the target exerciser */
  status = val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, tgt_instance);
  if (status == PCIE_CAP_NOT_FOUND)
  {
      val_print(ACS_PRINT_ERR, "\n       Transaction Monitoring capability not found", 0);
      return 1;
  }

  /* Compare the transaction data */
  val_exerciser_get_param(DATA_ATTRIBUTES, &transaction_data, &idx, tgt_instance);
  if (val_memory_compare(&transaction_data, &dma_buffer, size))
  {
      val_print(ACS_PRINT_ERR,
                "\n       Data mismatch for target exerciser instance: %x", tgt_instance);
      val_print(ACS_PRINT_ERR, " with value: %x", transaction_data);
      return 1;
  }

  return 0;
}

static
void
payload(void)
{

  uint32_t status;
  uint32_t pe_index;
  uint32_t req_instance;
  uint32_t fail_cnt;
  uint32_t test_skip;
  uint32_t req_e_bdf, req_rp_bdf, tgt_e_bdf, tgt_rp_bdf, tgt_instance;
  uint64_t bar_base;
  uint64_t dma_buffer = 0xABCDC0DEABCDC0DE;


  fail_cnt = 0;
  test_skip = 1;
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  req_instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  status = val_pcie_p2p_support();
  /* Check If PCIe Hierarchy supports P2P. */
  if (status) {
    val_print(ACS_PRINT_DEBUG, "\n       PCIe hierarchy does not support P2P: %x", status);
    val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
    return;
  }


  while (req_instance-- != 0)
  {

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(req_instance))
          continue;

      req_e_bdf = val_exerciser_get_bdf(req_instance);
      val_print(ACS_PRINT_DEBUG, "\n       Requester exerciser BDF - 0x%x", req_e_bdf);

      /* Get RP of the exerciser */
      if (val_pcie_get_rootport(req_e_bdf, &req_rp_bdf))
          continue;

      /* Find another exerciser on other rootport,
         Break from the test if no such exerciser if found */
      if (get_target_exer_bdf(req_rp_bdf, &tgt_e_bdf, &tgt_rp_bdf, &bar_base, &tgt_instance))
          continue;

      test_skip = 0;

      status = check_sequence(dma_buffer, tgt_instance, req_instance, bar_base, 2);
      if (status)
      {
          val_print(ACS_PRINT_ERR,
                    "\n       Failed for 2B transaction from exerciser: %x", req_instance);
          fail_cnt++;
      }

      status = check_sequence(dma_buffer, tgt_instance, req_instance, bar_base, 4);
      if (status)
      {
          val_print(ACS_PRINT_ERR,
                    "\n       Failed for 4B transaction from exerciser: %x", req_instance);
          fail_cnt++;
      }

      status = check_sequence(dma_buffer, tgt_instance, req_instance, bar_base, 8);
      if (status)
      {
          val_print(ACS_PRINT_ERR,
                   "\n       Failed for 8B transaction from exerciser: %x", req_instance);
          fail_cnt++;
      }

  }

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 02));
  else if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));

  return;

}

uint32_t
e008_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
