/** @file
 * Copyright (c) 2020, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 15)
#define TEST_DESC  "Arrival order & Gathering Check   "

#define TEST_DATA_1B 0xEC
#define TEST_DATA_2B 0xABDE
#define TEST_DATA_4B 0xDEADDAED
#define TEST_DATA_8B 0xDEADDAEDABEDCEAC

/* 0 means read transction, 1 means write transaction */
static uint32_t transaction_order[] = {1, 1, 0, 1, 0, 0, 0, 0};
static uint32_t run_flag;
static uint32_t fail_cnt;

/* num of transactions captured and thier attributes is checked */
static uint32_t test_sequence_check(uint32_t instance)
{
  uint64_t idx;
  uint64_t num_transactions;
  uint64_t transaction_type;

  /* Get number of transactions captured from exerciser */
  val_exerciser_get_param(NUM_TRANSACTIONS, NULL, &num_transactions, instance);
  if (num_transactions !=  sizeof(transaction_order)/sizeof(transaction_order[0])) {
      val_print(AVS_PRINT_ERR, "\n       Exerciser %d gathering check failed", instance);
      return 1;
  }

  /* Check transactions arrival order */
  for (idx = 0; idx < sizeof(transaction_order)/sizeof(transaction_order[0]); idx++) {
      val_exerciser_get_param(TRANSACTION_TYPE, &idx, &transaction_type, instance);
      if (transaction_type !=  transaction_order[idx]) {
          val_print(AVS_PRINT_ERR, "\n       Exerciser %d arrival order check failed", instance);
          return 1;
      }
  }
  return 0;
}

/* Performs reads/write on 1B data */
static uint32_t test_sequence_1B(uint8_t *addr, uint8_t increment_addr, uint32_t instance)
{
  uint64_t idx;

  /* Start monitoring exerciser transactions */
  if (val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance))
      return AVS_STATUS_SKIP;

  run_flag = 1;

  /* Send the transaction on incrementalt addresses */
  for (idx = 0; idx < sizeof(transaction_order)/sizeof(transaction_order[0]); idx++) {
      /* Write transaction */
      if (transaction_order[idx])
          val_mmio_write8((addr_t)addr, TEST_DATA_1B);
      else
          val_mmio_read8((addr_t)addr);
      if (increment_addr)
          addr++;
  }

  /* Stop monitoring exerciser transactions */
  val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);

  return test_sequence_check(instance);
}

/* Performs reads/write on 2B data */
static uint32_t test_sequence_2B(uint16_t *addr, uint8_t increment_addr, uint32_t instance)
{
  uint64_t idx;

  /* Start monitoring exerciser transactions */
  if (val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance))
      return AVS_STATUS_SKIP;

  run_flag = 1;

  /* Send the transaction on incrementalt addresses */
  for (idx = 0; idx < sizeof(transaction_order)/sizeof(transaction_order[0]); idx++) {
      /* Write transaction */
      if (transaction_order[idx])
          val_mmio_write16((addr_t)addr, TEST_DATA_2B);
      else
          val_mmio_read16((addr_t)addr);
      if (increment_addr)
          addr++;
  }

  /* Stop monitoring exerciser transactions */
  val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);

  return test_sequence_check(instance);
}

/* Performs reads/write on 4B data */
static uint32_t test_sequence_4B(uint32_t *addr, uint8_t increment_addr, uint32_t instance)
{
  uint64_t idx;

  /* Start monitoring exerciser transactions */
  if (val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance))
      return AVS_STATUS_SKIP;

  run_flag = 1;

  /* Send the transaction on incrementalt addresses */
  for (idx = 0; idx < sizeof(transaction_order)/sizeof(transaction_order[0]); idx++) {
      /* Write transaction */
      if (transaction_order[idx])
          val_mmio_write((addr_t)addr, TEST_DATA_4B);
      else
          val_mmio_read((addr_t)addr);
      if (increment_addr)
          addr++;
  }

  /* Stop monitoring exerciser transactions */
  val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);

  return test_sequence_check(instance);
}

/* Performs reads/write on 8B data */
static uint32_t test_sequence_8B(uint64_t *addr, uint8_t increment_addr, uint32_t instance)
{
  uint64_t idx;

  /* Start monitoring exerciser transactions */
  if (val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance))
      return AVS_STATUS_SKIP;

  run_flag = 1;

  /* Send the transaction on incrementalt addresses */
  for (idx = 0; idx < sizeof(transaction_order)/sizeof(transaction_order[0]); idx++) {
      /* Write transaction */
      if (transaction_order[idx])
          val_mmio_write64((addr_t)addr, TEST_DATA_8B);
      else
          val_mmio_read64((addr_t)addr);
      if (increment_addr)
          addr++;
  }

  /* Stop monitoring exerciser transactions */
  val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);

  return test_sequence_check(instance);
}

/* Read and Write on config space mapped to Device memory */

static
void
cfgspace_transactions_order_check(void)
{
  uint32_t instance;
  uint32_t bdf;
  char *baseptr;
  uint32_t cid_offset;
  uint64_t bdf_addr;

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  while (instance-- != 0) {

    /* if init fail moves to next exerciser */
    if (val_exerciser_init(instance))
        continue;

    /* Get exerciser bdf */
    bdf = val_exerciser_get_bdf(instance);

    /* If exerciser doesn't have PCI_ECAP skip the bdf */
    if (val_pcie_find_capability(bdf, PCIE_ECAP, CID_PCIECS, &cid_offset) == PCIE_CAP_NOT_FOUND)
        continue;

    bdf_addr = val_pcie_get_bdf_config_addr(bdf);

    /* Map config space to ARM device memory in MMU page tables */
    baseptr = (char *)val_memory_ioremap((void *)bdf_addr, 512, DEVICE_nGnRnE);

    if (!baseptr) {
        val_print(AVS_PRINT_ERR, "\n       Failed in config ioremap for instance %x", instance);
        continue;
    }

    /* Test Scenario 1 : Transactions on aligned address */
    fail_cnt += test_sequence_1B((uint8_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 1, instance);

    /* Test Scenario 2 : Transactions on PCIe misaligned address */
    baseptr += 1;
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 1, instance);

    /* Test Scenario 3 : Transactions on same address */
    fail_cnt += test_sequence_1B((uint8_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 0, instance);

    /* Test Scenario 4 : Transactions on same misaligned PCIe address */
    baseptr += 3;
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 0, instance);

  }
}

/* Read and Write on BAR space mapped to Device memory */

static
void
barspace_transactions_order_check(void)
{
  uint32_t instance;
  exerciser_data_t e_data;
  char *baseptr;

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  while (instance-- != 0) {

    /* if init fail moves to next exerciser */
    if (val_exerciser_init(instance))
        continue;

    /* Get BAR 0 details for this instance */
    if (val_exerciser_get_data(EXERCISER_DATA_BAR0_SPACE, &e_data, instance)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %d data read error     ", instance);
        continue;
    }

    /* If BAR region is not Prefetchable, skip the exerciser */
    if (e_data.bar_space.type != MMIO_PREFETCHABLE)
        continue;

    /* Map mmio space to ARM device memory in MMU page tables */
    baseptr = (char *)val_memory_ioremap((void *)e_data.bar_space.base_addr, 512, DEVICE_nGnRnE);

    if (!baseptr) {
        val_print(AVS_PRINT_ERR, "\n       Failed in BAR ioremap for instance %x", instance);
        continue;;
    }

    /* Test Scenario 1 : Transactions on aligned address */
    fail_cnt += test_sequence_1B((uint8_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_8B((uint64_t *)baseptr, 1, instance);

    /* Test Scenario 2 : Transactions on PCIe misaligned address */
    baseptr += 1;
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 1, instance);
    fail_cnt += test_sequence_8B((uint64_t *)baseptr, 1, instance);

    /* Test Scenario 3 : Transactions on same address */
    fail_cnt += test_sequence_1B((uint8_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_8B((uint64_t *)baseptr, 0, instance);

    /* Test Scenario 4 : Transactions on same misaligned PCIe address */
    baseptr += 3;
    fail_cnt += test_sequence_2B((uint16_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_4B((uint32_t *)baseptr, 0, instance);
    fail_cnt += test_sequence_8B((uint64_t *)baseptr, 0, instance);

  }
}

static
void
payload(void)
{
  uint32_t pe_index;

  pe_index = val_pe_get_index_mpid (val_pe_get_mpid());

  cfgspace_transactions_order_check();
  barspace_transactions_order_check();

  if(!run_flag) {
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
e015_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
