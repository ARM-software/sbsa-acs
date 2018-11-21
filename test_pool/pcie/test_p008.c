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
#include "val/include/sbsa_avs_exerciser.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 8)
#define TEST_DESC  "Check MSI(X) vectors uniqueness   "

#define EBUSY 16  /* Device or resource busy */

static uint32_t instance;
static uint32_t irq_num;
static uint32_t mapped_irq_num;
static uint32_t irq_pending;

static
int
intr_handler(void)
{
  /* Call exerciser specific function to handle the interrupt */
  val_exerciser_ops(HANDLE_INTR, irq_num, instance);

  /* Clear the interrupt pending state */
  irq_pending = 0;

  val_print(AVS_PRINT_DEBUG, "\n       Received MSI interrupt %d           ", irq_num);
  return 0;
}

/**
    @brief   Returns MSI(X) status of the device

    @param   dev_index    index of PCI device

    @return  0    device does not support MSI(X)
    @return  1    device supports MSI(X)
**/
static
uint32_t
check_msi_status (uint32_t dev_index) {
  uint32_t data;

  data = val_peripheral_get_info (ANY_FLAGS, dev_index);

  if ((data & PER_FLAG_MSI_ENABLED) &&
      val_peripheral_get_info (ANY_GSIV, dev_index)) {
    return 1;
  }

  return 0;
}

/**
    @brief   Compare two lists of MSI(X) vectors

    @param   list_one    pointer to a first list of MSI(X) vectors
    @param   list_two    pointer to a second list of MSI(X) vectors

    @return  0    no vectors duplicates are found
    @return  1    lists contain at leas one common MSI(X) vector
**/
static
uint32_t
check_list_duplicates (PERIPHERAL_VECTOR_LIST *list_one, PERIPHERAL_VECTOR_LIST *list_two)
{
  PERIPHERAL_VECTOR_LIST *flist_node;
  PERIPHERAL_VECTOR_LIST *slist_node;

  uint32_t fcount = 0;
  uint32_t scount = 0;
  uint32_t irq_start1, irq_end1;
  uint32_t irq_start2, irq_end2;

  flist_node = list_one;
  slist_node = list_two;

  while (flist_node != NULL) {
    while (slist_node != NULL) {
      irq_start1 = flist_node->vector.vector_irq_base;
      irq_end1 = flist_node->vector.vector_irq_base + flist_node->vector.vector_n_irqs - 1;
      irq_start2 = slist_node->vector.vector_irq_base;
      irq_end2 = slist_node->vector.vector_irq_base + slist_node->vector.vector_n_irqs - 1;
      if (!(irq_end1 < irq_start2 || irq_start1 > irq_end2))
        return 1;
      slist_node = slist_node->next;
      scount++;
    }
    slist_node = list_two;
    flist_node = flist_node->next;
    fcount++;
    scount = 0;
  }

  return 0;
}

/**
    @brief   Free memory allocated for a list of MSI(X) vectors

    @param   list    pointer to a list of MSI(X) vectors
**/
static
void
clean_msi_list (PERIPHERAL_VECTOR_LIST *list)
{
  PERIPHERAL_VECTOR_LIST *next_node;
  PERIPHERAL_VECTOR_LIST *current_node;

  current_node = list;
  while (current_node != NULL) {
    next_node = current_node->next;
    kfree (current_node);
    current_node = next_node;
  }
}

static
void
payload (void)
{

  uint32_t count = val_peripheral_get_info (NUM_ALL, 0);
  uint32_t index = val_pe_get_index_mpid (val_pe_get_mpid());
  uint8_t status;
  PERIPHERAL_VECTOR_LIST *current_dev_mvec;
  PERIPHERAL_VECTOR_LIST *next_dev_mvec;
  uint64_t current_dev_bdf;
  uint64_t next_dev_bdf;
  uint32_t count_next;
  uint32_t e_bdf = 0;
  uint32_t start_bus;
  uint32_t start_segment;
  uint32_t start_bdf;
  uint32_t irq_offset;
  uint32_t timeout;
  int32_t ret_val;
  PERIPHERAL_VECTOR_LIST *e_mvec;
  PERIPHERAL_VECTOR_LIST *temp_mvec;

  if(!count) {
     val_set_status (index, RESULT_SKIP (g_sbsa_level, TEST_NUM, 3));
     return;
  }

  status = 0;
  current_dev_mvec = NULL;
  next_dev_mvec = NULL;

  /*
    Pull each discovered PCI device and its list of MSI(X) vectors.
    Compare this list with MSI(X) vector lists of other discovered
    PCI devices and find duplicates exist.
  */
  while (count > 0 && !status) {
    count_next = count - 1;
    if (check_msi_status (count - 1)) {
      /* Get BDF of a device */
      current_dev_bdf = val_peripheral_get_info (ANY_BDF, count - 1);
      if (current_dev_bdf) {
        val_print (AVS_PRINT_INFO, "       Checking PCI device with BDF %4X\n", current_dev_bdf);
        /* Read MSI(X) vectors */
        if (val_get_msi_vectors (current_dev_bdf, &current_dev_mvec)) {

          /* Pull other PCI devices left in the devices list */
          while (count_next > 0 && !status) {
            if (check_msi_status (count_next - 1)) {
              /* Get BDF of a device */
              next_dev_bdf = val_peripheral_get_info (ANY_BDF, count_next - 1);
              /* Read MSI(X) vectors */
              if (val_get_msi_vectors (next_dev_bdf, &next_dev_mvec)) {
                /* Compare two lists of MSI(X) vectors */
                if(check_list_duplicates (current_dev_mvec, next_dev_mvec)) {
                  val_print (AVS_PRINT_ERR, "\n       Allocated MSIs are not unique", 0);
                  val_set_status (index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
                  status = 1;
                }
                clean_msi_list (next_dev_mvec);
              }
            }
            count_next--;
          }

          clean_msi_list (current_dev_mvec);
        }
      } else {
        val_print (AVS_PRINT_ERR, "\n       Failed to get address of PCI device", 0);
        val_set_status (index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 01));
        status = 1;
      }
    }
    count--;
  }

  if (!status) {
    val_set_status (index, RESULT_PASS (g_sbsa_level, TEST_NUM, 01));
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

    e_mvec = NULL;

    /* Read the MSI vectors for this exerciser instance */
    if (val_get_msi_vectors(e_bdf, &e_mvec)) {

        /* Check the uniqueness of MSI vectors for this instance */
        if(check_list_duplicates(e_mvec, e_mvec->next)) {
            val_print (AVS_PRINT_ERR, "\n       MSIs are not unique for instance %4x", instance);
            val_set_status (index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
            return;
        }

        temp_mvec = e_mvec;

        while (e_mvec) {

            for (irq_offset = 0; irq_offset < e_mvec->vector.vector_n_irqs; irq_offset++) {

                irq_num = e_mvec->vector.vector_irq_base + irq_offset;
                mapped_irq_num = e_mvec->vector.vector_mapped_irq_base + irq_offset;

                /* Register the interrupt handler */
                ret_val = val_gic_request_irq(irq_num, mapped_irq_num, intr_handler);

                if (ret_val == -EBUSY) {
                    val_print(AVS_PRINT_WARN, "\n       IRQ is already in use for instance %4x", instance);
                    continue;
                } else if ((ret_val != -EBUSY) && (ret_val != 0)) {
                    val_print(AVS_PRINT_ERR, "\n       IRQ registration failed for instance %4x", instance);
                    val_set_status(index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
                    clean_msi_list(temp_mvec);
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
                    val_gic_free_interrupt(irq_num, mapped_irq_num);
                    clean_msi_list(temp_mvec);
                    return;
                }

                /* Return the interrupt */
                val_gic_free_interrupt(irq_num, mapped_irq_num);
            }

            e_mvec = e_mvec->next;
        }

        /* Return this instance dynamic memory to the heap manager */
        clean_msi_list(temp_mvec);

    } else {
        val_print(AVS_PRINT_ERR, "\n       Failed to get MSI vectors for instance %4x", instance);
        val_set_status(index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 02));
        return;
    }

  }

}

uint32_t
p008_entry (uint32_t num_pe)
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
