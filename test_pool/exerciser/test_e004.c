/** @file
 * Copyright (c) 2018-2020, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_exerciser.h"

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 4)
#define TEST_DESC  "Generate MSI(X) interrupts        "


static uint32_t irq_pending;
static uint32_t lpi_int_id = 0x204C;

static
void
intr_handler(void)
{
  /* Clear the interrupt pending state */
  irq_pending = 0;

  val_print(AVS_PRINT_DEBUG, "\n       Received MSI interrupt %x", lpi_int_id);
  val_gic_end_of_interrupt(lpi_int_id);
  return;
}

static
void
payload (void)
{

  uint32_t count = val_peripheral_get_info (NUM_ALL, 0);
  uint32_t index = val_pe_get_index_mpid (val_pe_get_mpid());
  uint32_t e_bdf = 0;
  uint32_t timeout;
  uint32_t status;
  uint32_t instance;
  uint32_t num_cards;
  uint32_t msi_index = 0;

  if(!count) {
     val_set_status (index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 3));
     return;
  }

  /* Read the number of excerciser cards */
  num_cards = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  for (instance = 0; instance < num_cards; num_cards++) {

    /* Get the exerciser BDF */
    e_bdf = val_exerciser_get_bdf(instance);

    status = val_gic_request_msi(e_bdf, lpi_int_id, msi_index);

    if (status) {
        val_print(AVS_PRINT_ERR, "\n       MSI Assignment failed for bdf : 0x%x", e_bdf);
        val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
        return;
    }

    status = val_gic_install_isr(lpi_int_id, intr_handler);

    if (status) {
        val_print(AVS_PRINT_ERR, "\n       Intr handler registration failed for Interrupt : 0x%x", lpi_int_id);
        val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
        return;
    }

    /* Set the interrupt trigger status to pending */
    irq_pending = 1;

    /* Trigger the interrupt */
    val_exerciser_ops(GENERATE_MSI, msi_index, instance);

    /* PE busy polls to check the completion of interrupt service routine */
    timeout = TIMEOUT_LARGE;
    while ((--timeout > 0) && irq_pending);

    if (timeout == 0) {
        val_print(AVS_PRINT_ERR, "\n Interrupt trigger failed for Interrupt : 0x%x   ", lpi_int_id);
        val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
        val_gic_free_msi(e_bdf, lpi_int_id, msi_index);
        return;
    }

    /* Clear Interrupt and Mappings */
    val_gic_free_msi(e_bdf, lpi_int_id, msi_index);

  }

  /* Pass Test */
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

}

uint32_t
e004_entry (void)
{
  uint32_t num_pe = 1;
  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test (TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP) {
      val_run_test_payload (TEST_NUM, num_pe, payload, 0);
  }

  /* Get the result from all PE and check for failure */
  status = val_check_for_error (TEST_NUM, num_pe);

  val_report_status (0, SBSA_AVS_END (g_sbsa_level, TEST_NUM));

  return status;
}
