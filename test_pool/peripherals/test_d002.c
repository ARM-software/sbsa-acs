/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

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

#include "val/include/sbsa_avs_peripherals.h"
#include "val/include/sbsa_avs_pcie.h"

#define TEST_NUM   (AVS_PER_TEST_NUM_BASE + 2)
#define TEST_DESC  "Check SATA CTRL Interface via PCIe"

static
void
payload()
{

  uint32_t interface = 0;
  uint32_t bdf;
  uint32_t ret;
  uint32_t count = val_peripheral_get_info(NUM_SATA, 0);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  if (count == 0) {
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  while (count != 0) {
      bdf = val_peripheral_get_info(SATA_BDF, count - 1);
      ret = val_pcie_read_cfg(bdf, 0x8, &interface);
      interface = (interface >> 8) & 0xFF;
      if (ret == PCIE_READ_ERR || interface != 0x01) {
          val_print(AVS_PRINT_WARN, "\n       WARN: SATA CTRL ECAM access failed %x  ", interface);
          val_print(AVS_PRINT_WARN, "\n       Re-checking SATA CTRL using PciIo protocol       ", 0);
          ret = val_pcie_io_read_cfg(bdf, 0x8, &interface);
          if (ret == PCIE_READ_ERR) {
              val_print(AVS_PRINT_ERR, "\n       Reading device class code using PciIo protocol failed ", 0);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
              return;
          }
          interface = (interface >> 8) & 0xFF;
          if (interface != 0x01) {
              val_print(AVS_PRINT_ERR, "\n Detected SATA CTRL not AHCI        ", 0);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
              return;
          }
      }
      count--;
  }
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  return;
}

/**
  @brief     Read PCI CFG space class and sub-class register
            to determine the SATA interface version
**/
uint32_t
d002_entry(uint32_t num_pe)
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
