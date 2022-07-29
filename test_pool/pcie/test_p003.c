/** @file
 * Copyright (c) 2016-2021, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 3)
#define TEST_DESC  "Check ECAM Memory accessibility   "

static void *branch_to_test;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t pe_index;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(AVS_PRINT_INFO, "\n       Received exception of type: %d", interrupt_type);
  val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
}

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
  uint32_t func_index;
  uint32_t ret;
  uint32_t next_offset = 0;
  uint32_t curr_offset = 0;
  uint32_t status;

  /* Install sync and async handlers to handle exceptions.*/
  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status)
  {
      val_print(AVS_PRINT_ERR, "\n      Failed in installing the exception handler", 0);
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
      return;
  }
  branch_to_test = &&exception_return;

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

      /* Accessing the BDF PCIe config range */
      for (bus_index = bus; bus_index <= end_bus; bus_index++) {
        for (dev_index = 0; dev_index < PCIE_MAX_DEV; dev_index++) {
          for (func_index = 0; func_index < PCIE_MAX_FUNC; func_index++) {

               bdf = PCIE_CREATE_BDF(segment, bus_index, dev_index, func_index);
               ret = val_pcie_read_cfg(bdf, TYPE01_VIDR, &data);

               //If this is really PCIe CFG space, Device ID and Vendor ID cannot be 0
               if (ret == PCIE_NO_MAPPING || (data == 0)) {
                  val_print(AVS_PRINT_ERR, "\n      Incorrect data at ECAM Base %4x    ", data);
                  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM,
                                  (bus_index << PCIE_BUS_SHIFT)|dev_index));
                  return;
               }

               /* Access the config space, if device ID and vendor ID are valid */
               if (data != PCIE_UNKNOWN_RESPONSE)
               {
                  if (val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS,  &data) != PCIE_SUCCESS)
                      continue;

                  /* Read till the last capability in Extended Capability Structure */
                  next_offset = PCIE_ECAP_START;
                  while (next_offset)
                  {
                     val_pcie_read_cfg(bdf, next_offset, &data);
                     curr_offset = next_offset;
                     next_offset = ((data >> PCIE_ECAP_NCPR_SHIFT) & PCIE_ECAP_NCPR_MASK);
                  }

                  /* Read the start and end from the end of last valid capability register */
                  val_pcie_read_cfg(bdf, curr_offset, &data);
                  val_pcie_read_cfg(bdf, PCIE_ECAP_END, &data);
               }

               /* Access the start and end of the config space for PCIe devices whose
                  device ID and vendor ID are all FF's */
               else{
                  val_pcie_read_cfg(bdf, PCIE_ECAP_START, &data);

                  /* Returned data should be FF's, otherwise the test should fail */
                  if (data != PCIE_UNKNOWN_RESPONSE) {
                     val_print(AVS_PRINT_ERR, "\n      Incorrect data for Bdf 0x%x    ", bdf);
                     val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM,
                                     (bus_index << PCIE_BUS_SHIFT)|dev_index));
                     return;
                  }

                  val_pcie_read_cfg(bdf, PCIE_ECAP_END, &data);

                  /* Returned data should be FF's, otherwise the test should fail */
                  if (data != PCIE_UNKNOWN_RESPONSE) {
                     val_print(AVS_PRINT_ERR, "\n      Incorrect data for Bdf 0x%x    ", bdf);
                     val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM,
                                     (bus_index << PCIE_BUS_SHIFT)|dev_index));
                     return;
                  }

               }
          }
        }
      }
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

exception_return:
  return;
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
