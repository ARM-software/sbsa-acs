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

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 6)
#define TEST_DESC  "Generate PCIe legacy interrupts   "

#define LEGACY_INTR_PIN_COUNT 1

static uint32_t instance;
static uint32_t e_intr_line;
static volatile uint32_t e_intr_pending;

static void intr_handler(void)
{
    /* Call exerciser specific function to handle the interrupt */
    val_exerciser_ops(CLEAR_INTR, e_intr_line, instance);

    /* Clear the interrupt pending state */
    e_intr_pending = 0;

    val_print(AVS_PRINT_INFO, " \n  Received legacy interrupt %d", e_intr_line);
    val_gic_end_of_interrupt(e_intr_line);

}

static
void
payload (void)
{

  uint32_t pe_index;
  uint32_t e_bdf;
  uint32_t ret_val;
  uint32_t timeout;
  uint32_t e_intr_pin;
  uint32_t status;
  PERIPHERAL_IRQ_MAP *e_intr_map;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

/* Allocate memory for interrupt mappings */
  e_intr_map = val_memory_alloc(sizeof(PERIPHERAL_IRQ_MAP));
  if (!e_intr_map) {
    val_print (AVS_PRINT_ERR, "\n       Memory allocation error", 00);
    val_set_status(pe_index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
    return;
  }

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  while (instance-- != 0) {

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

    /* Get the exerciser BDF */
    e_bdf = val_exerciser_get_bdf(instance);

    val_pcie_read_cfg(e_bdf, PCIE_INTERRUPT_LINE, &e_intr_pin);
    val_print (AVS_PRINT_DEBUG, "  e_intr_pin %x", e_intr_pin);

    if (((e_intr_pin >> 8) == 0) || ((e_intr_pin >> 8) > 4))
        continue;

    status = val_pci_get_legacy_irq_map(e_bdf, e_intr_map);
    if(!status) {

        e_intr_pending = 1;
        e_intr_line = e_intr_map->legacy_irq_map[0].irq_list[0];

        /* Register an interrupt handler to verify legacy interrupt functionality */
        ret_val = val_gic_install_isr(e_intr_line, intr_handler);
        if (ret_val)
            goto test_fail;

        /* Trigger the legacy interrupt */
        val_exerciser_ops(GENERATE_L_INTR, e_intr_line, instance);

        /* PE busy polls to check the completion of interrupt service routine */
        timeout = TIMEOUT_LARGE;
        while ((--timeout > 0) && e_intr_pending);

        if (timeout == 0) {
            val_gic_free_irq(e_intr_line, 0);
            val_print(AVS_PRINT_ERR, "\n       Interrupt trigger failed for bdf %lx   ", e_bdf);
            goto test_fail;
        }

        /* Return the interrupt */
        val_gic_free_irq(e_intr_line, 0);

   } else {
        val_print (AVS_PRINT_ERR, "\n       Legacy interrupt mapping Read error", status);
        goto test_fail;
   }
 }

  val_memory_free(e_intr_map);
  val_set_status (pe_index, RESULT_PASS (g_sbsa_level, TEST_NUM, 01));
  return;

test_fail:
  val_set_status(pe_index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
  val_memory_free(e_intr_map);
  return;

}

uint32_t
e006_entry (void)
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
