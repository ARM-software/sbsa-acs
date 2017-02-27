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
#include "val/include/sbsa_avs_pcie.h"

#define TEST_NUM   (AVS_GIC_TEST_NUM_BASE + 2)
#define TEST_DESC  "If PCIe, then GIC implements ITS  "

static
void
payload()
{

  uint32_t data;
  uint32_t num_ecam = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);

  if (num_ecam != 0) {
      data = val_gic_get_info(GIC_INFO_NUM_ITS);

      if (data == 0) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          return;
      }
  }
  //If PCIe is not present, just Pass the test
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
g002_entry(uint32_t num_pe)
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
