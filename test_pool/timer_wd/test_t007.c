/** @file
 * Copyright (c) 2016-2018,2021-2022 Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_timer.h"
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM   (AVS_TIMER_TEST_NUM_BASE + 7)
#define TEST_DESC  "CNTCTLBase & CNTBaseN access      "

#define ARBIT_VALUE 0xA000

static
void
payload(void)
{

  uint64_t cnt_ctl_base, cnt_base_n;
  uint32_t data;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t timer_num = val_timer_get_info(TIMER_INFO_NUM_PLATFORM_TIMERS, 0);
  uint64_t data1;

  if (!timer_num) {
      val_print(AVS_PRINT_WARN, "\n       No System timers are defined      ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 0x1));
      return;
  }

  while(timer_num){
      --timer_num;  //array index starts from 0, so subtract 1 from count

      if (val_timer_get_info(TIMER_INFO_IS_PLATFORM_TIMER_SECURE, timer_num))
          continue;    //Skip Secure Timer

      cnt_ctl_base = val_timer_get_info(TIMER_INFO_SYS_CNTL_BASE, timer_num);
      cnt_base_n   = val_timer_get_info(TIMER_INFO_SYS_CNT_BASE_N, timer_num);

      if (cnt_ctl_base == 0) {
          val_print(AVS_PRINT_WARN, "\n       CNTCTL BASE_N is zero             ", 0);
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 0x2));
          return;
      }

      data = val_mmio_read(cnt_ctl_base + 0x8);
      val_mmio_write(cnt_ctl_base + 0x8, 0xFFFFFFFF);
      if(data != val_mmio_read(cnt_ctl_base + 0x8)) {
          val_print(AVS_PRINT_ERR, "\n       Read-write check failed for CNTCTLBase.CNTTIDR", 0);
          val_print(AVS_PRINT_ERR, ", expected value %x ", data);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 1));
          return;
      }

      if (cnt_base_n == 0) {
          val_print(AVS_PRINT_WARN, "\n       CNT_BASE_N is zero                 ", 0);
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 0x3));
          return;
      }

      // Read CNTPCT register
      data1 = val_mmio_read64(cnt_base_n + 0x0);
      val_print(AVS_PRINT_DEBUG, "\n       CNTPCT Read value = 0x%llx       ", data1);

      // Writes to Read-Only registers should be ignore
      val_mmio_write64(cnt_base_n + 0x0, (data1 - ARBIT_VALUE));

      if (val_mmio_read64(cnt_base_n + 0x0) < data1) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 2));
          val_print(AVS_PRINT_ERR, "\n       CNTBaseN: CNTPCT reg should be read-only ", 0);
          return;
      }

      // Read CNTVCT register
      data1 = val_mmio_read64(cnt_base_n + 0x8);
      val_print(AVS_PRINT_DEBUG, "\n       CNTVCT Read value = 0x%llx       ", data1);

      // Writes to Read-Only registers should be ignore
      val_mmio_write64(cnt_base_n + 0x8, (data1 - ARBIT_VALUE));

      if (val_mmio_read64(cnt_base_n + 0x8) < data1) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 3));
          val_print(AVS_PRINT_ERR, "\n       CNTBaseN: CNTVCT reg should be read-only ", 0);
          return;
      }

      // Read CNTFRQ register
      data = val_mmio_read(cnt_base_n + 0x10);
      val_print(AVS_PRINT_DEBUG, "\n       CNTFRQ Read value = 0x%x         ",
                data);

      // Writes to Read-Only registers should be ignore
      val_mmio_write(cnt_base_n + 0x10, (data - ARBIT_VALUE));

      if (val_mmio_read(cnt_base_n + 0x10) != data) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 4));
          val_print(AVS_PRINT_ERR, "\n       CNTBaseN: CNTFRQ reg should be read-only ", 0);
          return;
      }

      data = 0x3;
      val_mmio_write(cnt_base_n + 0x2C, data);
      if(data != (val_mmio_read(cnt_base_n + 0x2C) & 0x3)) {
          val_print(AVS_PRINT_ERR, "\n       Read-write check failed for CNTBaseN.CNTP_CTL", 0);
          val_print(AVS_PRINT_ERR, ", expected value %x ", data);
          val_print(AVS_PRINT_ERR, "\n       Read value %x ", val_mmio_read(cnt_base_n + 0x2C));
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 0x5));
          val_mmio_write(cnt_base_n + 0x2C, 0x0); // Disable the timer before return
          return;
      }
      val_mmio_write(cnt_base_n + 0x2C, 0x0); // Disable timer

      data1 = 0xFF00FF00FF00FF00;
      /* Write a random value to CNTP_CVAL*/
      val_mmio_write64(cnt_base_n + 0x20, data1);

      if (data1 != val_mmio_read64(cnt_base_n + 0x20)) {
          val_print(AVS_PRINT_ERR, "\n       Read-write check failed for "
              "CNTBaseN.CNTP_CVAL, read value %llx ",
              val_mmio_read64(cnt_base_n + 0x20));
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 6));
          return;
      }

      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  }

}

uint32_t
t007_entry(uint32_t num_pe)
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
