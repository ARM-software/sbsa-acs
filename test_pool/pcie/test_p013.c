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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 13)
#define TEST_DESC  "Addressability of Non-Sec masters "

#define BAR_64BIT_SUPPORT 0x2
#define ADDR_TYPE_MASK    0x6
#define BAR0_OFFSET       0x10
#define BAR2_OFFSET       0x18

static
void
payload (void)
{

  uint32_t index;
  uint32_t count;
  uint32_t data;
  uint32_t dev_type;
  uint32_t dev_bdf;

  index = val_pe_get_index_mpid (val_pe_get_mpid());
  count = val_peripheral_get_info (NUM_ALL, 0);

  if(!count) {
     val_print (AVS_PRINT_WARN, "\n       Skip as No peripherals detected   ", 0);
     val_set_status (index, RESULT_SKIP (g_sbsa_level, TEST_NUM, 1));
     return;
  }

  while (count) {
      count--;
      dev_bdf = (uint32_t)val_peripheral_get_info (ANY_BDF, count);
      dev_type = val_pcie_get_device_type(dev_bdf);
      // 1: Normal PCIe device, 2: PCIe Host bridge, 3: PCIe bridge device, else: INVALID

      if ((!dev_type) || (dev_type > 1)) {
          //Skip this device, if we either got pdev as NULL or if it is a bridge
          continue;
      }

      data = val_pcie_is_devicedma_64bit(dev_bdf);
      if (data == 0) {
          if(!val_pcie_is_device_behind_smmu(dev_bdf)) {
              val_print (AVS_STATUS_ERR, "\n       The device with bdf=0x%x doesn't support 64 bit addressing", dev_bdf);
              val_print (AVS_STATUS_ERR, "\n       and is not behind SMMU", 0);
              val_print (AVS_STATUS_ERR, "\n       The device is of type = %d", dev_type);
              val_set_status (index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 1));
              return;
          }
      }

  }

  val_set_status (index, RESULT_PASS (g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p013_entry (uint32_t num_pe)
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
