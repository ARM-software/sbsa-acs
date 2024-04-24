/** @file
 * Copyright (c) 2016-2018, 2021-2024, Arm Limited or its affiliates. All rights reserved.
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
#include "val/common/include/acs_pe.h"
#include "val/sbsa/include/sbsa_val_interface.h"

#include "val/sbsa/include/sbsa_acs_smmu.h"
#include "val/sbsa/include/sbsa_acs_pe.h"

#define TEST_NUM   (ACS_SMMU_TEST_NUM_BASE + 9)
#define TEST_RULE  "B_SMMU_03"
#define TEST_DESC  "Check SMMU Large VA Support           "

static
void
payload()
{

  uint64_t data_va_range, data_vax;
  uint32_t num_smmu;
  uint32_t index;

  index = val_pe_get_index_mpid(val_pe_get_mpid());

  data_va_range = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64MMFR2_EL1), 16, 19);
  if (data_va_range == 0) {
    val_print(ACS_PRINT_DEBUG, "\n       Large VA Not Supported by PE                        ", 0);
    val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
    return;
  }

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);
  if (num_smmu == 0) {
    val_print(ACS_PRINT_ERR, "\n       No SMMU Controllers are discovered                  ", 0);
    val_set_status(index, RESULT_SKIP(TEST_NUM, 2));
    return;
  }

  while (num_smmu--) {
      if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) == 2) {
          val_print(ACS_PRINT_WARN, "\n       Large VA Not Supported in SMMUv2", 0);
          val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
          return;
      }

      data_vax = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_IDR5, num_smmu), 10, 11);

      /* If PE Supports Large VA Range then SMMU_IDR5.VAX = 0b01 */
      if (data_va_range == 1) {
          if (data_vax != 1) {
              val_print(ACS_PRINT_ERR, "\n       Large VA Not Supported in SMMU %x", num_smmu);
              val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
              return;
          }
      }
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
}

uint32_t
i009_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
