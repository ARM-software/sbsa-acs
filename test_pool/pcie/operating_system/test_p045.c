/** @file
 * Copyright (c) 2020-2024, Arm Limited or its affiliates. All rights reserved.
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

#include "val/sbsa/include/sbsa_acs_pcie.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_memory.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 45)
#define TEST_DESC  "Check all RP in HB is in same ECAM"
#define TEST_RULE  "PCI_IN_03"


static
void
payload(void)
{

  uint32_t bdf;
  uint32_t dp_type;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t ecam_index;
  uint64_t ecam_base;
  uint32_t reg_value;
  uint16_t vendor_id;
  uint32_t device_id;
  uint32_t test_skip = 1;
  uint32_t test_fail = 0;
  uint32_t segment;
  uint64_t rp_ecam_base;
  uint32_t rp_segment;

  pcie_device_bdf_table *bdf_tbl_ptr;

  tbl_index = 0;
  ecam_index = 0;
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      next_bdf:
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      if (dp_type == iEP_RP)
      {
          test_skip = 0;
          ecam_index = 0;

          val_pcie_read_cfg(bdf, TYPE01_VIDR, &reg_value);
          device_id = (reg_value >> TYPE01_DIDR_SHIFT) & TYPE01_DIDR_MASK;
          vendor_id = (reg_value >> TYPE01_VIDR_SHIFT) & TYPE01_VIDR_MASK;
          val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x ", bdf);
          val_print(ACS_PRINT_DEBUG, "Dev ID: 0x%x ", device_id);
          val_print(ACS_PRINT_DEBUG, "Vendor ID: 0x%x", vendor_id);

          rp_ecam_base = val_pcie_get_ecam_base(bdf);
          rp_segment = PCIE_EXTRACT_BDF_SEG(bdf);

          while (ecam_index < val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0))
          {
              ecam_base = val_pcie_get_info(PCIE_INFO_ECAM, ecam_index);
              segment = val_pcie_get_info(PCIE_INFO_SEGMENT, ecam_index);

              if (ecam_base == rp_ecam_base && segment == rp_segment)
              {
                  val_print(ACS_PRINT_DEBUG,
                            "\n       ECAM base 0x%x matches with RPs base address ", ecam_base);
                  goto next_bdf;
              }

              ecam_index++;
          }

          val_print(ACS_PRINT_ERR, "\n       RP BDF 0x%x not under any HB", bdf);
          test_fail++;
      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG, "\n       No iEP_RP type device found. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  }
  else if (test_fail)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, test_fail));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
p045_entry(uint32_t num_pe)
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
