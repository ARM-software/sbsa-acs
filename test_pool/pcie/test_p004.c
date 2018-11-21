/** @file
 * Copyright (c) 2016-2018, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_pcie_enumeration.h"
#include "val/include/sbsa_avs_exerciser.h"

/* SBSA-checklist 63 & 64 */
#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 4)
#define TEST_DESC  "PCIe Unaligned access, Norm mem   "

#define TEST_DATA 0xDEADDAED
static const ARM_NORMAL_MEM ARM_NORMAL_MEM_ARRAY[] = {NORMAL_NC, NORMAL_WT};
static const ARM_DEVICE_MEM ARM_DEVICE_MEM_ARRAY[] = {DEVICE_nGnRnE, DEVICE_nGnRE, DEVICE_nGRE, DEVICE_GRE};

static
void
payload(void)
{
  uint32_t count = 0;
  uint64_t base;
  uint32_t data;
  char *baseptr;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t mem_index;
  uint32_t ret;
  uint32_t instance;
  uint32_t status = 0;
  exerciser_data_t bar_space;

  /* Map SATA Controller BARs to a NORMAL memory attribute. check unaligned access */
  count = val_peripheral_get_info(NUM_SATA, 0);

  while (count--) {
      base = val_peripheral_get_info(SATA_BASE1, count);
      baseptr = (char *)val_memory_ioremap((void *)base, 1024, NORMAL_NC);

      data = *(uint32_t *)(baseptr+3);

      val_memory_unmap(baseptr);
  }

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  if (instance == 0) {
      val_print(AVS_PRINT_INFO, "    No exerciser cards in the system %x", 0);
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 0));
      return;
  }

  while (instance-- != 0) {

    /* Get the BAR 0 details for this instance */
    ret = val_exerciser_get_data(EXERCISER_DATA_BAR0_SPACE, &bar_space, instance);
    if (ret) {
        val_print(AVS_PRINT_ERR, "\n      Exerciser bar space Read error %4x    ", ret);
        status = 1;
        continue;
    }

    /* Map the mmio space to ARM device memory in MMU page tables */
    for (mem_index = 0; mem_index < 4; mem_index++) {

        baseptr = (char *)val_memory_ioremap((void *)bar_space.bar.base_addr, 512, ARM_DEVICE_MEM_ARRAY[mem_index]);
        if (!baseptr) {
            val_print(AVS_PRINT_ERR, "\n     Failed in BAR ioremap for instance %x", instance);
            status = 1;
            continue;
        }

        /* Write predefined data to BAR space and read it back */
        val_mmio_write((addr_t)baseptr, TEST_DATA);
        if (TEST_DATA != val_mmio_read((addr_t)baseptr)) {
            val_print(AVS_PRINT_ERR, "\n     Host Bridge packet drop for instance %x", instance);
            status = 1;
            continue;
        }

        /* Remove BAR mapping from MMU page tables */
        val_memory_unmap(baseptr);
    }

    /* Do additional checks if the BAR is pcie prefetchable mmio space */
    if (bar_space.bar.type == MMIO_PREFETCHABLE) {

        /* Map the mmio space to ARM normal memory in MMU page tables */
        for (mem_index = 0; mem_index < 2; mem_index++) {
            baseptr = (char *)val_memory_ioremap((void *)bar_space.bar.base_addr, 512, ARM_NORMAL_MEM_ARRAY[mem_index]);
            if (!baseptr) {
                val_print(AVS_PRINT_ERR, "\n     Failed in BAR ioremap for instance %x", instance);
                status = 1;
                continue;
            }

            /* Write predefined data to an unaligned address in mmio space and read it back */
            val_mmio_write((addr_t)(baseptr+3), TEST_DATA);
            if (TEST_DATA != val_mmio_read((addr_t)(baseptr+3))) {
                val_print(AVS_PRINT_ERR, "\n     Host Bridge packet drop for instance %x", instance);
                status = 1;
                continue;
            }

            /* Remove BAR mapping from MMU page tables */
            val_memory_unmap(baseptr);
        }
    }

  }

  if(!status)
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 0));
  else
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));

}

uint32_t
p004_entry(uint32_t num_pe)
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
