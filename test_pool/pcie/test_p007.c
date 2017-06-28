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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 7)
#define TEST_DESC  "Check MSI support for PCIe device "

static
void
payload(void)
{

  uint32_t count = val_peripheral_get_info(NUM_SATA, 0);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t data;

  if (!count){
      val_print(AVS_PRINT_WARN, "\n       Skipping because no SATA controller present ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  while (count != 0) {
      data = val_peripheral_get_info(SATA_FLAGS, count - 1);

      if ((data & PER_FLAG_MSI_ENABLED) == 0) {
          val_print(AVS_STATUS_ERR, "\n       MSI should be enabled for a PCIe device ", 0);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          break;
      } else {
          if (val_peripheral_get_info(SATA_GSIV, count - 1))
              val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
          else {
              val_print(AVS_STATUS_ERR, "\n       IRQ not assigned to the Device ", 0);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
              break;
          }
      }
      count--;
  }

}

uint32_t
p007_entry(uint32_t num_pe)
{
  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
