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

#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_peripherals.h"
#include "val/include/sbsa_avs_secure.h"

typedef struct {
  uint32_t  test_num;
  uint64_t  test_index;
  char      test_desc[42];
  uint64_t  arg1;
  uint64_t  arg2;
  uint64_t  return_arg1;
}secure_test_list;

secure_test_list list[] = {
    {(AVS_SECURE_TEST_NUM_BASE + 3), SBSA_SECURE_TEST_EL3_PHY,
                            "Check EL1-S PE timer interrupt    ", 0, 0, 0},
    {(AVS_SECURE_TEST_NUM_BASE + 4), SBSA_SECURE_TEST_WD_WS0,
                            "Check Secure Watchdog WS0 intr    ", 0, 0, 0},
    {(AVS_SECURE_TEST_NUM_BASE + 5), SBSA_SECURE_TEST_UART,
                            "Check Secure UART Access          ", 0, 0, 0},
    {(AVS_SECURE_TEST_NUM_BASE + 6), SBSA_SECURE_TEST_WAKEUP,
                            "Check Wakeup from Secure timer    ", 0, 0, 0},
    {(AVS_SECURE_TEST_NUM_BASE + 7), SBSA_SECURE_TEST_SYS_TIMER_INT,
                            "System Wakeup Timer interrupt     ", 0, 0, 0},
    {(AVS_SECURE_TEST_NUM_BASE + 8), SBSA_SECURE_TEST_FINISH,
                            "Last entry                        ", 0, 0, 0}
};

static
void
start_secure_tests(uint32_t num_pe)
{
  uint32_t   index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t   i = 0;
  uint32_t   timeout = 3;
  uint32_t   status;
  SBSA_SMC_t smc;

  while (list[i].test_index != SBSA_SECURE_TEST_FINISH)
  {
      smc.test_index = list[i].test_index;
      smc.test_arg01 = list[i].arg1;
      smc.test_arg02 = list[i].arg2;

      status  = val_initialize_test(list[i].test_num, list[i].test_desc, num_pe, g_sbsa_level);
      if (status != AVS_STATUS_SKIP)
          val_secure_call_smc(&smc);


      switch (val_secure_get_result(&smc, timeout))
      {
      case AVS_STATUS_PASS:
          val_set_status(index, RESULT_PASS(g_sbsa_level, list[i].test_num, i));
          break;
      case AVS_STATUS_SKIP:
          val_set_status(index, RESULT_SKIP(g_sbsa_level, list[i].test_num, i));
          break;
      case AVS_STATUS_FAIL:
          val_set_status(index, RESULT_FAIL(g_sbsa_level, list[i].test_num, i));
          break;
      default:
          val_print(AVS_PRINT_ERR, "\n       Unexpected SMC result      ", 0);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, list[i].test_num, i));
      }

      /* get the result from all PE and check for failure */
      val_check_for_error(list[i].test_num, num_pe);

      i++;
  }
  val_report_status(0, SBSA_AVS_END(g_sbsa_level, list[i].test_num));

}

uint32_t
s003_entry(uint32_t num_pe)
{

  num_pe = 1;

  start_secure_tests(num_pe);

  return 0;

}
