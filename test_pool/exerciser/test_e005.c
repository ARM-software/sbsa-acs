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
#include "val/include/sbsa_avs_pcie_enumeration.h"
#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_exerciser.h"

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 5)
#define TEST_DESC  "Generate PASID PCIe transactions  "

#define TEST_PASID_VALUE ((1 << 16) + (1 << 12))
#define TEST_DATA_BLK_SIZE  512
#define TEST_DATA 0xDE

void write_source_buf_data(void *buf, uint32_t size)
{

  uint32_t index;

  for (index = 0; index < size; index++) {
    *((char8_t *)buf + index) = TEST_DATA;
  }

}

static void
payload(void)
{
  uint32_t pe_index;
  uint32_t instance;
  uint64_t e_bdf;
  uint32_t e_valid_cnt;
  uint32_t e_pasid;
  uint64_t e_pasid_support;
  uint32_t start_bus;
  uint32_t start_segment;
  uint32_t start_bdf;
  uint32_t dma_len;
  uint32_t smmu_index;
  void *src_buf_virt;
  void *src_buf_phys;
  void *dest_buf_virt;
  void *dest_buf_phys;

  e_valid_cnt = 0;
  src_buf_virt = NULL;
  pe_index = val_pe_get_index_mpid (val_pe_get_mpid());
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  /* Set start_bdf segment and bus numbers to 1st ecam region values */
  start_segment = val_pcie_get_info(PCIE_INFO_SEGMENT, 0);
  start_bus = val_pcie_get_info(PCIE_INFO_START_BUS, 0);
  start_bdf = PCIE_CREATE_BDF(start_segment, start_bus, 0, 0);

  while (instance-- != 0) {

    /* Get exerciser bdf */
    e_bdf = val_pcie_get_bdf(EXERCISER_CLASSCODE, start_bdf);
    start_bdf = val_pcie_increment_bdf(e_bdf);

    /* Check if exerciser and it's root port have PASID TLP Prefix support */
    val_exerciser_get_param(PASID_ATTRIBUTES, &e_bdf, &e_pasid_support, instance);
    if(e_pasid_support == 0)  {
        continue;
    }

    /* Increment the exerciser count with pasid support */
    e_valid_cnt++;

    /* Find SMMU node index for this pcie endpoint */
    smmu_index = val_iovirt_get_rc_smmu_index(PCIE_EXTRACT_BDF_SEG(e_bdf));
    if (smmu_index == AVS_INVALID_INDEX) {
        continue;
    }

    /* If the smmu already support the requested pasid,
     * return success. Else, PAL layer has to set up the
     * corresponding page tables correctlyi and then return
     * to support e_pasid. Upon success return zero, else one.
     */
    e_pasid = TEST_PASID_VALUE;
    if(val_smmu_create_pasid_entry(smmu_index, e_pasid)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %x smmu PASID enable error", instance);
        val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
        return;
    }

    /* Program exerciser and it's root port to start sending TLPs
     * with PASID TLP Prefixes. This includes setting PASID Enable bit
     * in exerciser PASID Control register and the implementation specific
     * PASID Enable bit of the Root Port.
     */
    if(val_exerciser_ops(PASID_TLP_START, (uint64_t)&e_pasid, instance)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %x PASID TLP Prefix enable error", instance);
        val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
        return;
    }

    /* Get a Caheable DDR Buffer of size test data block size */
    src_buf_virt = val_memory_alloc(TEST_DATA_BLK_SIZE);
    if (!src_buf_virt) {
      val_print(AVS_PRINT_ERR, "\n      Cacheable mem alloc failure %x", 02);
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
      return;
    }

    /* Set the virtual addresses for test buffers */
    src_buf_phys = val_memory_virt_to_phys(src_buf_virt);
    dest_buf_virt = src_buf_virt + (TEST_DATA_BLK_SIZE / 2);
    dest_buf_phys = src_buf_phys + (TEST_DATA_BLK_SIZE / 2);
    dma_len = TEST_DATA_BLK_SIZE / 2;

    /* Initialize the source buffer with test specific data */
    write_source_buf_data(src_buf_virt, dma_len);

    /* Program Exerciser DMA controller with the source buffer information */
    val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)src_buf_phys, dma_len, instance);
    if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA write failure to exerciser %4x", instance);
        goto test_fail;
    }

    /* READ Back from Exerciser to validate above DMA write */
    val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)dest_buf_phys, dma_len, instance);
    if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA read failure from exerciser %4x", instance);
        goto test_fail;
    }

    /* Disbale exerciser and it's root port to stop sending TLPs
     * with PASID TLP Prefixes. This includes re-setting PASID Enable bit
     * in exerciser PASID Control register and the implementation specific
     * PASID Enable bit of the Root Port.
     */
    if(val_exerciser_ops(PASID_TLP_STOP, (uint64_t)&e_pasid, instance)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %x PASID TLP Prefix disable error", instance);
        goto test_fail;
    }

    if (val_memory_compare(src_buf_virt, dest_buf_virt, dma_len)) {
        val_print(AVS_PRINT_ERR, "\n        Data Comparison failure for Exerciser %4x", instance);
        goto test_fail;
    }

  }

  if (e_valid_cnt) {
    val_memory_free(src_buf_virt);
    val_set_status (pe_index, RESULT_PASS (g_sbsa_level, TEST_NUM, 01));
  } else {
    val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 00));
  }

  return;

test_fail:
  val_memory_free(src_buf_virt);
  val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));;
  return;

}

uint32_t
e005_entry(void)
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
