/** @file
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
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
#include "val/common/include/val_interface.h"

#include "val/sbsa/include/sbsa_acs_pcie.h"
#include "val/common/include/acs_pe.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 67)
#define TEST_DESC  "Check Supported Link Speed for iEPs   "
#define TEST_RULE  "IE_REG_6, IE_REG_7, IE_REG_8, IE_REG_9"

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t test_fails;
  uint32_t cap_base;
  uint32_t dp_type;
  uint32_t status;
  uint32_t supp_link_speed;
  uint32_t driver_sw;
  uint32_t test_skip = 1;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  tbl_index = 0;
  test_fails = 0;

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);
      val_print(ACS_PRINT_INFO, "\n       BDF - 0x%x", bdf);

      if ((dp_type == iEP_EP) || (dp_type == iEP_RP))
      {
          status = val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &cap_base);
          if (status == PCIE_CAP_NOT_FOUND)
              continue;

          /* If test runs for atleast one iEP */
          test_skip = 0;

          val_pcie_read_cfg(bdf, cap_base + LCAP2R_OFFSET, &reg_value);
          supp_link_speed = (reg_value & LCAP2R_SLSV_MASK) >> LCAP2R_SLSV_SHIFT;

          if (supp_link_speed >= 4)
          {
              status = val_pcie_find_capability(bdf, PCIE_ECAP, ECID_SPCIECS, &cap_base);
              if (status == PCIE_CAP_NOT_FOUND)
              {
                  test_fails++;
                  val_print(ACS_PRINT_ERR, "\n       No Sec PCI ECS found for BDF: 0x%x", bdf);
              }
          }

          if (supp_link_speed >= 8)
          {
              status = val_pcie_find_capability(bdf, PCIE_ECAP, ECID_DLFECS, &cap_base);
              if (status == PCIE_CAP_NOT_FOUND)
              {
                  test_fails++;
                  val_print(ACS_PRINT_ERR, "\n       No DL feature ECS found for BDF: 0x%x", bdf);
              }

              status = val_pcie_find_capability(bdf, PCIE_ECAP, ECID_PL16ECS,  &cap_base);
              if (status == PCIE_CAP_NOT_FOUND)
              {
                  test_fails++;
                  val_print(ACS_PRINT_ERR, "\n       No PL 16GT/s ECS found for BDF: 0x%x", bdf);
              }
              else if (status == PCIE_SUCCESS)
              {
                  val_pcie_read_cfg(bdf, cap_base + 0x10, &reg_value);
                  if (reg_value != 0)
                  {
                      test_fails++;
                      val_print(ACS_PRINT_ERR, "\n       16 GT/s LDP not 0 for BDF: 0x%x", bdf);
                  }

                  val_pcie_read_cfg(bdf, cap_base + 0x14, &reg_value);
                  if (reg_value != 0)
                  {
                      test_fails++;
                      val_print(ACS_PRINT_ERR, "\n       16 GT/s FRDP not 0 for BDF: 0x%x", bdf);
                  }

                  val_pcie_read_cfg(bdf, cap_base + 0x18, &reg_value);
                  if (reg_value != 0)
                  {
                      test_fails++;
                      val_print(ACS_PRINT_ERR, "\n       16 GT/s SRDP not 0 for BDF: 0x%x", bdf);
                  }
              }

              status = val_pcie_find_capability(bdf, PCIE_ECAP, ECID_LMREC, &cap_base);
              if (status == PCIE_CAP_NOT_FOUND)
              {
                  test_fails++;
                  val_print(ACS_PRINT_ERR, "\n       No LM at Rx EC found for BDF: 0x%x", bdf);
              }
              else if (status == PCIE_SUCCESS)
              {
                  val_pcie_read_cfg(bdf, cap_base + 0x4, &reg_value);
                  driver_sw = (reg_value & MPCAPR_DS_MASK) >> MPCAPR_DS_SHIFT;
                  if (driver_sw != 0)
                  {
                      test_fails++;
                      val_print(ACS_PRINT_ERR, "\n       Marging drv sw not 0 for BDF: 0x%x", bdf);
                  }
              }
          }
      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG,
            "\n       No RCiEP/iEP_EP with Extended Cap found. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
p067_entry(uint32_t num_pe)
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
