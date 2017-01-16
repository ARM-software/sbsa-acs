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

#include "val/include/sbsa_avs_gic.h"

#define TEST_NUM   (AVS_GIC_TEST_NUM_BASE + 1)
#define TEST_DESC  "Check GIC version                 "

static
void
payload()
{

  uint32_t gic_version;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  val_print(AVS_PRINT_INFO, "\n       Received GIC version = %4d      ", gic_version);

  if (g_sbsa_level < 2) {
      if (gic_version < 2) {
          val_print(AVS_PRINT_ERR, "\n       GIC version is %x                 ", gic_version);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          return;
      }
  } else {
      if (gic_version < 3) {
          val_print(AVS_PRINT_ERR, "\n       GIC version is %3x                ", gic_version);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
          return;
      }

  }
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

}

uint32_t
g001_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This GIC test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);

  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
