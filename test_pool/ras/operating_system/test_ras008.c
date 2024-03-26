/** @file
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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

#include "val/common/include/acs_val.h"
#include "val/common/include/acs_pe.h"
#include "val/sbsa/include/sbsa_val_interface.h"
#include "val/sbsa/include/sbsa_acs_memory.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_ras.h"
#include "val/common/include/acs_peripherals.h"
#include "val/common/include/acs_common.h"

#define TEST_NUM   (ACS_RAS_TEST_NUM_BASE + 8)
#define TEST_RULE  "RAS_11, RAS_12"
#define TEST_DESC  "Software Fault Error Check        "

/* The generic peripherals which ACS can rely on like PCIe spec it ruled out as the
   rule mentions to follow PCIe specification mentioned behaviour for handling those
   errors.
   The most generic address access which can be used will be UART space, In the PL011
   there are some unused address space and the UART specification is mandating the
   response for access to those space.

   As part of this rule we can make sure data abort if not generated as part of access
   to the unused address space.
*/

static uint64_t branch_to_test;
static uint64_t l_uart_base;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to point to next instrcution */
  val_pe_update_elr(context, branch_to_test);

  val_print(ACS_PRINT_ERR, "\n       Error : Received Sync Exception type %d", interrupt_type);
  val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
}

static
void
payload(void)
{

  uint32_t count = val_peripheral_get_info(NUM_UART, 0);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t value;

  val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);

  branch_to_test = (uint64_t)&&exception_taken;

  if (count == 0) {
      val_print(ACS_PRINT_WARN, "\n       No UART defined by Platform      ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  while (count != 0) {
      l_uart_base = val_peripheral_get_info(UART_BASE0, count - 1);
      if (l_uart_base == 0) {
          val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
          return;
      }

      /*Make sure access to Reserved doesn't cause any exceptions*/
      value = *((volatile uint32_t *)(l_uart_base + UART_RES));
      val_print(ACS_PRINT_DEBUG, "\n       Value from UART Reserved Space 0x%llx", value);

      *((volatile uint32_t *)(l_uart_base + UART_RES)) = (uint32_t)(0xDEAD);

      val_set_status(index, RESULT_PASS(TEST_NUM, 01));

      count--;
  }
exception_taken:
  return;
}

uint32_t
ras008_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
