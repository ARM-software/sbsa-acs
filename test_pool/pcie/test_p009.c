/** @file
 * Copyright (c) 2017, ARM Limited or its affiliates. All rights reserved.

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

#include "val/include/sbsa_avs_pcie.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 9)
#define TEST_DESC  "Check all MSI(X) vectors are LPIs "

#define LPI_BASE 8192

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
  PERIPHERAL_VECTOR_LIST *dev_mvec, *mvec;
  uint64_t dev_bdf;

  if(!count) {
     val_set_status (index, RESULT_SKIP (g_sbsa_level, TEST_NUM, 2));
     return;
  }

  status = 0;
  dev_mvec = NULL;

  /*
    Pull each discovered PCI device and its list of MSI(X) vectors.
    Check whether every vector IRQ number is an LPI or not.
  */
  while (count > 0 && !status) {
    if (check_msi_status (count - 1)) {
      /* Get BDF of a device */
      dev_bdf = val_peripheral_get_info (ANY_BDF, count - 1);
      if (dev_bdf) {
        val_print (AVS_PRINT_INFO, "       Checking PCI device with BDF %4X\n", dev_bdf);
        /* Read MSI(X) vectors */
        if (val_get_msi_vectors (dev_bdf, &dev_mvec)) {
          mvec = dev_mvec;
          while(mvec) {
              if(mvec->vector.vector_irq_base < LPI_BASE) {
                 val_print(AVS_PRINT_INFO, "     MSI vector irq %d is not an LPI\n", mvec->vector.vector_irq_base);
                 val_set_status (index, RESULT_FAIL (g_sbsa_level, TEST_NUM, mvec->vector.vector_irq_base));
                 status = 1;
              }
              mvec = mvec->next;
          }
          clean_msi_list (dev_mvec);
        }
      } else {
        val_print (AVS_STATUS_ERR, "\n       Failed to get address of PCI device", 0);
        val_set_status (index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 1));
        status = 1;
      }
    }
    count--;
  }

  if (!status) {
    val_set_status (index, RESULT_PASS (g_sbsa_level, TEST_NUM, 0));
  }
}

uint32_t
p009_entry (uint32_t num_pe)
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
