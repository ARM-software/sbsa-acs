/** @file
 * Copyright (c) 2018, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 2)
#define TEST_DESC  "PCIe BAR access check             "

#define TEST_DATA 0xDEADDAED
static const ARM_NORMAL_MEM ARM_NORMAL_MEM_ARRAY[] = {NORMAL_NC, NORMAL_WT};
static const ARM_DEVICE_MEM ARM_DEVICE_MEM_ARRAY[] = {DEVICE_nGnRnE, DEVICE_nGnRE, DEVICE_nGRE, DEVICE_GRE};

static
void
payload(void)
{
  char *baseptr;
  uint32_t idx;
  uint32_t pe_index;
  uint32_t instance;
  exerciser_data_t e_data;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  while (instance-- != 0) {

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

    /* Get BAR 0 details for this instance */
    if (val_exerciser_get_data(EXERCISER_DATA_BAR0_SPACE, &e_data, instance)) {
        val_print(AVS_PRINT_ERR, "\n      Exerciser %d data read error     ", instance);
        goto test_fail;
    }

    /* Map mmio space to ARM device memory in MMU page tables */
    for (idx = 0; idx < sizeof(ARM_DEVICE_MEM_ARRAY)/sizeof(ARM_DEVICE_MEM_ARRAY[0]); idx++) {

        baseptr = (char *)val_memory_ioremap((void *)e_data.bar_space.base_addr,
                                              512,
                                              ARM_DEVICE_MEM_ARRAY[idx]);
        if (!baseptr) {
            val_print(AVS_PRINT_ERR, "\n     Failed in BAR ioremap for instance %x", instance);
            goto test_fail;
        }

        /* Write predefined data to BAR space and read it back */
        val_mmio_write((addr_t)baseptr, TEST_DATA);
        if (TEST_DATA != val_mmio_read((addr_t)baseptr)) {
            val_print(AVS_PRINT_ERR, "\n     Exerciser %d BAR space access error %x", instance);
            goto test_fail;
        }

        /* Remove BAR mapping from MMU page tables */
        val_memory_unmap(baseptr);
    }

    /* Do additional checks if the BAR is pcie prefetchable mmio space */
    if (e_data.bar_space.type == MMIO_PREFETCHABLE) {

        /* Map the mmio space to ARM normal memory in MMU page tables */
        for (idx = 0; idx < sizeof(ARM_NORMAL_MEM_ARRAY)/sizeof(ARM_NORMAL_MEM_ARRAY[0]); idx++) {
            baseptr = (char *)val_memory_ioremap((void *)e_data.bar_space.base_addr,
                                                  512,
                                                  ARM_NORMAL_MEM_ARRAY[idx]);
            if (!baseptr) {
                val_print(AVS_PRINT_ERR, "\n     Failed in BAR ioremap for instance %x", instance);
                goto test_fail;
            }

            /* Write predefined data to an unaligned address in mmio space and read it back */
            val_mmio_write((addr_t)(baseptr+3), TEST_DATA);
            if (TEST_DATA != val_mmio_read((addr_t)(baseptr+3))) {
                val_print(AVS_PRINT_ERR, "\n     Exerciser %d BAR space access error %x", instance);
                goto test_fail;
            }

            /* Remove BAR mapping from MMU page tables */
            val_memory_unmap(baseptr);
        }
    }

  }

  val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  return;

test_fail:
  val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
  return;

}

uint32_t
e002_entry(void)
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
