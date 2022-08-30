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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 41)
#define TEST_DESC  "Check MSI and MSI-X support rule  "

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dp_type;
  uint32_t cap_base;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  uint32_t reg_value;
  uint32_t int_pin;

  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  test_fails = 0;

  /* Check for all the function present in bdf table */
  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Skip this Check for Host Bridge */
      if (val_pcie_is_host_bridge(bdf))
        continue;

      /* Check entry is endpoint or rciep */
      if ((dp_type == iEP_EP) || (dp_type == RCiEP))
      {
         val_print(AVS_PRINT_DEBUG, "\n    BDF 0x%x", bdf);

         val_pcie_read_cfg(bdf, TYPE01_ILR, &reg_value);
         int_pin = VAL_EXTRACT_BITS(reg_value, TYPE01_IPR_SHIFT, TYPE01_IPR_SHIFT + 7);
         val_print(AVS_PRINT_DEBUG, " int pin value %d", int_pin);

         val_print(AVS_PRINT_DEBUG, " MSI cap %d",
                                    val_pcie_find_capability(bdf, PCIE_CAP, CID_MSI, &cap_base));
         val_print(AVS_PRINT_DEBUG, " MSIX cap %d",
                                   val_pcie_find_capability(bdf, PCIE_CAP, CID_MSIX, &cap_base));

         /* If test runs for atleast an endpoint */
         test_skip = 0;

         /* If MSI or MSI-X not supported, but INTx supported test fails */
         if ((val_pcie_find_capability(bdf, PCIE_CAP, CID_MSI, &cap_base) == PCIE_CAP_NOT_FOUND)
           && (val_pcie_find_capability(bdf, PCIE_CAP, CID_MSIX, &cap_base) == PCIE_CAP_NOT_FOUND)
             && ((int_pin >= 1) && (int_pin <= 4)))
              test_fails++;
      }
  }

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p041_entry(uint32_t num_pe)
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
