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

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 4)
#define TEST_DESC  "Generate MSI(X) interrupts        "


static uint32_t instance;
static uint32_t irq_num;
static uint32_t mapped_irq_num;
static uint32_t irq_pending;

static
int
intr_handler(void)
{
  /* Call exerciser specific function to handle the interrupt */
  val_exerciser_ops(CLEAR_INTR, irq_num, instance);

  /* Clear the interrupt pending state */
  irq_pending = 0;

  val_print(AVS_PRINT_DEBUG, "\n       Received MSI interrupt %d           ", irq_num);
  return 0;
}

/**
    @brief   Free memory allocated for a list of MSI(X) vectors

    @param   list    pointer to a list of MSI(X) vectors
**/
static
void
free_msi_list (PERIPHERAL_VECTOR_LIST *list)
{
  PERIPHERAL_VECTOR_LIST *next_node;
  PERIPHERAL_VECTOR_LIST *current_node;

  current_node = list;
  while (current_node != NULL) {
    next_node = current_node->next;
    val_memory_free(current_node);
    current_node = next_node;
  }
}

static
void
payload (void)
{

  uint32_t count = val_peripheral_get_info (NUM_ALL, 0);
  uint32_t index = val_pe_get_index_mpid (val_pe_get_mpid());
  uint32_t e_bdf = 0;
  uint32_t start_bus;
  uint32_t start_segment;
  uint32_t start_bdf;
  uint32_t irq_offset;
  uint32_t timeout;
  uint32_t ret_val;
  PERIPHERAL_VECTOR_LIST *e_mvec;
  PERIPHERAL_VECTOR_LIST *temp_mvec;

  if(!count) {
     val_set_status (index, RESULT_SKIP (g_sbsa_level, TEST_NUM, 3));
     return;
  }

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  /* Set start_bdf segment and bus numbers to 1st ecam region values */
  start_segment = val_pcie_get_info(PCIE_INFO_SEGMENT, 0);
  start_bus = val_pcie_get_info(PCIE_INFO_START_BUS, 0);
  start_bdf = PCIE_CREATE_BDF(start_segment, start_bus, 0, 0);

  while (instance-- != 0) {

    /* Get the exerciser BDF */
    e_bdf = val_pcie_get_bdf(EXERCISER_CLASSCODE, start_bdf);
    start_bdf = val_pcie_increment_bdf(e_bdf);

    e_mvec = NULL;

    /* Read the MSI vectors for this exerciser instance */
    if (val_get_msi_vectors(e_bdf, &e_mvec)) {

        temp_mvec = e_mvec;

        while (e_mvec) {

            for (irq_offset = 0; irq_offset < e_mvec->vector.vector_n_irqs; irq_offset++) {

                irq_num = e_mvec->vector.vector_irq_base + irq_offset;
                mapped_irq_num = e_mvec->vector.vector_mapped_irq_base + irq_offset;

                /* Register the interrupt handler */
                ret_val = val_gic_request_irq(irq_num, mapped_irq_num, intr_handler);
                if (ret_val) {
                    val_print(AVS_PRINT_ERR, "\n       IRQ registration failed for instance %4x", instance);
                    val_set_status(index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
                    free_msi_list(temp_mvec);
                    return;
                }

                /* Set the interrupt trigger status to pending */
                irq_pending = 1;

                /* Trigger the interrupt */
                val_exerciser_ops(GENERATE_MSI, irq_num, instance);

                /* PE busy polls to check the completion of interrupt service routine */
                timeout = TIMEOUT_LARGE;
                while ((--timeout > 0) && irq_pending);

                if (timeout == 0) {
                    val_print(AVS_PRINT_ERR, "\n Interrupt trigger failed for instance %4x   ", instance);
                    val_set_status(index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
                    val_gic_free_irq(irq_num, mapped_irq_num);
                    free_msi_list(temp_mvec);
                    return;
                }

                /* Return the interrupt */
                val_gic_free_irq(irq_num, mapped_irq_num);
            }

            e_mvec = e_mvec->next;
        }

        /* Return this instance dynamic memory to the heap manager */
        free_msi_list(temp_mvec);

    } else {
        val_print(AVS_PRINT_ERR, "\n       Failed to get MSI vectors for instance %4x", instance);
        val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
        return;
    }

  }

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
