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

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 43)
#define TEST_DESC  "Check ARI forwarding enable rule      "
#define TEST_RULE  "PCI_IN_17"

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dp_type;
  uint32_t cap_base;
  uint32_t seg_num;
  uint32_t dev_num;
  uint32_t dev_bdf;
  uint32_t sec_bus;
  uint32_t sub_bus;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  uint32_t reg_value;
  uint32_t status;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  test_fails = 0;

  /* Check for all the function present in bdf table */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is Downstream port or RP */
      if ((dp_type == DP) || (dp_type == iEP_RP))
      {
          /* Disable the ARI forwarding enable bit */
          val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &cap_base);
          val_pcie_read_cfg(bdf, cap_base + DCTL2R_OFFSET, &reg_value);
          reg_value &= DCTL2R_AFE_NORMAL;
          val_pcie_write_cfg(bdf, cap_base + DCTL2R_OFFSET, reg_value);

          /* Read the secondary and subordinate bus number */
          val_pcie_read_cfg(bdf, TYPE1_PBN, &reg_value);
          sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);
          sub_bus = ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK);
          status = val_pcie_data_link_layer_status(bdf);

          /* Skip the port, if switch is present below it or no device present*/
          if ((sec_bus != sub_bus) || (status != PCIE_DLL_LINK_STATUS_ACTIVE))
              continue;

          /* If test runs for atleast an endpoint */
          test_skip = 0;

          seg_num = PCIE_EXTRACT_BDF_SEG(bdf);
          dev_bdf = PCIE_CREATE_BDF(seg_num, sec_bus, 0, 0);
          status = val_pcie_read_cfg(dev_bdf, TYPE01_VIDR, &reg_value);
          if (status || (reg_value == PCIE_UNKNOWN_RESPONSE))
          {
              test_fails++;
              val_print(ACS_PRINT_ERR, "\n       Dev 0x%x found under", dev_bdf);
              val_print(ACS_PRINT_ERR, " RP bdf 0x%x", bdf);
          }

          /* Configuration Requests specifying Device Numbers (1-31) must be terminated by the
           * Downstream Port or the Root Port with an Unsupported Request Completion Status
           */
          for (dev_num = 1; dev_num < PCIE_MAX_DEV; dev_num++)
          {
              /* Create bdf for Dev 1 to 31 below the RP */
              dev_bdf = PCIE_CREATE_BDF(seg_num, sec_bus, dev_num, 0);
              val_pcie_read_cfg(dev_bdf, TYPE01_VIDR, &reg_value);
              if (reg_value != PCIE_UNKNOWN_RESPONSE)
              {
                  test_fails++;
                  val_print(ACS_PRINT_ERR, "\n       Dev 0x%x found under", dev_bdf);
                  val_print(ACS_PRINT_ERR, " RP bdf 0x%x", bdf);
              }
          }
      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG, "\n       No DP/ iEP_RP type device found.Skipping test", bdf);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  }

  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
p043_entry(uint32_t num_pe)
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
