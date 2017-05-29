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

#include "val/include/sbsa_avs_wd.h"
#include "val/include/sbsa_avs_secure.h"
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM   (AVS_SECURE_TEST_NUM_BASE + 2)
#define TEST_DESC  "Check System Genric Counter       "

static void *branch_to_test;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(AVS_PRINT_INFO, "\n       Received exception           ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

static
void
payload()
{

  uint32_t timeout = 2;
  uint64_t data = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  SBSA_SMC_t  smc;

  if (!val_timer_get_info(TIMER_INFO_NUM_PLATFORM_TIMERS, 0)) {
      val_print(AVS_PRINT_WARN, "\n       No System timers implemented      ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  if (!val_is_el3_enabled())
  {
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
      return;
  }

  smc.test_index = SBSA_SECURE_TEST_SYS_COUNTER;

  val_secure_call_smc(&smc);

  switch (val_secure_get_result(&smc, timeout))
  {
      case AVS_STATUS_PASS:
          val_print(AVS_PRINT_DEBUG, "\n       Secure CNT base is   0x%x   ", smc.test_arg02);
          val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
          break;
      case AVS_STATUS_SKIP:
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
          break;
      case AVS_STATUS_FAIL:
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          break;
      default:
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
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

  if (data == smc.test_arg03)
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
  else
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 02));

}

uint32_t
s002_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
