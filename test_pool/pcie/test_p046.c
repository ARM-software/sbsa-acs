/** @file
 * Copyright (c) 2020 Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 46)
#define TEST_DESC  "Check RP Byte Enable Rules        "

static
void
payload(void)
{

  int8_t   i;
  uint32_t bdf;
  uint32_t dp_type;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t test_skip = 1;
  uint32_t ecam_cr, ecam_cr_8, ecam_cr_16, ecam_cr_new;
  uint32_t write_value = 0;
  uint32_t command_reg_offset;
  addr_t ecam_base;
  pcie_device_bdf_table *bdf_tbl_ptr;

  tbl_index = 0;
  ecam_cr = 0;
  ecam_cr_8 = 0;
  ecam_cr_16 = 0;
  ecam_cr_new = 0;

  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      if ((dp_type == RP) || (dp_type == iEP_RP)) {

        /* If test runs for atleast an endpoint */
        test_skip = 0;

        ecam_base = val_pcie_get_ecam_base(bdf);
        command_reg_offset = PCIE_EXTRACT_BDF_BUS(bdf) * PCIE_MAX_DEV * PCIE_MAX_FUNC * PCIE_CFG_SIZE +
                             PCIE_EXTRACT_BDF_DEV(bdf) * PCIE_MAX_FUNC * PCIE_CFG_SIZE +
                             PCIE_EXTRACT_BDF_FUNC(bdf) * PCIE_CFG_SIZE +
                             TYPE01_CR;

        /* Read Command Register of RP with 8 Bit, 16 Bit, 32 Bit and compare it */
        ecam_cr = val_mmio_read(ecam_base + command_reg_offset);
        for (i = 3; i >= 0; i--) {
          ecam_cr_8 = ecam_cr_8 << 8;
          ecam_cr_8 |= val_mmio_read8(ecam_base + command_reg_offset + i);
        }
        for (i = 1; i >= 0; i--) {
          ecam_cr_16 = ecam_cr_16 << 16;
          ecam_cr_16 |= val_mmio_read16(ecam_base + command_reg_offset + i*2);
        }

        if ((ecam_cr != ecam_cr_8) || (ecam_cr_8 != ecam_cr_16))
        {
          val_print(AVS_PRINT_ERR, "\n        Byte Enable Read Failed", 0);
          val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          return;
        }

        /* Check Read-Write-Read Behaviour for each 8 Bit */
        ecam_cr = val_mmio_read8(ecam_base + command_reg_offset);

        /* Change BME & MSE Bit Value */
        write_value = ecam_cr ^ ((1 << CR_MSE_SHIFT) | (1 << CR_BME_SHIFT));
        val_mmio_write8(ecam_base + command_reg_offset, write_value);

        ecam_cr_new = val_mmio_read8(ecam_base + command_reg_offset);
        if (write_value != ecam_cr_new)
        {
          val_print(AVS_PRINT_ERR, "\n        8 Bit Write Failed", 0);
          val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
          return;
        }

        /* Restore the value */
        val_mmio_write8(ecam_base + command_reg_offset, ecam_cr);

        /* Check Read-Write-Read Behaviour for each 16 Bit */
        ecam_cr = val_mmio_read16(ecam_base + command_reg_offset);

        /* Change BME & MSE Bit Value */
        write_value = ecam_cr ^ ((1 << CR_MSE_SHIFT) | (1 << CR_BME_SHIFT));
        val_mmio_write16(ecam_base + command_reg_offset, write_value);

        ecam_cr_new = val_mmio_read16(ecam_base + command_reg_offset);
        if (write_value != ecam_cr_new)
        {
          val_print(AVS_PRINT_ERR, "\n        16 Bit Write Failed", 0);
          val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
          return;
        }

        /* Restore the value */
        val_mmio_write16(ecam_base + command_reg_offset, ecam_cr);

        /* Check Read-Write-Read Behaviour for 32 Bit */
        ecam_cr = val_mmio_read(ecam_base + command_reg_offset);

        /* Change BME & MSE Bit Value */
        write_value = ecam_cr ^ ((1 << CR_MSE_SHIFT) | (1 << CR_BME_SHIFT));
        val_mmio_write(ecam_base + command_reg_offset, write_value);

        ecam_cr_new = val_mmio_read(ecam_base + command_reg_offset);
        if (write_value != ecam_cr_new)
        {
          val_print(AVS_PRINT_ERR, "\n        32 Bit Write Failed", 0);
          val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 04));
          return;
        }

        /* Restore the value */
        val_mmio_write(ecam_base + command_reg_offset, ecam_cr);

      }
  }

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p046_entry(uint32_t num_pe)
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
