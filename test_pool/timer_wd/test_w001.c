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

#define TEST_NUM   (AVS_WD_TEST_NUM_BASE + 1)
#define TEST_DESC  "Check NS Watchdog Accessibility   "

static
void
payload()
{

  uint64_t ctrl_base;
  uint64_t refresh_base;
  uint64_t wd_num = val_wd_get_info(0, WD_INFO_COUNT);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t data, ns_wdg = 0;

  val_print(AVS_PRINT_DEBUG, "\n       Found %d watchdogs in ACPI table ", wd_num);

  if (wd_num == 0) {
      val_print(AVS_PRINT_WARN, "\n       No Watchdogs reported          %d  ", wd_num);
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  do {
      wd_num--; //array index starts from 0, so subtract 1 from count

      if (val_wd_get_info(wd_num, WD_INFO_ISSECURE))
          continue;    //Skip Secure watchdog

      ns_wdg++;
      refresh_base = val_wd_get_info(wd_num, WD_INFO_REFRESH_BASE);
      val_print(AVS_PRINT_INFO, "\n      Watchdog Refresh base is %x ", refresh_base);
      ctrl_base    = val_wd_get_info(wd_num, WD_INFO_CTRL_BASE);
      val_print(AVS_PRINT_INFO, "\n      Watchdog CTRL base is  %x      ", ctrl_base);

      data = val_mmio_read(ctrl_base);
      //Control register bits 31:4 are reserved 0
      if(data >> 4) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
          return;
      }

      data = val_mmio_read(refresh_base);
      //refresh frame offset 0 must return 0 on reads.
      if(data) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
          return;
      }
  } while(wd_num);

  if(!ns_wdg) {
      val_print(AVS_PRINT_WARN, "\n       No non-secure Watchdogs reported", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
      return;
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

}

uint32_t
w001_entry(uint32_t num_pe)
{

  uint32_t error_flag = 0;
  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);

  val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  error_flag = val_check_for_error(TEST_NUM, num_pe);

  if (!error_flag)
      status = AVS_STATUS_PASS;

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));
  return status;

}
