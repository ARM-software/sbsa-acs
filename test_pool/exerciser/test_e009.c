/** @file
 * Copyright (c) 2019-2023, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_pcie_enumeration.h"
#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_smmu.h"
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_exerciser.h"

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 9)
#define TEST_DESC  "Check RP Sec Bus transactions are TYPE0"
#define TEST_RULE  "PCI_IN_11"

static
void
payload(void)
{

  uint32_t pe_index;
  uint32_t e_bdf;
  uint32_t erp_bdf;
  uint32_t reg_value;
  uint32_t instance;
  uint32_t fail_cnt;
  uint32_t status;
  uint64_t header_type;

  fail_cnt = 0;
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  while (instance-- != 0)
  {

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

      e_bdf = val_exerciser_get_bdf(instance);

      /* Check if exerciser is child of one of the rootports */
      if (val_pcie_parent_is_rootport(e_bdf, &erp_bdf))
          continue;
      /*
       * Generate a config request from PE to the Secondary bus
       * of the exerciser's root port. Exerciser should see this
       * request as a Type 0 Request.
       */
      status = val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance);
      if (status == PCIE_CAP_NOT_FOUND)
      {
          val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
          return;
      }

      val_pcie_read_cfg(e_bdf, TYPE01_VIDR, &reg_value);
      status = val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);
      if (status == PCIE_CAP_NOT_FOUND)
      {
          val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
          return;
      }

      val_exerciser_get_param(CFG_TXN_ATTRIBUTES, (uint64_t *)&header_type, 0, instance);
      if (header_type != TYPE0)
      {
          val_print(AVS_PRINT_ERR, "\n       BDF 0x%x Sec Bus Transaction failure", erp_bdf);
          fail_cnt++;
      }
  }

  if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

  return;

}

uint32_t
e009_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
