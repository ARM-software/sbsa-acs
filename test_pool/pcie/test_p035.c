/** @file
 * Copyright (c) 2019, 2020 Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 35)
#define TEST_DESC  "Check Function level reset rule   "

uint32_t is_flr_failed(uint32_t bdf)
{
  uint32_t reg_value;
  uint32_t index;
  uint32_t check_failed;

  check_failed = 0;
  /* Check BAR base address is cleared */
  for (index = 0; index < TYPE0_MAX_BARS; index++)
  {
      val_pcie_read_cfg(bdf, TYPE01_BAR + (index * BAR_BASE_SHIFT), &reg_value);
      if ((reg_value >> BAR_BASE_SHIFT) != 0)
      {
          val_print(AVS_PRINT_ERR, "\n BAR%d base addr not cleared", 0);
          check_failed++;
      }
  }

  /* Check the Bus Master Enable bit is cleared */
  val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value);
  if (((reg_value >> CR_BME_SHIFT) & CR_BME_MASK) != 0)
  {
      val_print(AVS_PRINT_ERR, "\n BME is not cleared", 0);
      check_failed++;
  }

  /* Check the Memory Space Enable bit is cleared */
  val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value);
  if (((reg_value >> CR_MSE_SHIFT) & CR_MSE_MASK) != 0)
  {
      val_print(AVS_PRINT_ERR, "\n MSE is not cleared", 0);
      check_failed++;
  }

  return check_failed;
}

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t dp_type;
  uint32_t cap_base;
  uint32_t flr_cap;
  uint32_t test_fails;
  uint32_t test_skip = 1;
  uint32_t idx;
  addr_t config_space_addr;
  void *func_config_space;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  tbl_index = 0;
  test_fails = 0;

  /* Check for all the function present in bdf table */
  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is  RCiEP or iEP endpoint or normal EP */
      if ((dp_type == RCiEP) || (dp_type == iEP_EP) || (dp_type == EP))
      {
          /* Read FLR capability bit value */
          val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &cap_base);
          val_pcie_read_cfg(bdf, cap_base + DCAPR_OFFSET, &reg_value);
          flr_cap = (reg_value >> DCAPR_FLRC_SHIFT) & DCAPR_FLRC_MASK;

          /* If FLR capability is not set, move to next entry */
          if (!flr_cap)
              continue;

          /* Allocate 4KB of space for saving function configuration space */
          func_config_space = NULL;
          func_config_space = val_memory_alloc(PCIE_CFG_SIZE);

          /* If memory allocation fail, fail the test */
          if (func_config_space == NULL)
          {
              val_print(AVS_PRINT_ERR, "\n Memory allocation fail", 0);
              val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, test_fails));
              return;
          }

          /* Get function configuration space address */
          config_space_addr = val_pcie_get_bdf_config_addr(bdf);
          val_print(AVS_PRINT_INFO, "\n    BDF 0x%x ", bdf);
          val_print(AVS_PRINT_INFO, "config space addr 0x%x", config_space_addr);

          /* Save the function config space to restore after FLR */
          for (idx = 0; idx < PCIE_CFG_SIZE / 4; idx ++) {
              *((uint32_t *)func_config_space + idx) = *((uint32_t *)config_space_addr + idx);
          }

          /* Initiate FLR by setting the FLR bit */
          val_pcie_read_cfg(bdf, cap_base + DCTLR_OFFSET, &reg_value);
          reg_value = reg_value | DCTLR_FLR_SET;
          val_pcie_write_cfg(bdf, cap_base + DCTLR_OFFSET, reg_value);

          /* Wait for 100 ms */
          val_time_delay_ms(100 * ONE_MILLISECOND);

          /* If test runs for atleast an endpoint */
          test_skip = 0;

          /* Vendor Id should not be 0xFF after max FLR period */
          val_pcie_read_cfg(bdf, 0, &reg_value);
          if ((reg_value & TYPE01_VIDR_MASK) == TYPE01_VIDR_MASK)
          {
              val_print(AVS_PRINT_ERR, "\n BDF 0x%x not present", bdf);
              test_fails++;
              val_memory_free(func_config_space);
              continue;
          }

          if (is_flr_failed(bdf))
              test_fails++;

          /* Initialize the function config space */
          for (idx = 0; idx < PCIE_CFG_SIZE / 4; idx ++) {
              *((uint32_t *)config_space_addr + idx) = *((uint32_t *)func_config_space + idx);
          }

          val_memory_free(func_config_space);
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
p035_entry(uint32_t num_pe)
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
