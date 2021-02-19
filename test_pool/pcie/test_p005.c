/** @file
 * Copyright (c) 2016-2018, 2020-2021 Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_memory.h"

/* SBSA-checklist 63 & 64 */
#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 5)
#define TEST_DESC  "PCIe Unaligned access, Norm mem   "

#define DATA 0xC0DECAFE

static
void
payload(void)
{
  uint32_t count = 0;
  uint32_t data;
  uint32_t bdf;
  uint32_t bar_reg_value;
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

  count = val_peripheral_get_info(NUM_SATA, 0);

  while (count--) {
next_bdf:
      bdf = val_peripheral_get_info(SATA_BDF, count);
      offset = BAR0_OFFSET;

      while(offset <= BAR_MAX_OFFSET) {
          val_pcie_read_cfg(bdf, offset, &bar_value);
          val_print(AVS_PRINT_DEBUG, "\n The BAR value of bdf %x", bdf);
          val_print(AVS_PRINT_DEBUG, " is %x ", bar_value);
          base = 0;

          if (bar_value == 0)
          {
              /** This BAR is not implemented **/
              count--;
              goto next_bdf;
          }

          /* Skip for IO address space */
          if (bar_value & 0x1) {
              count--;
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
              val_pcie_read_cfg(bdf, offset, &bar_reg_value);
              bar_size = bar_reg_value & 0xFFFFFFF0;
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
              val_pcie_read_cfg(bdf, offset, &bar_reg_value);
              bar_reg_value = bar_reg_value & 0xFFFFFFF0;
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

          /* Map SATA Controller BARs to a NORMAL memory attribute. check unaligned access */
          baseptr = (char *)val_memory_ioremap((void *)base, 1024, NORMAL_NC);

          /* Check for unaligned access */
          *(uint32_t *)(baseptr) = DATA;
          data = *(char *)(baseptr+3);

          val_memory_unmap(baseptr);

          if (data != (DATA >> 24)) {
              val_print(AVS_PRINT_ERR, "Unaligned data mismatch", 0);
              test_fail++;
          }

          /* Map SATA Controller BARs to a DEVICE memory attribute and check transaction */
          baseptr = (char *)val_memory_ioremap((void *)base, 1024, DEVICE_nGnRnE);

          *(uint32_t *)(baseptr) = DATA;
          data = *(uint32_t *)(baseptr);

          val_memory_unmap(baseptr);

          if (data != DATA) {
              val_print(AVS_PRINT_ERR, "Data value mismatch", 0);
              test_fail++;
          }

next_bar:
          if (BAR_REG(bar_reg_value) == BAR_32_BIT)
              offset=offset+4;

          if (BAR_REG(bar_reg_value) == BAR_64_BIT)
              offset=offset+8;
      }
  }

  if (test_skip)
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 0));
  else if (test_fail)
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, test_fail));
  else
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 0));

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
