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

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 65)
#define TEST_DESC  "Check RP Extensions for DPC           "
#define TEST_RULE  "PCI_ER_09"

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t new_reg_value;
  uint32_t test_fails;
  uint32_t cap_base;
  uint32_t dp_type;
  uint32_t status;
  uint32_t dpc_supported;
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

      if ((dp_type == RP) || (dp_type == iEP_RP))
      {
          val_print(ACS_PRINT_DEBUG, "\n       BDF - 0x%x", bdf);

          /* Retrieve the addr of Downstream Port Containment (1Dh) and check if the
          * capability structure is supported.
          */
          status = val_pcie_find_capability(bdf, PCIE_ECAP, ECID_DPC, &cap_base);
          if (status == PCIE_CAP_NOT_FOUND)
              continue;

          /* If test runs for atleast one Rootport */
          test_skip = 0;

          /* Read DPC Capability Register(04h) present in Downstream Port Containment struct(1Dh)*/
          val_pcie_read_cfg(bdf, cap_base + DPC_CTRL_OFFSET, &reg_value);

          /* Extract 'RP Extensions for DPC' value and write the negated value to the bitfield*/
          dpc_supported = (reg_value >> DPC_RP_EXT_OFFSET) & DPC_RP_EXT_MASK;
          val_pcie_write_cfg(bdf, cap_base + DPC_CTRL_OFFSET,
                                            (reg_value) ^ ((dpc_supported) << DPC_RP_EXT_OFFSET));

          /* Read the register after the write. RP Extensions for DPC is a RO bit. Test fails if
             the reg values before and after the write are different.*/
          val_pcie_read_cfg(bdf, cap_base + DPC_CTRL_OFFSET, &new_reg_value);

          if (new_reg_value != reg_value) {
              val_print(ACS_PRINT_ERR, "\n       Failed. BDF - 0x%x ", bdf);
              val_print(ACS_PRINT_ERR, "RP Extension for DPC has incorrect access permission", 0);
              test_fails++;
          }
      }
  }


  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG,
               "\n       Found no RP with DPC Capability. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 02));
  }
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
p065_entry(uint32_t num_pe)
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