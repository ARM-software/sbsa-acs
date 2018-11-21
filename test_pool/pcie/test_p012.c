/** @file
 * Copyright (c) 2016-2018, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 12)
#define TEST_DESC  "PCI legacy interrupt SPI ID unique"

static uint32_t instance;
static uint32_t e_irq_line;
static uint32_t e_irq_pending;

static void intr_handler(void)
{
  /* Call exerciser specific function to handle the interrupt */
  val_exerciser_ops(HANDLE_INTR, e_irq_line, instance);

  /* Clear the interrupt pending state */
  e_irq_pending = 0;

  val_print(AVS_PRINT_DEBUG, "\n       Received legacy interrupt %d", e_irq_line);
}

static inline char pin_name(int pin)
{
    return 'A' + pin;
}

void check_irqmap_unique(PERIPHERAL_IRQ_MAP *irq_map, uint8_t *status)
{

  uint32_t ccnt;
  uint32_t ncnt;
  uint32_t current_irq_pin;
  uint32_t next_irq_pin;

  current_irq_pin = 0;
  next_irq_pin = current_irq_pin + 1;

  while (current_irq_pin < LEGACY_PCI_IRQ_CNT && status == 0) {

    while (next_irq_pin < LEGACY_PCI_IRQ_CNT && status == 0) {

        for (ccnt = 0; (ccnt < irq_map->legacy_irq_map[current_irq_pin].irq_count) && (status == 0); ccnt++) {

            for (ncnt = 0; (ncnt < irq_map->legacy_irq_map[next_irq_pin].irq_count) && (status == 0); ncnt++) {

                if (irq_map->legacy_irq_map[current_irq_pin].irq_list[ccnt] ==
                                  irq_map->legacy_irq_map[next_irq_pin].irq_list[ncnt]) {
                    *status = 7;
                    val_print (AVS_PRINT_ERR, "\n       Legacy interrupt %c routing", pin_name(current_irq_pin));
                    val_print (AVS_PRINT_ERR, "\n       is the same as %c routing", pin_name(next_irq_pin));
                }
            }
        }

        next_irq_pin++;
     }

     current_irq_pin++;
     next_irq_pin = current_irq_pin + 1;
  }

}

uint8_t irq_mapping_error(uint8_t *status)
{
    switch (*status) {
    case 0:
      break;
    case 1:
      val_print (AVS_PRINT_WARN, "\n       Unable to access PCI bridge device", 0);
      break;
    case 2:
      val_print (AVS_PRINT_WARN, "\n       Unable to fetch _PRT ACPI handle", 0);
      /* Not a fatal error, just skip this device */
      *status = 0;
      return 1;
    case 3:
      val_print (AVS_PRINT_WARN, "\n       Unable to access _PRT ACPI object", 0);
      /* Not a fatal error, just skip this device */
      *status = 0;
      return 1;
    case 4:
      val_print (AVS_PRINT_WARN, "\n       Interrupt hard-wire error", 0);
      /* Not a fatal error, just skip this device */
      *status = 0;
      return 1;
    case 5:
      val_print (AVS_PRINT_ERR, "\n       Legacy interrupt out of range", 0);
      break;
    case 6:
      val_print (AVS_PRINT_ERR, "\n       Maximum number of interrupts has been reached", 0);
      break;
    default:
      val_print (AVS_PRINT_ERR, "\n       Unknown error", 0);
      break;
    }

    return 0;
}

static
void
payload (void)
{

  uint32_t index;
  uint32_t count;
  PERIPHERAL_IRQ_MAP *irq_map;
  PERIPHERAL_IRQ_MAP *erp_irq_map;
  uint8_t status;
  uint64_t dev_bdf;
  uint32_t e_bdf = 0;
  uint32_t erp_bdf = 0;
  uint32_t start_bus;
  uint32_t start_segment;
  uint32_t start_bdf;
  uint8_t e_irq_pin;
  uint8_t e_irq_line;
  uint32_t ret_val;
  uint32_t timeout;

  status = 0;
  index = val_pe_get_index_mpid (val_pe_get_mpid());
  count = val_peripheral_get_info (NUM_ALL, 0);

  if(!count) {
     val_set_status (index, RESULT_SKIP (g_sbsa_level, TEST_NUM, 3));
     return;
  }

  irq_map = kzalloc(sizeof(PERIPHERAL_IRQ_MAP), GFP_KERNEL);
  if (!irq_map) {
    val_print (AVS_PRINT_ERR, "\n       Memory allocation error", 0);
    val_set_status (index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 01));
    return;
  }

  /* get legacy IRQ info from PCI devices */
  while (count > 0 && status == 0) {
    count--;
    if (val_peripheral_get_info (ANY_GSIV, count)) {
      dev_bdf = val_peripheral_get_info (ANY_BDF, count);
      status = val_pci_get_legacy_irq_map (dev_bdf, irq_map);

      /* Check if the irq mappings for this device are proper */
      if (irq_mapping_error(&status))
        continue;
    }

    /* Compare IRQ routings */
    if (!status)
      check_irqmap_unique(irq_map, &status);
  }

  kfree (irq_map);

  if (!status) {
    val_set_status (index, RESULT_PASS (g_sbsa_level, TEST_NUM, 01));
  } else {
    val_set_status (index, RESULT_FAIL (g_sbsa_level, TEST_NUM, status));
  }

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);
  if (instance == 0) {
    val_print(AVS_PRINT_INFO, "    No exerciser cards in the system %x", 0);
    return;
  }

  /* Set start_bdf segment and bus numbers to 1st ecam region values */
  start_segment = val_pcie_get_info(PCIE_INFO_SEGMENT, 0);
  start_bus = val_pcie_get_info(PCIE_INFO_START_BUS, 0);
  start_bdf = PCIE_CREATE_BDF(start_segment, start_bus, 0, 0);

  while (instance-- != 0) {

    /* Get the exerciser BDF */
    e_bdf = val_pcie_get_bdf(EXERCISER_CLASSCODE, start_bdf);
    start_bdf = val_pcie_increment_bdf(e_bdf);

    /* Read exerciser Interrupt Pin register. A value of 00h
     * indicates that the function uses no legacy interrupt
     * Message(s). If a device implements a single legacy
     * interrupt message, it must be INTA; if it implements
     * two legacy interrupt messages, they must be INTA and
     * INTB; and so forth.
     */
    val_pci_read_config_byte(e_bdf, PCIE_INTERRUPT_PIN, &e_irq_pin);

    /* Read exerciser interrupt line routing information
     * from Interrupt Line register. Values in this register
     * are programmed by system software and are system
     * architecture specific.
     */
    if (e_irq_pin) {
      val_pci_read_config_byte(e_bdf, PCIE_INTERRUPT_LINE, &e_irq_line);
    } else {
      continue;
    }

    /* Derive exerciser root port (ERP) bdf */
    erp_bdf = e_bdf;
    if(val_pcie_get_root_port_bdf(&erp_bdf)) {
      val_print(AVS_PRINT_ERR, "\n       ERP %x BDF fetch error", instance);
      val_set_status(index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
      return;
    }

    /* Allocate memory for irq mappings */
    erp_irq_map = val_memory_alloc(sizeof(PERIPHERAL_IRQ_MAP));
    if (!erp_irq_map) {
      val_print (AVS_PRINT_ERR, "\n       Memory allocation error", 0);
      val_set_status(index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
      return;
    }

    /* Read ERP legacy interrupt mappings */
    status = val_pci_get_legacy_irq_map(erp_bdf, erp_irq_map);

    /* Skip this instance if ERP mappings not proper */
    if (irq_mapping_error(&status))
      continue;

    if (!status) {

      /* Verify whether ERP legacy interrupts are one-to-one
       * mapped to the system interrupt controller pins.
       */
      check_irqmap_unique(erp_irq_map, &status);
      if (status)
        goto legacy_irq_test_failure;

      /* Register an interrupt handler to verify legacy intr functionality */
      ret_val = val_gic_install_isr(e_irq_line, intr_handler);
      if (ret_val)
        goto legacy_irq_test_failure;

      /* Set the interrupt trigger status to pending */
      e_irq_pending = 1;

      /* Trigger the legacy interrupt */
      val_exerciser_ops(GENERATE_L_INTR, e_irq_line, instance);

      /* PE busy polls to check the completion of interrupt service routine */
      timeout = TIMEOUT_LARGE;
      while ((--timeout > 0) && e_irq_pending);

      if (timeout == 0) {
        val_gic_free_interrupt(e_irq_line, 0);
        val_print(AVS_PRINT_ERR, "\n Interrupt trigger failed for instance %4x   ", instance);
        goto legacy_irq_test_failure;
      }

      /* Return the interrupt */
      val_gic_free_interrupt(e_irq_line, 0);

    }

    /* Return the irq mapping space to the Heap */
    val_memory_free(erp_irq_map);

  }

  if (!status) {
    val_set_status (index, RESULT_PASS (g_sbsa_level, TEST_NUM, 01));
  } else {
    val_set_status (index, RESULT_FAIL (g_sbsa_level, TEST_NUM, status));
  }

  return;

legacy_irq_test_failure:
  val_set_status(index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
  val_memory_free(erp_irq_map);
  return;

}

uint32_t
p012_entry (uint32_t num_pe)
{
  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test (TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP) {
      val_run_test_payload (TEST_NUM, num_pe, payload, 0);
  }

  /* get the result from all PE and check for failure */
  status = val_check_for_error (TEST_NUM, num_pe);

  val_report_status (0, SBSA_AVS_END (g_sbsa_level, TEST_NUM));

  return status;
}
