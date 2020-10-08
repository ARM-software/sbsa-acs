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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 51)
#define TEST_DESC  "Check Sec Bus Reset For iEP_RP    "

uint32_t
get_iep_bdf_under_rp (uint32_t rp_bdf)
{
  uint32_t reg_value;
  uint32_t sec_bus;
  uint32_t dev_num;
  uint32_t seg;
  uint32_t dev_bdf;
  uint32_t dp_type;

  /* Read Secondary Bus from Config Space */
  val_pcie_read_cfg(rp_bdf, TYPE1_PBN, &reg_value);
  sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);

  /* Find iEP_EP device type under iEP_RP */
  for (dev_num = 0; dev_num < PCIE_MAX_DEV; dev_num++) {
      seg = PCIE_EXTRACT_BDF_SEG(rp_bdf);

      /* Create bdf for Dev 0 to 31 below the iEP_RP */
      dev_bdf = PCIE_CREATE_BDF(seg, sec_bus, dev_num, 0);

      val_pcie_read_cfg(dev_bdf, TYPE01_VIDR, &reg_value);
      if (reg_value == PCIE_UNKNOWN_RESPONSE)
          continue;

      dp_type = val_pcie_device_port_type(dev_bdf);
      if (dp_type == iEP_EP)
          return dev_bdf;
  }

  /* Could not find iEP */
  return 0x0;
}

uint32_t
is_sbr_failed (uint32_t bdf)
{
  uint32_t reg_value;
  uint32_t index;
  uint32_t check_failed;

  check_failed = 0;
  /* Check BAR base address is cleared */
  for (index = 0; index < TYPE0_MAX_BARS; index++) {

      val_pcie_read_cfg(bdf, TYPE01_BAR + (index * BAR_BASE_SHIFT), &reg_value);
      if ((reg_value >> BAR_BASE_SHIFT) != 0) {
          val_print(AVS_PRINT_ERR, "\n       BAR%d base addr not cleared", index);
          check_failed++;
      }
  }

  /* Check the Bus Master Enable bit & Memory Space Enable
   * bit is cleared
   */
  val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value);
  if ((((reg_value >> CR_BME_SHIFT) & CR_BME_MASK) != 0) ||
      (((reg_value >> CR_MSE_SHIFT) & CR_MSE_MASK) != 0))
  {
      val_print(AVS_PRINT_ERR, "\n       BME/MSE not cleared", 0);
      check_failed++;
  }

  return check_failed;
}

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t iep_bdf;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t dp_type;
  uint32_t reg_value;
  uint32_t iep_rp_found;
  uint32_t test_fails;
  uint32_t idx;
  void     *cfg_space_buf;
  addr_t   cfg_space_addr;
  pcie_device_bdf_table *bdf_tbl_ptr;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  tbl_index = 0;
  test_fails = 0;
  iep_rp_found = 0;
  cfg_space_buf = NULL;

  /* Check for all the function present in bdf table */
  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check entry is iEP_RP */
      if (dp_type == iEP_RP)
      {
          iep_rp_found = 1;

          /* Get BDF for iEP_EP under iEP_RP */
          iep_bdf = get_iep_bdf_under_rp(bdf);
          if (iep_bdf == 0x0) {
              val_print(AVS_PRINT_ERR, "\n       Could Not Find iEP_EP under iEP_RP.", 0);
              val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
              return;
          }

          /* Allocate 4KB of space for saving function configuration space */
          /* If memory allocation fail, fail the test */
          cfg_space_buf = val_memory_alloc(PCIE_CFG_SIZE);
          if (cfg_space_buf == NULL)
          {
              val_print(AVS_PRINT_ERR, "\n       Memory allocation failed.", 0);
              val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
              return;
          }

          /* Get configuration space address for iEP_EP */
          cfg_space_addr = val_pcie_get_bdf_config_addr(iep_bdf);
          val_print(AVS_PRINT_INFO, "\n       iEP_EP BDF 0x%x : ", iep_bdf);
          val_print(AVS_PRINT_INFO, "Config space addr 0x%x", cfg_space_addr);

          /* Save the iEP_EP config space to restore after Secondary Bus Reset */
          for (idx = 0; idx < PCIE_CFG_SIZE / 4; idx ++) {
              *((uint32_t *)cfg_space_buf + idx) = *((uint32_t *)cfg_space_addr + idx);
          }

          /* Set Secondary Bus Reset Bit in Bridge Control
           * Register of iEP_RP
           */
          val_pcie_read_cfg(bdf, TYPE01_ILR, &reg_value);
          reg_value = reg_value | BRIDGE_CTRL_SBR_SET;
          val_pcie_write_cfg(bdf,TYPE01_ILR, reg_value);

          /* Wait for Timeout */
          val_time_delay_ms(100 * ONE_MILLISECOND);

          /* Check whether iEP_RP Secondary Bus Reset works fine. */
          if (is_sbr_failed(iep_bdf)) {
              test_fails++;
          }

          /* Restore iEP_EP Config Space */
          for (idx = 0; idx < PCIE_CFG_SIZE / 4; idx ++) {
              *((uint32_t *)cfg_space_addr + idx) = *((uint32_t *)cfg_space_buf + idx);
          }

          val_memory_free(cfg_space_buf);
      }
  }

  /* Skip the test if no iEP_RP found */
  if (iep_rp_found == 0)
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
  else if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, test_fails));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p051_entry(uint32_t num_pe)
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
