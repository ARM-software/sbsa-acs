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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 15)
#define TEST_DESC  "PCIe No Snoop transaction attr    "

static
void
payload (void)
{

  uint32_t index;
  uint32_t count;
  uint32_t no_snoop_set;
  uint32_t status;
  uint64_t dev_bdf;

  status = 0;
  no_snoop_set = 0;
  index = val_pe_get_index_mpid (val_pe_get_mpid());
  count = val_peripheral_get_info (NUM_ALL, 0);

  if(!count) {
     val_set_status (index, RESULT_SKIP (g_sbsa_level, TEST_NUM, 3));
     return;
  }

  /* Read No Snoop bit from the device control register */
  while (count > 0) {
    count--;
    dev_bdf = val_peripheral_get_info (ANY_BDF, count);
    if (val_pcie_get_dma_support (dev_bdf) == 1) {
      val_print (AVS_PRINT_INFO, "    have DMA support on %X", dev_bdf);
      if (val_pcie_get_dma_coherent(dev_bdf) == 1) {
        val_print (AVS_PRINT_INFO, "    DMA is coherent on %X", dev_bdf);
        status = val_pcie_get_snoop_bit (dev_bdf);
        if (status != 2) {
          no_snoop_set |= status;
          val_print (AVS_PRINT_INFO, "    no snoop bit is %d", status);
        }
      } else {
        val_print (AVS_PRINT_INFO, "    DMA is not coherent on %X", dev_bdf);
      }
    }
  }

  if(no_snoop_set) {
    val_print (AVS_STATUS_ERR, "\n       PCIe no snoop bit set to %d for a device with coherent DMA", no_snoop_set);
    val_set_status (index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 1));
  } else {
    val_set_status (index, RESULT_PASS (g_sbsa_level, TEST_NUM, status));
  }

}

uint32_t
p015_entry (uint32_t num_pe)
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
