/** @file
 * Copyright (c) 2020-2023, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 47)
#define TEST_DESC  "Check Config Txn for RP in HB     "
#define TEST_RULE  "PCI_IN_12"

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t Status;
  uint32_t dp_type;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t fail_cnt;
  uint32_t test_skip = 1;
  uint32_t ecam_cc;
  uint32_t pciio_proto_cc;
  addr_t ecam_base;
  pcie_device_bdf_table *bdf_tbl_ptr;

  fail_cnt = 0;
  tbl_index = 0;
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      /*
       * Check that Host Bridge Consumes the Configuration
       * request intended for RootPort Configuration space.
       * Access RootPort Config Space using ECAM Method.
       */
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);
      if ((dp_type == RP) || (dp_type == iEP_RP)) {
        /* Read Vendor ID of RP with ECAM based mechanism, and compare it with the */
        ecam_base = val_pcie_get_ecam_base(bdf);

        /* Read Function's Class Code through ECAM method */
        ecam_cc = val_mmio_read(ecam_base +
                  PCIE_EXTRACT_BDF_BUS(bdf) * PCIE_MAX_DEV * PCIE_MAX_FUNC * PCIE_CFG_SIZE +
                  PCIE_EXTRACT_BDF_DEV(bdf) * PCIE_MAX_FUNC * PCIE_CFG_SIZE +
                  PCIE_EXTRACT_BDF_FUNC(bdf) * PCIE_CFG_SIZE +
                  TYPE01_RIDR);

        /* Read Function's Class Code through Pciio Protocol method */
        Status = val_pcie_io_read_cfg(bdf, TYPE01_RIDR, &pciio_proto_cc);
        if (Status == PCIE_NO_MAPPING) {
          val_print(AVS_PRINT_ERR, "\n       Reading Class code using PciIo protocol failed ", 0);
          val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          return;
        }

        /* If test runs for atleast an endpoint */
        test_skip = 0;

        if (ecam_cc != pciio_proto_cc)
        {
          val_print(AVS_PRINT_ERR, "\n        Config Txn Error : 0x%x ", bdf);
          fail_cnt++;
        }
      }
  }

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
  else if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p047_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
