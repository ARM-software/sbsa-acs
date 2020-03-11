/** @file
 * Copyright (c) 2019, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 30)
#define TEST_DESC  "Check Cmd Reg memory space enable "

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
  val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t dsf_bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t bar_data;
  uint32_t test_fails;
  uint64_t bar_base;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  /* Install sync and async handlers to handle exceptions.*/
  val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  branch_to_test = &&exception_return;

  bar_data = 0;
  tbl_index = 0;
  test_fails = 0;

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;

      /*
       * For a Function with type 0 config space header, obtain
       * base address of its Memory mapped BAR. For Function with
       * Type 1 config space header, obtain base address of the
       * downstream function memory mapped BAR. If there is no
       * downstream Function exist, obtain its own BAR address.
       */
      if ((val_pcie_function_header_type(bdf) == TYPE1_HEADER) &&
           (!val_pcie_get_downstream_function(bdf, &dsf_bdf)))
          val_pcie_get_mmio_bar(dsf_bdf, &bar_base);
      else
          val_pcie_get_mmio_bar(bdf, &bar_base);

      /* Skip this function if it doesn't have mmio BAR */
      if (!bar_base)
         continue;

      /* Disable error reporting of this function to the Upstream */
      val_pcie_disable_eru(bdf);

      /*
       * Clear unsupported request detected bit in Device
       * Status Register to clear any pending urd status.
       */
      val_pcie_clear_urd(bdf);

      /*
       * Disable BAR memory space access to cause address
       * decode failures. With memory space aceess disable,
       * all received memory space accesses are handled as
       * Unsupported Requests by the pcie function.
       */
      val_pcie_disable_msa(bdf);

      /* Set test status as FAIL, update to PASS in exception handler */
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));

      /*
       * Read memory mapped BAR to cause unsupported request
       * detected bit set in Device Status Register of the pcie
       * Function. Based on platform configuration, this may
       * even cause an sync/async exception.
       */
      bar_data = val_mmio_read((addr_t)(bar_base));

exception_return:
      /*
       * Check if unsupported request detected bit isn't set
       * and if either of UR response or abort isn't received.
       */
      if ((val_pcie_is_urd(bdf)) &&
          (IS_TEST_PASS(val_get_status(pe_index)) || (bar_data == PCIE_UNKNOWN_RESPONSE)))
      {
          /* Clear urd bit in Device Status Register */
          val_pcie_clear_urd(bdf);
       } else
       {
           val_print(AVS_PRINT_ERR, "\n      BDF %x MSE functionality failure", bdf);
           test_fails++;
       }

       /* Enable memory space access to decode BAR addresses */
       val_pcie_enable_msa(bdf);

       /* Reset the loop variables */
       bar_data = 0;
  }

  if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p030_entry(uint32_t num_pe)
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
