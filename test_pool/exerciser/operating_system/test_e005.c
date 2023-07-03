/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_pcie_enumeration.h"
#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_smmu.h"
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_exerciser.h"

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 5)
#define TEST_DESC  "PE 2/4/8B writes tp PCIe as 2/4/8B"
#define TEST_RULE  "S_PCIe_03"

static uint32_t transaction_size = 4;
static uint32_t run_flag;
static uint32_t fail_cnt;

static uint32_t test_sequence_check(uint32_t instance, uint64_t write_value)
{
  uint64_t idx;
  uint64_t transaction_data;

  for (idx = 0; idx < transaction_size; idx++) {
      val_exerciser_get_param(DATA_ATTRIBUTES, &transaction_data, &idx, instance);
      if (transaction_data !=  write_value) {
          val_print(AVS_PRINT_ERR, "\n       Exerciser %d arrival order check failed", instance);
          return 1;
      }
  }

  return 0;
}

/* Performs write on 2B data */
static uint32_t test_sequence_2B(uint16_t *addr, uint32_t instance)
{
  uint32_t e_bdf;
  uint64_t idx;
  uint64_t write_val = 0xABCD;

  e_bdf = val_exerciser_get_bdf(instance);

  /* Start monitoring exerciser transactions */
  if (val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance)) {
      val_print(AVS_PRINT_DEBUG,
               "\n       Exerciser BDF 0x%x - Unable to start transaction monitoring", e_bdf);
      return AVS_STATUS_SKIP;
  }

  run_flag = 1;

  /* Send the transaction on incremental addresses */
  for (idx = 0; idx < transaction_size; idx++) {
      /* Write transaction */
      val_mmio_write16((addr_t)addr, write_val);
      addr++;
  }

  /* Stop monitoring exerciser transactions */
  val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);

  return test_sequence_check(instance, write_val);
}

/* Performs write on 4B data */
static uint32_t test_sequence_4B(uint32_t *addr, uint32_t instance)
{
  uint32_t e_bdf;
  uint64_t idx;
  uint64_t write_val = 0xC0DEC0DE;

  e_bdf = val_exerciser_get_bdf(instance);

  /* Start monitoring exerciser transactions */
  if (val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance)) {
      val_print(AVS_PRINT_DEBUG,
               "\n       Exerciser BDF 0x%x - Unable to start transaction monitoring", e_bdf);
      return AVS_STATUS_SKIP;
  }

  run_flag = 1;

  /* Send the transaction on incremental addresses */
  for (idx = 0; idx < transaction_size; idx++) {
      val_mmio_write((addr_t)addr, write_val);
      addr++;
  }

  /* Stop monitoring exerciser transactions */
  val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);

  return test_sequence_check(instance, write_val);
}

/* Performs write on 8B data */
static uint32_t test_sequence_8B(uint64_t *addr, uint32_t instance)
{
  uint32_t e_bdf;
  uint64_t idx;
  uint64_t write_val = 0xCAFECAFECAFECAFE;

  e_bdf = val_exerciser_get_bdf(instance);

  /* Start monitoring exerciser transactions */
  if (val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance)) {
      val_print(AVS_PRINT_DEBUG,
               "\n       Exerciser BDF 0x%x - Unable to start transaction monitoring", e_bdf);
      return AVS_STATUS_SKIP;
  }

  run_flag = 1;

  /* Send the transaction on incremental addresses */
  for (idx = 0; idx < transaction_size; idx++) {
      /* Write transaction */
      val_mmio_write64((addr_t)addr, write_val);
      addr++;
  }

  /* Stop monitoring exerciser transactions */
  val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);

  return test_sequence_check(instance, write_val);
}

/* Read and Write on BAR space mapped to Device memory */
static
void
barspace_transactions_order_check(void)
{
  uint32_t instance;
  exerciser_data_t e_data;
  char *baseptr;
  uint32_t status;

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  while (instance-- != 0) {

    /* if init fail moves to next exerciser */
    if (val_exerciser_init(instance))
        continue;

    /* Get BAR 0 details for this instance */
    status = val_exerciser_get_data(EXERCISER_DATA_MMIO_SPACE, &e_data, instance);
    if (status == NOT_IMPLEMENTED) {
        val_print(AVS_PRINT_ERR, "\n       pal_exerciser_get_data() for MMIO not implemented", 0);
        continue;
    } else if (status) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %d data read error     ", instance);
        continue;
    }

    /* Map mmio space to ARM device memory in MMU page tables */
    baseptr = (char *)e_data.bar_space.base_addr;
    if (!baseptr) {
        val_print(AVS_PRINT_ERR, "\n       Failed in BAR ioremap for instance %x", instance);
        continue;
    }

    /* Test Scenario 1 : Transactions on incremental aligned address */
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, instance);
    fail_cnt += test_sequence_8B((uint64_t *)baseptr, instance);
  }
}

static
void
payload(void)
{
  uint32_t pe_index;

  pe_index = val_pe_get_index_mpid (val_pe_get_mpid());

  barspace_transactions_order_check();

  if (!run_flag) {
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
e005_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
