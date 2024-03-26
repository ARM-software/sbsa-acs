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

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 44)
#define TEST_DESC  "Check device under RP in same ECAM"
#define TEST_RULE  "PCI_IN_04"


static uint8_t func_ecam_is_rp_ecam(uint32_t dsf_bdf)
{

  uint8_t dsf_bus;
  uint32_t bdf;
  uint32_t dp_type;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t ecam_cc;
  uint32_t pciio_proto_cc;
  addr_t ecam_base;
  pcie_device_bdf_table *bdf_tbl_ptr;

  tbl_index = 0;
  dsf_bus = PCIE_EXTRACT_BDF_BUS(dsf_bdf);
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check if this table entry is a Root Port */
      if (dp_type == iEP_RP)
      {
          if (PCIE_EXTRACT_BDF_SEG(bdf) != PCIE_EXTRACT_BDF_SEG(dsf_bdf))
              continue;

          /* Check if the entry's bus range covers down stream function */
          val_pcie_read_cfg(bdf, TYPE1_PBN, &reg_value);
          if ((dsf_bus >= ((reg_value >> SECBN_SHIFT) & SECBN_MASK)) &&
              (dsf_bus <= ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK)))
          {
              ecam_base = val_pcie_get_ecam_base(bdf);

              /* Read Function's Class Code through ECAM method */
              ecam_cc = val_mmio_read(ecam_base +
                        dsf_bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * PCIE_CFG_SIZE +
                        PCIE_EXTRACT_BDF_DEV(dsf_bdf) * PCIE_MAX_FUNC * PCIE_CFG_SIZE +
                        PCIE_EXTRACT_BDF_FUNC(dsf_bdf) * PCIE_CFG_SIZE +
                        TYPE01_RIDR);

              /* Read Function's Class Code through Pciio Protocol method */
              val_pcie_io_read_cfg(dsf_bdf, TYPE01_RIDR, &pciio_proto_cc);

              /* Return success if both methods read same Class Code */
              if (ecam_cc == pciio_proto_cc)
                  return 0;
              else
                  return 1;
          }
      }
  }

  return 1;
}

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t dp_type;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t fail_cnt;
  uint32_t test_skip = 1;
  pcie_device_bdf_table *bdf_tbl_ptr;

  fail_cnt = 0;
  tbl_index = 0;
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      /*
       * If a function is in the hierarchy domain
       * originated by a Root Port, check its ECAM
       * is same as its RootPort ECAM.
       */
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);
      if (dp_type == iEP_EP) {
          val_print(ACS_PRINT_DEBUG, "\n        BDF - 0x%x ", bdf);
          /* If test runs for atleast an endpoint */
          test_skip = 0;

          if (func_ecam_is_rp_ecam(bdf))
          {
              val_print(ACS_PRINT_ERR, "dp_type: 0x%x ", dp_type);
              fail_cnt++;
          }
      }
  }

  if (test_skip == 1) {
      val_print(ACS_PRINT_DEBUG, "\n       No iEP_EP type device found. Skipping test", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  }
  else if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t
p044_entry(uint32_t num_pe)
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
