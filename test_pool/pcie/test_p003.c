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

#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 3)
#define TEST_DESC  "Check ECAM Memory accessibility   "

static void *branch_to_test;
uint64_t stack_pointer, ret_addr;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(AVS_PRINT_ERR, "\n      Exception occured while accessing ECAM address", 0);
  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
}

static
void
payload(void)
{

  uint64_t data;
  uint32_t num_ecam;
  uint64_t ecam_base;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t bdf = 0;
  uint32_t bus, segment;

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
  //Context save in case an exception occurs while accessing ECAM base
  stack_pointer = AA64ReadSp();
  ret_addr =  *(uint64_t *)(stack_pointer+8);

  while (num_ecam) {
      num_ecam--;
      segment = val_pcie_get_info(PCIE_INFO_SEGMENT, num_ecam);
      bus = val_pcie_get_info(PCIE_INFO_START_BUS, num_ecam);

      /* Install both sync and async handlers, so that the test could handle
         either of these exceptions.*/
      val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
      val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
      branch_to_test = &&exception_label;

      bdf = PCIE_CREATE_BDF(segment, bus, 0, 0);
      data = val_pcie_read_cfg(bdf, 0);

      //If this is really PCIe CFG space, Device ID and Vendor ID cannot be 0 or 0xFFFF
      if ((data == 0) || ((data & 0xFFFF) == 0xFFFF)) {
          val_print(AVS_PRINT_ERR, "\n      Incorrect data at ECAM Base %4x    ", data);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
          return;
      }

      data = val_pcie_read_cfg(bdf, 0xC);

      //If this really is PCIe CFG, Header type[6:0] must be 01 or 00
      if (((data >> 16) & 0x7F) > 01) {
          val_print(AVS_PRINT_ERR, "\n      Incorrect PCIe CFG Hdr type %4x    ", data);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
          return;
      }
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  return;

exception_label:
  //Context restore
  AA64WriteSp(stack_pointer);
  *(uint64_t *)(stack_pointer+8) = ret_addr;
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
