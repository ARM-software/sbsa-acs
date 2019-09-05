/** @file
 * Copyright (c) 2018-2019, Arm Limited or its affiliates. All rights reserved.
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

#define LEGACY_INTR_PIN_COUNT 4

static uint32_t instance;
static uint32_t e_intr_line;
static uint32_t e_intr_pending;

static void intr_handler(void)
{
    /* Call exerciser specific function to handle the interrupt */
    val_exerciser_ops(CLEAR_INTR, e_intr_line, instance);

    /* Clear the interrupt pending state */
    e_intr_pending = 0;

    val_print(AVS_PRINT_DEBUG, "\n       Received legacy interrupt %d", e_intr_line);
}


static
void
payload (void)
{

  uint32_t pe_index;
  uint8_t status;
  uint32_t e_bdf;
  uint32_t erp_bdf;
  uint32_t start_segment;
  uint32_t start_bus;
  uint32_t start_bdf;
  uint32_t ret_val;
  uint32_t timeout;
  uint32_t e_intr_pin;
  uint32_t pin_index;
  PERIPHERAL_IRQ_MAP *erp_intr_map;

  status = 0;
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  /* Set start_bdf segment and bus numbers to 1st ecam region values */
  start_segment = val_pcie_get_info(PCIE_INFO_SEGMENT, 0);
  start_bus = val_pcie_get_info(PCIE_INFO_START_BUS, 0);
  start_bdf = PCIE_CREATE_BDF(start_segment, start_bus, 0, 0);

  /* Allocate memory for interrupt mappings */
  erp_intr_map = val_memory_alloc(sizeof(PERIPHERAL_IRQ_MAP));
  if (!erp_intr_map) {
    val_print (AVS_PRINT_ERR, "\n       Memory allocation error", 00);
    val_set_status(pe_index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
    return;
  }

  while (instance-- != 0) {

    /* Get the exerciser BDF */
    e_bdf = val_pcie_get_bdf(EXERCISER_CLASSCODE, start_bdf);
    start_bdf = val_pcie_increment_bdf(e_bdf);

    for (pin_index = 1; pin_index <= LEGACY_INTR_PIN_COUNT; pin_index++) {

        /* Write to exerciser Interrupt Pin register. A value of 00h
         * indicates that the function uses no legacy interrupt
         * Message(s). Writing a value of 01h indicates, exerciser
         * supports legacy interrupt message INTA; Writing a value
         * of 02h indicates support for INTB; and so forth.
         */
        val_pcie_read_cfg(e_bdf, PCIE_INTERRUPT_LINE, &e_intr_pin);
        e_intr_pin = (e_intr_pin & (~0xFF00)) | (pin_index << 8);
        val_pcie_write_cfg(e_bdf, PCIE_INTERRUPT_LINE, e_intr_pin);

        /* Derive exerciser root port (ERP) bdf */
        erp_bdf = e_bdf;
        if (val_pcie_get_root_port_bdf(&erp_bdf)) {
            val_print(AVS_PRINT_ERR, "\n       ERP %x BDF fetch error", instance);
            val_set_status(pe_index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
            val_memory_free(erp_intr_map);
            return;
        }

        /* Read ERP legacy interrupt mappings */
        status = val_pci_get_legacy_irq_map(erp_bdf, erp_intr_map);
        if (!status) {

            e_intr_line = erp_intr_map->legacy_irq_map[pin_index].irq_list[0];

            /* Register an interrupt handler to verify legacy interrupt functionality */
            ret_val = val_gic_install_isr(e_intr_line, intr_handler);
            if (ret_val)
                goto test_fail;

            /* Set the interrupt trigger status to pending */
            e_intr_pending = 1;

            /* Trigger the legacy interrupt */
            val_exerciser_ops(GENERATE_L_INTR, e_intr_line, instance);

            /* PE busy polls to check the completion of interrupt service routine */
            timeout = TIMEOUT_LARGE;
            while ((--timeout > 0) && e_intr_pending);

            if (timeout == 0) {
                val_gic_free_irq(e_intr_line, 0);
                val_print(AVS_PRINT_ERR, "\n Interrupt trigger failed for instance %4x   ", instance);
                goto test_fail;
            }

            /* Return the interrupt */
            val_gic_free_irq(e_intr_line, 0);

        } else {
            val_print (AVS_PRINT_ERR, "\n       Legacy interrupt mapping Read error", status);
            goto test_fail;
        }

    }

  }

  val_memory_free(erp_intr_map);
  val_set_status (pe_index, RESULT_PASS (g_sbsa_level, TEST_NUM, 01));

test_fail:
  val_set_status(pe_index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
  val_memory_free(erp_intr_map);
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
