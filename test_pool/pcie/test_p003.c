/** @file
 * Copyright (c) 2016-2019, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 3)
#define TEST_DESC  "Check ECAM Memory accessibility   "

#define PCIE_VENDOR_ID_REG_OFFSET 0x0
#define PCIE_CACHE_LINE_SIZE_REG_OFFSET 0xC


static
void
payload(void)
{

  uint32_t data;
  uint32_t num_ecam;
  uint64_t ecam_base;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t bdf = 0;
  uint32_t bus, segment;
  uint32_t end_bus;
  uint32_t bus_index;
  uint32_t dev_index;
  uint32_t ret;

  num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);

  if (num_ecam == 0) {
      val_print(AVS_PRINT_ERR, "\n       No ECAM in MCFG                   ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  ecam_base = val_pcie_get_info(PCIE_INFO_MCFG_ECAM, 0);

  if (ecam_base == 0) {
      val_print(AVS_PRINT_ERR, "\n       ECAM Base in MCFG is 0            ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  while (num_ecam) {
      num_ecam--;
      segment = val_pcie_get_info(PCIE_INFO_SEGMENT, num_ecam);
      bus = val_pcie_get_info(PCIE_INFO_START_BUS, num_ecam);
      end_bus = val_pcie_get_info(PCIE_INFO_END_BUS, num_ecam);

      bdf = PCIE_CREATE_BDF(segment, bus, 0, 0);
      ret = val_pcie_read_cfg(bdf, PCIE_VENDOR_ID_REG_OFFSET, &data);
      if (data == 0xFFFFFFFF) {
          val_print(AVS_PRINT_ERR, "\n      First device in a ECAM space is not a valid device", 0);
           val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, (bus << 8)));
           return;
      }

      for(bus_index = bus; bus_index <= end_bus; bus_index++) {
        for(dev_index = 0; dev_index < PCIE_MAX_DEV; dev_index++) {

           /* Only function zero is checked */
           bdf = PCIE_CREATE_BDF(segment, bus_index, dev_index, 0);
           ret = val_pcie_read_cfg(bdf, PCIE_VENDOR_ID_REG_OFFSET, &data);

           //If this is really PCIe CFG space, Device ID and Vendor ID cannot be 0 or 0xFFFF
           if (ret == PCIE_READ_ERR || (data == 0) || ((data != 0xFFFFFFFF) && ((data & 0xFFFF) == 0xFFFF))) {
              val_print(AVS_PRINT_ERR, "\n      Incorrect data at ECAM Base %4x    ", data);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, (bus_index << 8)|dev_index));
              return;
           }

           ret = val_pcie_read_cfg(bdf, PCIE_CACHE_LINE_SIZE_REG_OFFSET, &data);

           //If this really is PCIe CFG, Header type[6:0] must be 01 or 00
           if (ret == PCIE_READ_ERR || ((data != 0xFFFFFFFF) && (((data >> 16) & 0x7F) > 01))) {
              val_print(AVS_PRINT_ERR, "\n      Incorrect PCIe CFG Hdr type %4x    ", data);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, (bus_index << 8)|dev_index));
              return;
           }
        }
      }
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

}

uint32_t
p003_entry(uint32_t num_pe)
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
