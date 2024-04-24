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
#include "val/sbsa/include/sbsa_acs_pcie.h"
#include "val/sbsa/include/sbsa_acs_pe.h"

#define TEST_NUM   (ACS_SMMU_TEST_NUM_BASE + 12)
#define TEST_RULE  "B_SMMU_14"
#define TEST_DESC  "Check SMMU Endianess Support          "

static
void
payload()
{

  uint64_t data;
  uint64_t data_pe_endian = 0;
  uint32_t num_smmu;
  uint32_t index;

  index = val_pe_get_index_mpid(val_pe_get_mpid());

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);

  if (num_smmu == 0) {
    val_print(ACS_PRINT_ERR, "\n       No SMMU Controllers are discovered                  ", 0);
    val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
    return;
  }

  /* Check the current endianness setting of SCTLR.EE */
  if (val_pe_reg_read(CurrentEL) == AARCH64_EL2) {
      data_pe_endian = VAL_EXTRACT_BITS(val_pe_reg_read(SCTLR_EL2), 25, 25);
  } else if (val_pe_reg_read(CurrentEL) == AARCH64_EL1) {
      data_pe_endian = VAL_EXTRACT_BITS(val_pe_reg_read(SCTLR_EL1), 25, 25);
  }

  while (num_smmu--) {
      if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) == 2) {
          val_print(ACS_PRINT_WARN, "\n       Not valid for SMMU v2           ", 0);
          val_set_status(index, RESULT_SKIP(TEST_NUM, 2));
          return;
      }

      data = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_IDR0, num_smmu), 21, 22);

      if ((data_pe_endian == 1) && ((data == 1) || (data == 2))) {
          /* If PE supports big endian */
          val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
          val_print(ACS_PRINT_ERR, "\n       PE supports big endian, "
                                   "but SMMU %x does not", num_smmu);
          return;
      } else if ((data_pe_endian == 0) && ((data == 1) || (data == 3))) {
          /* If PE supports little endian */
          val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
          val_print(ACS_PRINT_ERR, "\n       PE supports little endian, "
                                   "but SMMU %x does not", num_smmu);
          return;
      }
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
}

uint32_t
i012_entry(uint32_t num_pe)
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
