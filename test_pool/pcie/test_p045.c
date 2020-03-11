/** @file
 * Copyright (c) 2019-2020, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_memory.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 45)
#define TEST_DESC  "Check all RP in HB is in same ECAM"


static
void
payload(void)
{

  uint32_t bdf;
  uint32_t dp_type;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t ecam_index;
  uint32_t ecam_base;
  uint32_t reg_value;
  uint16_t vendor_id;
  uint32_t device_id;
  pcie_device_bdf_table *bdf_tbl_ptr;

  tbl_index = 0;
  ecam_index = 0;
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  while (ecam_index < val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0))
  {
      ecam_base = val_pcie_get_info(PCIE_INFO_ECAM, ecam_index);
      val_print(AVS_PRINT_ERR, "\n       WARNING: RPs under ECAM Base 0x%x :", ecam_base);

      while (tbl_index < bdf_tbl_ptr->num_entries)
      {
          bdf = bdf_tbl_ptr->device[tbl_index++].bdf;

          dp_type = val_pcie_device_port_type(bdf);
          if (dp_type == RP || dp_type == iEP_RP)
          {
              if (ecam_base == val_pcie_get_ecam_base(bdf))
              {
                  val_pcie_read_cfg(bdf, TYPE01_VIDR, &reg_value);
                  device_id = (reg_value >> TYPE01_DIDR_SHIFT) & TYPE01_DIDR_MASK;
                  vendor_id = (reg_value >> TYPE01_VIDR_SHIFT) & TYPE01_VIDR_MASK;

                  val_print(AVS_PRINT_ERR, "\n        BDF: 0x%x ", bdf);
                  val_print(AVS_PRINT_ERR, "Dev ID: 0x%x ", device_id);
                  val_print(AVS_PRINT_ERR, "Vendor ID: 0x%x", vendor_id);
              }
          }

      }

      ecam_index++;
  }

  val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p045_entry(uint32_t num_pe)
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
