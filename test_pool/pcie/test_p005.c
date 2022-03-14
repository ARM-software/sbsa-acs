/** @file
 * Copyright (c) 2016-2018, 2020-2022 Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_memory.h"

/* SBSA-checklist 63 & 64 */
#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 5)
#define TEST_DESC  "PCIe Unaligned access, Norm mem   "

#define DATA 0xC0DECAFE

static void *branch_to_test;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to point to next instrcution */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(AVS_PRINT_ERR, "\n       Received Exception %d", interrupt_type);
  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
}

static
void
payload(void)
{
  uint32_t old_data;
  uint32_t bdf;
  uint32_t bar_reg_value;
  uint32_t bar_reg_lower_value;
  uint64_t bar_upper_bits;
  uint32_t bar_value;
  uint32_t bar_value_1;
  uint64_t bar_size;
  char    *baseptr;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t test_skip = 1;
  uint32_t test_fail = 0;
  uint64_t offset;
  uint64_t base;
  pcie_device_bdf_table *bdf_tbl_ptr;
  uint32_t tbl_index = 0;
  uint32_t status;
  uint32_t dev_type;
  uint32_t max_bar_offset;
  uint32_t msa_en = 0;

  val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 0));

  /* Install exception handlers */
  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status)
  {
      val_print(AVS_PRINT_ERR, "\n      Failed in installing the exception handler", 0);
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

next_bdf:
  for (;tbl_index < bdf_tbl_ptr->num_entries; tbl_index++) {
      msa_en = 0;
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;

      /* Configure the max BAR offset */
      dev_type = val_pcie_get_device_type(bdf);
      if (dev_type == 0)
          max_bar_offset = BAR_TYPE_0_MAX_OFFSET;
      else
          max_bar_offset = BAR_TYPE_1_MAX_OFFSET;

      offset = BAR0_OFFSET;

      while(offset <= max_bar_offset) {
          val_pcie_read_cfg(bdf, offset, &bar_value);
          val_print(AVS_PRINT_DEBUG, "\n The BAR value of bdf %x", bdf);
          val_print(AVS_PRINT_DEBUG, " is %x ", bar_value);
          base = 0;

          if (bar_value == 0)
          {
              /** This BAR is not implemented **/
              tbl_index++;
              goto next_bdf;
          }

          /* Skip for IO address space */
          if (bar_value & 0x1) {
              tbl_index++;
              goto next_bdf;
          }

          if (BAR_REG(bar_value) == BAR_64_BIT)
          {
              val_print(AVS_PRINT_INFO, "The BAR supports 64-bit address decoding capability \n", 0);
              val_pcie_read_cfg(bdf, offset+4, &bar_value_1);
              base = bar_value_1;

              /* BAR supports 64-bit address therefore, write all 1's
               * to BARn and BARn+1 and identify the size requested
               */
              val_pcie_write_cfg(bdf, offset, 0xFFFFFFF0);
              val_pcie_write_cfg(bdf, offset + 4, 0xFFFFFFFF);
              val_pcie_read_cfg(bdf, offset, &bar_reg_lower_value);
              bar_size = bar_reg_lower_value & 0xFFFFFFF0;
              val_pcie_read_cfg(bdf, offset + 4, &bar_reg_value);
              bar_upper_bits = bar_reg_value;
              bar_size = bar_size | (bar_upper_bits << 32 );
              bar_size = ~bar_size + 1;

              /* Restore the original BAR value */
              val_pcie_write_cfg(bdf, offset + 4, bar_value_1);
              val_pcie_write_cfg(bdf, offset, bar_value);
              base = (base << 32) | bar_value;
          }

          else {
              val_print(AVS_PRINT_INFO, "The BAR supports 32-bit address decoding capability\n", 0);

              /* BAR supports 32-bit address. Write all 1's
               * to BARn and identify the size requested
               */
              val_pcie_write_cfg(bdf, offset, 0xFFFFFFF0);
              val_pcie_read_cfg(bdf, offset, &bar_reg_lower_value);
              bar_reg_value = bar_reg_lower_value & 0xFFFFFFF0;
              bar_size = ~bar_reg_value + 1;

              /* Restore the original BAR value */
              val_pcie_write_cfg(bdf, offset, bar_value);
              base = bar_value;
          }

          val_print(AVS_PRINT_DEBUG, "\n BAR size is %x", bar_size);

          /* Check if bar supports the remap size */
          if (1024 > bar_size) {
              val_print(AVS_PRINT_ERR, "Bar size less than remap requested size", 0);
              goto next_bar;
          }

          test_skip = 0;

          /* Enable Memory Space Access to the BDF if not enabled */
          msa_en = val_pcie_is_msa_enabled(bdf);
          if (msa_en)
              val_pcie_enable_msa(bdf);

          branch_to_test = &&exception_return_normal;

          /* Map the BARs to a NORMAL memory attribute. check unaligned access */
          baseptr = (char *)val_memory_ioremap((void *)base, 1024, NORMAL_NC);

          /* Check for unaligned access. Normal memory can be read-only.
           * Not performing data comparison check.
           */
          old_data = *(uint32_t *)(baseptr);
          *(uint32_t *)(baseptr) = DATA;
          *(uint32_t *)(baseptr) = old_data;

exception_return_normal:
          val_memory_unmap(baseptr);
          if (IS_TEST_FAIL(val_get_status(index))) {
              val_print(AVS_PRINT_ERR, "\n       Normal memory access failed for Bdf: 0x%x", bdf);

              /* Setting the status to Pass to enable next check for current BDF.
               * Failure has been recorded with test_fail.
               */
              val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
              test_fail++;
          }

          branch_to_test = &&exception_return_device;

          /* Map the BARs to a DEVICE memory (non-cachable) attribute
           * and check transaction.
           */
          baseptr = (char *)val_memory_ioremap((void *)base, 1024, DEVICE_nGnRnE);

          /* Access check. Not performing data comparison check. */
          old_data = *(uint32_t *)(baseptr);
          *(uint32_t *)(baseptr) = DATA;
          *(uint32_t *)(baseptr) = old_data;

          val_memory_unmap(baseptr);

exception_return_device:
          if (IS_TEST_FAIL(val_get_status(index))) {
              val_print(AVS_PRINT_ERR, "\n       Device memory access failed for Bdf: 0x%x", bdf);
              /* Setting the status to Pass to enable test for next BDF.
               * Failure has been recorded with test_fail.
               */
              val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 02));
              test_fail++;
          }

next_bar:
          if (BAR_REG(bar_reg_lower_value) == BAR_32_BIT)
              offset = offset + 4;

          if (BAR_REG(bar_reg_lower_value) == BAR_64_BIT)
              offset = offset + 8;

          if (msa_en)
              val_pcie_disable_msa(bdf);
      }
  }

  if (test_skip)
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 0));
  else if (test_fail)
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
  else
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 03));

}

uint32_t
p005_entry(uint32_t num_pe)
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
