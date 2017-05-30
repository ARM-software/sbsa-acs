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
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_secure.h"

#define TEST_NUM   (AVS_PER_TEST_NUM_BASE + 6)
#define TEST_DESC  "Non secure Access to Secure addr  "

static void *branch_to_test;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t syndrome;

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  syndrome = val_pe_reg_read(ESR_EL2);
  syndrome &= 0x3F;    // Get the DFSC field from ESR
  if((syndrome >= 4) && (syndrome <= 7)){
    val_print(AVS_PRINT_DEBUG, "\n       The fault reported is translation fault, the address accessed needs to be mapped", 0);
    val_print(AVS_PRINT_DEBUG, "\n       DFSC = 0x%x", syndrome);
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  val_print(AVS_PRINT_INFO, "\n       Received DAbort Exception ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

static
void
payload()
{
  uint32_t timeout = 2, i;
  SBSA_SMC_t  smc;
  uint64_t data;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  for(i = 0 ; i < 4 ; i++){
      data = 0xDEED;
      smc.test_index = SBSA_SECURE_PLATFORM_ADDRESS;
      smc.test_arg01 = i;   // arg01 is used to select one of the 4 addresses
      val_secure_call_smc(&smc);

      switch (val_secure_get_result(&smc, timeout))
      {
          case AVS_STATUS_PASS:
              val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
              val_print(AVS_PRINT_DEBUG, "\n       Secure platform address is 0x%lx ", smc.test_arg02);
              val_print(AVS_PRINT_DEBUG, "\n       Value at secure platform address is 0x%x ", smc.test_arg03);
              break;
          case AVS_STATUS_SKIP:
              val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
              return;
          case AVS_STATUS_FAIL:
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
              val_print(AVS_PRINT_DEBUG, "\n       Failed for iteration value i = %d", i);
              return;
          default:
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
              return;
      }

      /* Install both sync and async handlers, so that the test could handle
         either of these exceptions.*/
      val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
      val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);

      branch_to_test = &&exception_taken;
      data = *(uint64_t *)smc.test_arg02;

exception_taken:
      if(IS_TEST_FAIL(val_get_status(index)))
          return;

      if (data == smc.test_arg03){
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
          return;
      }
      else
          val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 02));
  }

}

uint32_t
m002_entry(uint32_t num_pe)
{

  uint32_t error_flag = 0;
  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, val_pe_get_num(), g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  error_flag = val_check_for_error(TEST_NUM, num_pe);

  if (!error_flag)
      status = AVS_STATUS_PASS;
  else
      status = AVS_STATUS_FAIL;

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}

