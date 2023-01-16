/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
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
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM   (AVS_GIC_TEST_NUM_BASE + 2)
#define TEST_RULE  "S_L5PP_01"
#define TEST_DESC  "Check Reserved PPI Assignments    "

/* PPI IDs 1056-1071 and 1088-1103 are reserved for future SBSA usage */
#define IS_PPI_RESERVED(id) ((id > 1055 && id < 1072) || (id > 1087 && id < 1104))

static
void
payload()
{

  uint32_t intid;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  if (g_sbsa_level < 5) {
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  /* Check non-secure physical timer interrupt */
  intid = val_timer_get_info(TIMER_INFO_PHY_EL1_INTID, 0);
  if (IS_PPI_RESERVED(intid))
  {
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
      val_print(AVS_PRINT_ERR, "\n       Interrupt ID is reserved for future SBSA usage ", 0);
      return;
  }

  /* Check non-secure virtual timer interrupt */
  intid = val_timer_get_info(TIMER_INFO_VIR_EL1_INTID, 0);
  if (IS_PPI_RESERVED(intid))
  {
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
      val_print(AVS_PRINT_ERR, "\n       Interrupt ID is reserved for future SBSA usage ", 0);
      return;
  }

  /* Check for EL2 virtual timer interrupt  */
  intid = val_timer_get_info(TIMER_INFO_VIR_EL2_INTID, 0);
  if (IS_PPI_RESERVED(intid))
  {
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
      val_print(AVS_PRINT_ERR, "\n       Interrupt ID is reserved for future SBSA usage ", 0);
      return;
  }

  /* Check non-secure EL2 physical timer interrupt */
  intid = val_timer_get_info(TIMER_INFO_PHY_EL2_INTID, 0);
  if (IS_PPI_RESERVED(intid))
  {
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 04));
      val_print(AVS_PRINT_ERR, "\n       Interrupt ID is reserved for future SBSA usage ", 0);
      return;
  }

  /* Check  GIC maintenance interrupt */
  intid = val_pe_get_gmain_gsiv(index);
  if (IS_PPI_RESERVED(intid))
  {
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 05));
      val_print(AVS_PRINT_ERR, "\n       Interrupt ID is reserved for future SBSA usage ", 0);
      return;
  }

  /* Check  Performance monitor interrupt */
  intid = val_pe_get_pmu_gsiv(index);
  if (IS_PPI_RESERVED(intid))
  {
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 06));
      val_print(AVS_PRINT_ERR, "\n       Interrupt ID is reserved for future SBSA usage ", 0);
      return;
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

}

uint32_t
g002_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This GIC test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);

  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
