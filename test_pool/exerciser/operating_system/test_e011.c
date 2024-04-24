/** @file
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
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
#include "val/sbsa/include/sbsa_acs_ras.h"
#include "val/sbsa/include/sbsa_acs_exerciser.h"

#include "val/common/include/acs_pe.h"
#include "val/common/include/acs_pcie_enumeration.h"
#include "val/common/include/acs_pcie.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 11)
#define TEST_RULE  "PCI_ER_08"
#define TEST_DESC  "RAS ERR record for poisoned data      "

static
void
payload()
{

  uint32_t instance;
  uint32_t pe_index;
  uint32_t e_bdf;
  uint32_t erp_bdf;
  exerciser_data_t e_data;
  uint32_t status;
  uint32_t poison_support;
  uint32_t ras_node;
  uint32_t bar_data;
  uint64_t data;
  uint32_t fail_cnt = 0;
  uint32_t test_skip = 1;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  while (instance-- != 0) {

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

      e_bdf = val_exerciser_get_bdf(instance);
      val_print(ACS_PRINT_DEBUG, "\n       Exerciser BDF - 0x%x", e_bdf);

      val_pcie_enable_eru(e_bdf);
      val_pcie_enable_msa(e_bdf);

      if (val_pcie_get_rootport(e_bdf, &erp_bdf))
          continue;

      val_pcie_enable_eru(erp_bdf);
      val_pcie_enable_msa(erp_bdf);

      /* Enable the Poison mode in the exerciser,
       * so that it generates posion data on reads */
      val_exerciser_set_param(ENABLE_POISON_MODE, 0, 0, instance);

      /* Check if poison data forwarding is supported
       * Skip the RAS check if not supported */
      poison_support = val_exerciser_check_poison_data_forwarding_support();
      if (poison_support == 0) {
          val_print(ACS_PRINT_DEBUG, "\n       Poison forwarding not supported", 0);
          goto get_bar_data;
      }

      ras_node = val_exerciser_get_pcie_ras_compliant_err_node(e_bdf, erp_bdf);
      if (ras_node == NOT_IMPLEMENTED) {
          val_print(ACS_PRINT_ERR, "\n       No RAS compliant node to record PCIe Error", 0);
          val_print(ACS_PRINT_ERR, "\n       Skipping RAS check for BDF  - 0x%x", e_bdf);
          goto get_bar_data;
      }

      /* Set the ED bit RAS control register of the RAS node
       * to enable Error reporting and logging */
      status = val_exerciser_set_param(ENABLE_RAS_CTRL, ras_node, erp_bdf, instance);

get_bar_data:
      /* Get BAR 0 details for this instance */
      status = val_exerciser_get_data(EXERCISER_DATA_BAR0_SPACE, &e_data, instance);
      if (status == NOT_IMPLEMENTED) {
          val_print(ACS_PRINT_ERR, "\n       pal_exerciser_get_data() for MMIO not implemented", 0);

          /* Disable the Poison mode in the exerciser*/
          val_exerciser_set_param(DISABLE_POISON_MODE, 0, 0, instance);
          continue;
      } else if (status) {
          val_print(ACS_PRINT_ERR, "\n       Exerciser %d data read error", instance);

          /* Disable the Poison mode in the exerciser*/
          val_exerciser_set_param(DISABLE_POISON_MODE, 0, 0, instance);
          continue;
      }

      /* Test will run for atleast one endpoint */
      test_skip = 0;

      /* Read the BAR address should result in poisoned TLP being forwarded to the PE*/
      bar_data = (*(volatile addr_t *)e_data.bar_space.base_addr);
      if (bar_data != PCIE_UNKNOWN_RESPONSE) {
          val_print(ACS_PRINT_ERR, "\n       Memory reads not returning all 1's, BDF %x", e_bdf);
          fail_cnt++;
      }

      if (poison_support == 0) {
          val_print(ACS_PRINT_DEBUG, "\n       Skippping RAS check for BDF  - 0x%x", e_bdf);

          /* Disable the Poison mode in the exerciser*/
          val_exerciser_set_param(DISABLE_POISON_MODE, 0, 0, instance);
          continue;
      }

      /* Get the RAS Error Status register value of the RAS node implemented*/
      data = val_exerciser_get_ras_status(ras_node, e_bdf, erp_bdf);
      if (data == NOT_IMPLEMENTED) {
          val_print(ACS_PRINT_ERR, "\n       Couldn't read ERR STATUS reg for node %x", ras_node);
          fail_cnt++;

          /* Disable the Poison mode in the exerciser*/
          val_exerciser_set_param(DISABLE_POISON_MODE, 0, 0, instance);
          continue;
      }

      /* SERR code to be 0x19 for PCIe error */
      if ((data & SERR_MASK) != 0x19) {
          val_print(ACS_PRINT_ERR, "\n       SERR bits did not record PCIe error, bdf %x", e_bdf);
          fail_cnt++;
      }

      /* PN bit to be set if Poisoned value is detected */
      if (!((data >> PN_SHIFT) & PN_MASK)) {
          val_print(ACS_PRINT_ERR, "\n       Poisoned(PN) bit not set, bdf %x", e_bdf);
          fail_cnt++;
      }

      /* UE and ER bit to be set indicating Error is undeferred and reported */
      if (((data >> UE_ER_SHIFT) & UE_ER_MASK) != 0x3) {
          val_print(ACS_PRINT_ERR, "\n       ER and UE bit not set, bdf %x", e_bdf);
          fail_cnt++;
      }

      /* UET bit to be 0x3 for PCIe error Uncorrected error, Signaled or Recoverable error*/
      if (((data >> UET_SHIFT) & UET_MASK) != 0x3) {
          val_print(ACS_PRINT_ERR, "\n       UET error not received, bdf %x", e_bdf);
          fail_cnt++;
      }

      /* DE to be 0 indicating that no errors was deferred*/
      if ((data >> DE_SHIFT) & DE_MASK) {
          val_print(ACS_PRINT_ERR, "\n       DE bit must not be set, bdf %x", e_bdf);
          fail_cnt++;
      }

      /* Disable the Poison mode in the exerciser*/
      val_exerciser_set_param(DISABLE_POISON_MODE, 0, 0, instance);

  }

  if (test_skip)
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  else if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));

  return;
}

uint32_t
e011_entry(void)
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
