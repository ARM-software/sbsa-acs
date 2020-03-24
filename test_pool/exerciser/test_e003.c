/** @file
 * Copyright (c) 2018-2019, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_smmu.h"
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_pcie_enumeration.h"
#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_exerciser.h"

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 3)
#define TEST_DESC  "PCIe Address translation check    "

#define TEST_DATA_BLK_SIZE  (16*1024)
#define TEST_DATA 0xDE

static
void
write_test_data(void *buf, uint32_t size)
{

  uint32_t index;

  for (index = 0; index < size; index++) {
    *((char8_t *)buf + index) = TEST_DATA;
  }

  val_data_cache_ops_by_va((addr_t)buf, CLEAN_AND_INVALIDATE);
}

static
void
payload(void)
{

  uint32_t pe_index;
  uint32_t dma_len;
  uint32_t base_index;
  uint32_t instance;
  uint32_t e_bdf;
  uint32_t smmu_index;
  void *dram_buf1_virt;
  void *dram_buf1_phys;
  void *dram_buf2_virt;
  void *dram_buf2_phys;
  void *dram_buf2_iova;

  dram_buf1_phys = NULL;
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  while (instance-- != 0) {

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

    /* Get exerciser bdf */
    e_bdf = val_exerciser_get_bdf(instance);

    /* Get SMMU node index for this exerciser instance */
    smmu_index = val_iovirt_get_rc_smmu_index(PCIE_EXTRACT_BDF_SEG(e_bdf));

    for (base_index = 0; base_index < TEST_DDR_REGION_CNT; base_index++) {

        /* Get a non-cacheable DDR Buffer of size test data block size */
        dram_buf1_virt = val_memory_alloc(TEST_DATA_BLK_SIZE);
        if (!dram_buf1_virt) {
            val_print(AVS_PRINT_ERR, "\n      Cacheable mem alloc failure %x", 02);
            val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
            return;
        }

        /* Set the virtual addresses for test buffers */
        dram_buf1_phys = val_memory_virt_to_phys(dram_buf1_virt);
        dram_buf2_virt = dram_buf1_virt + (TEST_DATA_BLK_SIZE / 2);
        dram_buf2_phys = dram_buf1_phys + (TEST_DATA_BLK_SIZE / 2);
        dma_len = TEST_DATA_BLK_SIZE / 2;
        if (smmu_index == AVS_INVALID_INDEX) {
            dram_buf2_iova = dram_buf2_phys;
        } else {
            dram_buf2_iova = (void *)val_smmu_pa2iova(smmu_index, (uint64_t)dram_buf2_phys);
        }

        /* Initialize the sender buffer with test specific data */
        write_test_data(dram_buf1_virt, dma_len);

        /* Program Exerciser DMA controller with sender buffer information.
         * As exerciser is not behind SMMU, IOVA is same as PA. Use PA to
         * program the exerciser DMA.
         */

        if (val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)dram_buf1_phys, dma_len, instance)) {
            val_print(AVS_PRINT_ERR, "\n      DMA attributes setting failure %4x", instance);
            goto test_fail;
        }

        if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance)) {
            val_print(AVS_PRINT_ERR, "\n      DMA write failure to exerciser %4x", instance);
            goto test_fail;
        }

       /* READ Back from Exerciser to validate above DMA write */
        if (val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)dram_buf2_iova, dma_len, instance)) {
            val_print(AVS_PRINT_ERR, "\n      DMA attributes setting failure %4x", instance);
            goto test_fail;
        }

        if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance)) {
            val_print(AVS_PRINT_ERR, "\n      DMA read failure from exerciser %4x", instance);
            goto test_fail;
        }

        if (val_memory_compare(dram_buf1_virt, dram_buf2_virt, dma_len)) {
            val_print(AVS_PRINT_ERR, "\n        Data Comparasion failure for Exerciser %4x", instance);
            goto test_fail;
        }

        /* Return the buffer to the heap manager */
        val_memory_free(dram_buf1_virt);
    }

  }

  val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  return;

test_fail:
  val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
  val_memory_free(dram_buf1_virt);
  return;

}


uint32_t
e003_entry(void)
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
