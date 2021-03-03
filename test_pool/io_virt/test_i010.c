/** @file
 * Copyright (c) 2020, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_smmu.h"
#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM   (AVS_SMMU_TEST_NUM_BASE + 10)
#define TEST_DESC  "Check SMMU Granule Support        "

static
void
payload()
{

  uint64_t data;
  uint64_t data_pe_mmfr0, data_smmu_idr5;
  uint32_t is_gran4k = 0;
  uint32_t is_gran64k = 0;
  uint32_t num_smmu;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  data = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);

  if (data == 0) {
      val_print(AVS_PRINT_WARN, "\n       PCIe Subsystem not  discovered   ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
      return;
  }

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);

  if (num_smmu == 0) {
      val_print(AVS_PRINT_ERR, "\n       No SMMU Controllers are discovered ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 03));
      return;
  }

  data_pe_mmfr0 = val_pe_reg_read(ID_AA64MMFR0_EL1);

  while (num_smmu--) {
      if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) == 2) {
          val_print(AVS_PRINT_WARN, "\n       Not valid for SMMU v2           ", 0);
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 04));
          return;
      }

      data_smmu_idr5 = val_smmu_read_cfg(SMMUv3_IDR5, num_smmu);

      /* If PE Supports 4KB Gran, (TGran4 == 0) or (TGran4_2 == 0x2) */
      if ((VAL_EXTRACT_BITS(data_pe_mmfr0, 28, 31) == 0x0) ||
          (VAL_EXTRACT_BITS(data_pe_mmfr0, 40, 43) == 0x2)) {
          is_gran4k = 1;
          if (VAL_EXTRACT_BITS(data_smmu_idr5, 4, 4) != 1) {
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
              return;
          }
      }

      /* If PE Supports 16KB Gran, (TGran16 == 1) or (TGran16_2 == 0x2) */
      if ((VAL_EXTRACT_BITS(data_pe_mmfr0, 20, 23) == 0x1) ||
          (VAL_EXTRACT_BITS(data_pe_mmfr0, 32, 35) == 0x2)) {
          if (VAL_EXTRACT_BITS(data_smmu_idr5, 5, 5) != 1) {
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
              return;
          }
      }

      /* If PE Supports 64KB Gran, (TGran64 == 0) or (TGran64_2 == 0x2) */
      if ((VAL_EXTRACT_BITS(data_pe_mmfr0, 24, 27) == 0x0) ||
          (VAL_EXTRACT_BITS(data_pe_mmfr0, 36, 39) == 0x2)) {
          is_gran64k = 1;
          if (VAL_EXTRACT_BITS(data_smmu_idr5, 6, 6) != 1) {
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
              return;
          }
      }

      /* If 4K & 64K Granule Not Supported */
      if (!(is_gran4k && is_gran64k)) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 04));
          return;
      }
  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
i010_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
