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

#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_memory.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 43)
#define TEST_DESC  "Check ARI forwarding enable rule  "

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dp_type;
  uint32_t cap_base;
  uint32_t ari_frwd_enable;
  uint32_t seg_num;
  uint32_t dev_num;
  uint32_t dev_bdf;
  uint32_t sec_bus;
  uint32_t sub_bus;
  uint32_t test_fails;
  uint32_t reg_value;
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
      if ((dp_type == DP) || (dp_type == iEP_RP) || (dp_type == RP))
      {
          /* Read the ARI forwarding enable bit */
          val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &cap_base);
          val_pcie_read_cfg(bdf, cap_base + DCTL2R_OFFSET, &reg_value);
          ari_frwd_enable = (reg_value >> DCTL2R_AFE_SHIFT) & DCTL2R_AFE_MASK;

          /* If ARI forwarding enable set, skip the entry */
          if (ari_frwd_enable != 0)
              continue;

          val_pcie_read_cfg(bdf, TYPE1_PBN, &reg_value);
          sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);
          sub_bus = ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK);

          /* Skip the port, if switch is present below it */
          if (sec_bus != sub_bus)
              continue;

          /* Configuration Requests specifying Device Numbers (1-31) must be terminated by the
           * Downstream Port or the Root Port with an Unsupported Request Completion Status
           */

          for (dev_num = 1; dev_num < PCIE_MAX_DEV; dev_num++)
          {
              seg_num = PCIE_EXTRACT_BDF_SEG(bdf);

              /* Create bdf for Dev 1 to 31 below the RP */
              dev_bdf = PCIE_CREATE_BDF(seg_num, sec_bus, dev_num, 0);
              val_pcie_read_cfg(dev_bdf, TYPE01_VIDR, &reg_value);
              if (reg_value != PCIE_UNKNOWN_RESPONSE)
              {
                  test_fails++;
                  val_print(AVS_PRINT_ERR, "\n    Dev 0x%x found under", dev_bdf);
                  val_print(AVS_PRINT_ERR, " RP bdf 0x%x", bdf);
              }
          }
      }
  }

  if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p043_entry(uint32_t num_pe)
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
