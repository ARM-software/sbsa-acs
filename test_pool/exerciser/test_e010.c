/** @file
 * Copyright (c) 2019-2021, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_pcie_enumeration.h"
#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_smmu.h"
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_exerciser.h"

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 10)
#define TEST_DESC  "Check RP Sub Bus transactions are TYPE1"

#define MAX_BUS   255
#define BUS_SHIFT 16
#define BUS_MASK  0xff

uint8_t
get_rp_right_sibling(uint32_t rp_bdf, uint32_t *rs_bdf)
{

  uint32_t dp_type;
  uint32_t tbl_bdf;
  uint32_t tbl_index;
  uint32_t tbl_reg_value;
  uint32_t rp_reg_value;
  pcie_device_bdf_table *bdf_tbl_ptr;

  tbl_index = 0;
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      tbl_bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(tbl_bdf);

      if (tbl_bdf != rp_bdf && dp_type == RP)
      {
         /*
          * Check if the secondary bus of the root port
          * corresponding to tbl_bdf is one gretaer than
          * the suborinate bus number of the input root
          * port. If equal, tbl_bdf must be right sibling
          * of rp_bdf.
          */
          val_pcie_read_cfg(rp_bdf, TYPE1_PBN, &rp_reg_value);
          val_pcie_read_cfg(tbl_bdf, TYPE1_PBN, &tbl_reg_value);
          if (((tbl_reg_value >> SECBN_SHIFT) & SECBN_MASK) ==
              (((rp_reg_value >> SUBBN_SHIFT) & SUBBN_MASK) + 1))
          {
              *rs_bdf = tbl_bdf;
              return 0;
          }
      }
  }

  /* Return failure if No right sibling */
  *rs_bdf = 0;
  return 1;
}

static
void
payload(void)
{

  uint32_t pe_index;
  uint32_t rs_flag;
  uint32_t e_bdf;
  uint32_t e_bus;
  uint32_t erp_bdf;
  uint32_t erp_rs_bdf;
  uint32_t reg_value;
  uint32_t erp_sub_bus;
  uint32_t erp_reg_value;
  uint32_t erp_rs_reg_value;
  uint32_t instance;
  uint32_t fail_cnt;
  uint32_t test_skip;
  uint32_t status;
  uint64_t header_type;

  fail_cnt = 0;
  test_skip = 1;
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  while (instance-- != 0)
  {
      rs_flag = 0;

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

      e_bdf = val_exerciser_get_bdf(instance);

      /* Check if exerciser is child of one of the rootports */
      if (val_pcie_parent_is_rootport(e_bdf, &erp_bdf))
          continue;

      /* Find right sibling of the exerciser rootport */
      if (!get_rp_right_sibling(erp_bdf, &erp_rs_bdf))
      {
          /*
           * Save exerciser right sibling sec and sub bus numbers.
           * Program those bus numbers with invalid range so that
           * the right sibling doesn't receive any transactions.
           */
          rs_flag = 1;
          val_pcie_read_cfg(erp_rs_bdf, TYPE1_PBN, &erp_rs_reg_value);
          reg_value = erp_rs_reg_value & (~(SECBN_MASK << SECBN_SHIFT));
          reg_value = reg_value & (~(SUBBN_MASK << SUBBN_SHIFT));
          reg_value = reg_value | (MAX_BUS << SECBN_SHIFT);
          reg_value = reg_value | ((MAX_BUS-1) << SUBBN_SHIFT);
          val_pcie_write_cfg(erp_rs_bdf, TYPE1_PBN, reg_value);
      }

      /* Increase Subordinate bus register of exerciser rootport by one */
      val_pcie_read_cfg(erp_bdf, TYPE1_PBN, &erp_reg_value);
      erp_sub_bus = (erp_reg_value >> SUBBN_SHIFT) & SUBBN_MASK;
      reg_value = erp_reg_value & (~(SUBBN_MASK << SUBBN_SHIFT));
      reg_value = reg_value | ((erp_sub_bus + 1) << SUBBN_SHIFT);
      val_pcie_write_cfg(erp_bdf, TYPE1_PBN, reg_value);

      /* Increment bus number in e_bdf variable */
      e_bus = (e_bdf >> BUS_SHIFT) & BUS_MASK;
      e_bdf = e_bdf & (~(BUS_MASK << BUS_SHIFT));
      e_bdf = e_bdf | ((e_bus + 1) << BUS_SHIFT);

      /*
       * Generate a config request from PE to the Subordinate bus
       * of the exerciser. Exerciser should see this request as a
       * Type 1 Request.
       */
      status = val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance);
      if (status == PCIE_CAP_NOT_FOUND)
      {
          goto restore;
      }

      val_pcie_read_cfg(e_bdf, TYPE01_VIDR, &reg_value);
      status = val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);
      if (status == PCIE_CAP_NOT_FOUND)
      {
          goto restore;
      }

      test_skip = 0;
      val_exerciser_get_param(CFG_TXN_ATTRIBUTES, (uint64_t *)&header_type, 0, instance);
      if (header_type != TYPE1)
      {
          val_print(AVS_PRINT_ERR, "\n       BDF 0x%x Sub Bus Transaction failure", erp_bdf);
          fail_cnt++;
      }

restore:
      /* Restore Exerciser rootport and it's right sibling subordinate bus registers */
      val_pcie_write_cfg(erp_bdf, TYPE1_PBN, erp_reg_value);
      if (rs_flag)
          val_pcie_write_cfg(erp_rs_bdf, TYPE1_PBN, erp_rs_reg_value);
  }

  if (test_skip)
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
  else if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

  return;

}

uint32_t
e010_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
