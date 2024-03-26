/** @file
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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

#include "val/common/include/acs_val.h"
#include "val/sbsa/include/sbsa_val_interface.h"
#include "val/sbsa/include/sbsa_acs_iovirt.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/common/include/acs_iovirt.h"

#define TEST_NUM   (ACS_SMMU_TEST_NUM_BASE + 15)
#define TEST_RULE  "S_L7SM_01"
#define TEST_DESC  "Check if all DMA reqs behind SMMU "

static
void
payload()
{

  uint32_t num_pcie_rc, num_named_comp;
  uint32_t i, test_fails = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_dma_req = 0;

  if (g_sbsa_level < 7) {
      val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  /* check whether all DMA capable PCIe root complexes are behind a SMMU */
  num_pcie_rc = val_iovirt_get_pcie_rc_info(NUM_PCIE_RC, 0);
  num_dma_req = num_pcie_rc;
  for (i = 0; i < num_pcie_rc; i++) {
      /* print info fields */
      val_print(ACS_PRINT_DEBUG, "\n       RC segment no  : 0x%llx",
                    val_iovirt_get_pcie_rc_info(RC_SEGMENT_NUM, i));
      val_print(ACS_PRINT_DEBUG, "\n       CCA attribute  : 0x%x",
                    val_iovirt_get_pcie_rc_info(RC_MEM_ATTRIBUTE, i));
      val_print(ACS_PRINT_DEBUG, "\n       SMMU base addr : 0x%llx\n",
                    val_iovirt_get_pcie_rc_info(RC_SMMU_BASE, i));

      if (val_iovirt_get_pcie_rc_info(RC_MEM_ATTRIBUTE, i) == 0x1 &&
                                    val_iovirt_get_pcie_rc_info(RC_SMMU_BASE, i) == 0) {
          val_print(ACS_PRINT_ERR,
                    "\n       DMA capable PCIe root port with segment no: %llx not behind a SMMU.",
                    val_iovirt_get_pcie_rc_info(RC_SEGMENT_NUM, i));
          test_fails++;
      }
  }

  /* check whether all DMA capable Named component requestors are behind a SMMU */
  num_named_comp = val_iovirt_get_named_comp_info(NUM_NAMED_COMP, 0);
  num_dma_req += num_named_comp;
  for (i = 0; i < num_named_comp; i++) {
      /* print info fields */
      val_print(ACS_PRINT_DEBUG, "\n       Named component  :", 0);
      val_print(ACS_PRINT_DEBUG,
                    (char8_t *)val_iovirt_get_named_comp_info(NAMED_COMP_DEV_OBJ_NAME, i), 0);
      val_print(ACS_PRINT_DEBUG, "\n       CCA attribute    : 0x%x",
                    val_iovirt_get_named_comp_info(NAMED_COMP_CCA_ATTR, i));
      val_print(ACS_PRINT_DEBUG, "\n       SMMU base addr   : 0x%llx\n",
                    val_iovirt_get_named_comp_info(NAMED_COMP_SMMU_BASE, i));
      if (val_iovirt_get_named_comp_info(NAMED_COMP_CCA_ATTR, i) == 0x1 &&
                                    val_iovirt_get_named_comp_info(NAMED_COMP_SMMU_BASE, i) == 0) {
          val_print(ACS_PRINT_ERR,
                    "\n       DMA capable named component with namespace path: ", 0);
          val_print(ACS_PRINT_ERR,
                    (char8_t *)val_iovirt_get_named_comp_info(NAMED_COMP_DEV_OBJ_NAME, i), 0);
          val_print(ACS_PRINT_ERR, " not behind a SMMU.", 0);
          test_fails++;
      }
  }

  if (test_fails)
      val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
  else if (!num_dma_req) {
      val_print(ACS_PRINT_DEBUG, "\n       No DMA requestors present", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
  } else {
      val_set_status(index, RESULT_PASS(TEST_NUM, 01));
  }
}

uint32_t
i015_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  /* This test is run on single processor */

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
