/** @file
 * Copyright (c) 2021-2023, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 16)
#define TEST_DESC  "ARI forwarding Rule               "
#define TEST_RULE  "PCI_IN_17"

static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t e_bdf;
  uint32_t erp_bdf;
  uint32_t reg_value;
  uint32_t instance;
  uint32_t fail_cnt;
  uint64_t header_type;
  uint32_t bus_value;
  uint32_t test_skip = 1;
  uint32_t cap_base;
  uint32_t seg_num;
  uint32_t sub_bus;
  uint32_t dev_num;
  uint32_t dev_bdf;
  uint32_t sec_bus;
  uint32_t dp_type;
  uint32_t func_num;
  uint32_t status;

  fail_cnt = 0;
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  while (instance-- != 0)
  {
      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

      e_bdf = val_exerciser_get_bdf(instance);

      /* ARI Capability not applicable for RCiEP */
      dp_type = val_pcie_device_port_type(e_bdf);
      if (dp_type == RCiEP)
          continue;

      /* Check if exerciser is child of one of the rootports */
      if (val_pcie_parent_is_rootport(e_bdf, &erp_bdf))
          continue;

      /* If test runs for atleast an endpoint */

      /* Enable the ARI forwarding enable bit in RP */
      val_pcie_find_capability(erp_bdf, PCIE_CAP, CID_PCIECS, &cap_base);
      val_pcie_read_cfg(erp_bdf, cap_base + DCTL2R_OFFSET, &reg_value);
      reg_value &= DCTL2R_MASK;
      reg_value |= (DCTL2R_AFE_MASK << DCTL2R_AFE_SHIFT);
      val_pcie_write_cfg(erp_bdf, cap_base + DCTL2R_OFFSET, reg_value);

      /* Enable the ARI forwarding enable bit in Exerciser */
      val_pcie_find_capability(e_bdf, PCIE_CAP, CID_PCIECS, &cap_base);
      val_pcie_read_cfg(e_bdf, cap_base + DCTL2R_OFFSET, &reg_value);
      reg_value &= DCTL2R_MASK;
      reg_value |= (DCTL2R_AFE_MASK << DCTL2R_AFE_SHIFT);
      val_pcie_write_cfg(e_bdf, cap_base + DCTL2R_OFFSET, reg_value);

      /* Read the secondary, subordinate bus and segment number */
      val_pcie_read_cfg(erp_bdf, TYPE1_PBN, &bus_value);
      sec_bus = ((bus_value >> SECBN_SHIFT) & SECBN_MASK);
      sub_bus = ((bus_value >> SUBBN_SHIFT) & SUBBN_MASK);
      seg_num = PCIE_EXTRACT_BDF_SEG(erp_bdf);

      /*
       * Generate a config request from PE to the Secondary bus
       * of the exerciser's root port. Exerciser should see this
       * request as a Type 0 Request.
       */
      for (dev_num = 0; dev_num < PCIE_MAX_DEV; dev_num++)
      {
          for (func_num = 0; func_num < PCIE_MAX_FUNC; func_num++)
          {
              /* Create bdf for function 0 to 255 below the RP and check request type */
              dev_bdf = PCIE_CREATE_BDF(seg_num, sec_bus, dev_num, func_num);
              status = val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance);
              if (status == PCIE_CAP_NOT_FOUND)
                  goto test_result;

              test_skip = 0;
              val_pcie_read_cfg(dev_bdf, TYPE01_VIDR, &reg_value);
              val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);
              val_exerciser_get_param(CFG_TXN_ATTRIBUTES, (uint64_t *)&header_type, 0, instance);
              if (header_type != TYPE0)
              {
                  val_print(AVS_PRINT_ERR, "\n       BDF 0x%x Sec Bus Type 0 error", erp_bdf);
                  fail_cnt++;
              }
          }
      }

      /* Skip the port, if there is only one device below it */
      if (sec_bus == sub_bus)
          continue;

      /* Change the bus number to a Bus number > RP secondary bus number
       * and < RP subordinate bus number.
       */
      bus_value &= SECBN_EXTRACT;
      bus_value |= (sec_bus + 1) << SECBN_SHIFT;
      val_pcie_write_cfg(erp_bdf, TYPE1_PBN, bus_value);

      /*
       * Generate a config request from PE to the Secondary bus
       * of the exerciser's root port. Exerciser should see this
       * request as a Type 1 Request.
       */
      for (dev_num = 0; dev_num < PCIE_MAX_DEV; dev_num++)
      {
          for (func_num = 0; func_num < PCIE_MAX_FUNC; func_num++)
          {
              /* Create bdf for function 0 to 255 below the RP and check request type */
              dev_bdf = PCIE_CREATE_BDF(seg_num, sec_bus, dev_num, func_num);
              status = val_exerciser_ops(START_TXN_MONITOR, CFG_READ, instance);
              if (status == PCIE_CAP_NOT_FOUND)
                  goto test_result;

              val_pcie_read_cfg(dev_bdf, TYPE01_VIDR, &reg_value);
              val_exerciser_ops(STOP_TXN_MONITOR, CFG_READ, instance);
              val_exerciser_get_param(CFG_TXN_ATTRIBUTES, (uint64_t *)&header_type, 0, instance);
              if (header_type != TYPE1)
              {
                  val_print(AVS_PRINT_ERR, "\n       BDF 0x%x Sec Bus Type 1 error", erp_bdf);
                  fail_cnt++;
              }
          }
      }

      /* Restore the Bus value */
      bus_value &= SECBN_EXTRACT;
      bus_value |= (sec_bus - 1) << SECBN_SHIFT;
      val_pcie_write_cfg(erp_bdf, TYPE1_PBN, bus_value);

  }

test_result:
  if (test_skip)
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
  else if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

  return;

}

uint32_t
e016_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
