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

#include "val/include/sbsa_avs_pcie.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 7)
#define TEST_DESC  "Check MSI support for PCIe device "

static
void
payload(void)
{

  uint32_t count = val_peripheral_get_info(NUM_ALL, 0);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t data;
  uint32_t dev_type;
  uint64_t dev_bdf;
  uint32_t status = 0;

  if (!count){
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  while (count > 0) {
      count--;
      dev_bdf = val_peripheral_get_info(ANY_BDF, count);
      /* Check for pcie device */
      if (!val_peripheral_is_pcie(dev_bdf))
          continue;

      dev_type = val_pcie_get_device_type(dev_bdf);
      /* Skipping MSI check for type-1 and type-2 headers */
      if ((!dev_type) || (dev_type > 1))
          continue;

      data = val_peripheral_get_info(ANY_FLAGS, count);

      if ((data & PER_FLAG_MSI_ENABLED) == 0) {
          val_print(AVS_PRINT_ERR, "\n       MSI should be enabled for a PCIe device ", 0);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          status = 1;
          break;
      }
  }
  if (!status)
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
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
