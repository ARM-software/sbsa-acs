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

#include "val/include/sbsa_avs_smmu.h"
#include "val/include/sbsa_avs_pcie_enumeration.h"
#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_exerciser.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 10)
#define TEST_DESC  "PASID support atleast 16 bits     "

#define MIN_PASID_SUPPORT (1 << 16)
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
  int num_per = 0, num_smmu = 0, skip = 1;
  uint32_t max_pasids = 0;
  uint32_t index = val_pe_get_index_mpid (val_pe_get_mpid());
  uint32_t instance;
  uint32_t e_bdf;
  uint32_t erp_bdf;
  uint32_t e_pasid;
  uint64_t e_pasid_support;
  uint64_t erp_pasid_support;
  uint32_t start_bus;
  uint32_t start_segment;
  uint32_t start_bdf;
  uint32_t dma_len;
  void *source_buf;
  void *dest_buf;
  exerciser_data_t e_data;

  num_per = val_peripheral_get_info(NUM_ALL, 0);
  /* For each peripheral check for PASID support */
  /* If PASID is supported, test the max number of PASIDs supported */
  for(num_per--; num_per >= 0; num_per--)
  {
     if((max_pasids = val_peripheral_get_info(MAX_PASIDS, num_per)) > 0)
     {
        skip = 0;
        if(max_pasids < MIN_PASID_SUPPORT)
        {
           val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 1));
           break;
        }
     }
  }
  if(num_per < 0)
  {
     /* For each SMMUv3 check for PASID support */
     /* If PASID is supported, test the max number of PASIDs supported */
     num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);
     for(num_smmu--; num_smmu >= 0; num_smmu--)
     {
         if(val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) == 3)
         {
             if((max_pasids = val_smmu_max_pasids(val_smmu_get_info(SMMU_CTRL_BASE, num_smmu))) > 0)
             {
                 skip = 0;
                 if(max_pasids < MIN_PASID_SUPPORT)
                 {
                    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 2));
                    break;
                 }
             }
         }
     }
  }
  if(num_smmu < 0) {
      if(skip)
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 3));
      else
          val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 0));
  }

  /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  if (instance == 0) {
      val_print(AVS_PRINT_INFO, "    No exerciser cards in the system %x", 0);
      return;
  }

  /* Set start_bdf segment and bus numbers to 1st ecam region values */
  start_segment = val_pcie_get_info(PCIE_INFO_SEGMENT, 0);
  start_bus = val_pcie_get_info(PCIE_INFO_START_BUS, 0);
  start_bdf = PCIE_CREATE_BDF(start_segment, start_bus, 0, 0);

  while (instance-- != 0) {

    /* Get exerciser bdf */
    e_bdf = val_pcie_get_bdf(EXERCISER_CLASSCODE, start_bdf);
    start_bdf = val_pcie_increment_bdf(e_bdf);

    /* Derive exerciser root port bdf */
    erp_bdf = e_bdf;
    if(val_pcie_get_root_port_bdf(&erp_bdf)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %4x root port BDF fetch error", instance);
        goto pasid_test_failure;
    }

    /* Send exerciser bdf and it's root port bdf information to PAL */
    val_exerciser_set_param(PASID_ATTRIBUTES, e_bdf, erp_bdf, instance);

    /* Check if exerciser and it's root port have PASID TLP Prefix support */
    val_exerciser_get_param(PASID_ATTRIBUTES, &e_pasid_support, &erp_pasid_support, instance);
    if((e_pasid_support == 0) || (erp_pasid_support == 0))  {
        continue;
    }

    /* Program exerciser and it's root port to start sending TLPs
     * with PASID TLP Prefixes. This includes setting PASID Enable bit
     * in exerciser PASID Control register and the implementation specific
     * PASID Enable bit of the Root Port.
     */
    if(val_exerciser_ops(PASID_TLP_START, (uint64_t)&e_pasid, instance)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %x PASID TLP Prefix enable error", instance);
        goto pasid_test_failure;
    }

    /* Check if exerciser PASID is of minimum 16-bits width */
    if(e_pasid < MIN_PASID_SUPPORT) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %x PASID less than 16-bits width", instance);
        goto pasid_test_failure;
    }

    /*
     * Set the buf size required in a cacheable and shareable DDR space.
     * The test expects PAL to allocate a buffer of TEST_DATA_BLK_SIZE.
     */
    e_data.cs_ddr.size = TEST_DATA_BLK_SIZE;
    e_data.cs_ddr.phy_addr = NULL;
    e_data.cs_ddr.virt_addr = NULL;

    /* Get the DDR DMA capable regions information */
    if (val_exerciser_get_data(EXERCISER_DATA_CS_DDR_SPACE, &e_data, instance)) {
        val_print(AVS_PRINT_ERR, "\n      Error to read DMA capability for inst %4x    ", instance);
        goto pasid_test_failure;
    }

    /* Consider the first DMA capable DDR region for PASID functionality check */
    if (e_data.cs_ddr.virt_addr == NULL) {
        val_print(AVS_PRINT_ERR, "\n      Unexpected DDR region for exerciser %4x", instance);
        goto pasid_test_failure;
    }

    /* Set the virtual addresses for test buffers */
    source_buf = e_data.cs_ddr.virt_addr;
    dest_buf = source_buf + (TEST_DATA_BLK_SIZE / 2);
    dma_len = TEST_DATA_BLK_SIZE / 2;

    /* Initialize the source buffer with test specific data */
    write_source_buf_data(source_buf, dma_len);

    /* Program Exerciser DMA controller with the source buffer information */
    val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)source_buf, dma_len, instance);
    if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA write failure to exerciser %4x", instance);
        goto pasid_test_failure;
    }

    /* READ Back from Exerciser to validate above DMA write */
    val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)dest_buf, dma_len, instance);
    if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA read failure from exerciser %4x", instance);
        goto pasid_test_failure;
    }

    /* Disbale exerciser and it's root port to stop sending TLPs
     * with PASID TLP Prefixes. This includes re-setting PASID Enable bit
     * in exerciser PASID Control register and the implementation specific
     * PASID Enable bit of the Root Port.
     */
    if(val_exerciser_ops(PASID_TLP_STOP, (uint64_t)&e_pasid, instance)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %x PASID TLP Prefix disable error", instance);
        goto pasid_test_failure;
    }

    if (memcmp(source_buf, dest_buf, dma_len)) {
        val_print(AVS_PRINT_ERR, "\n        Data Comparison failure for Exerciser %4x", instance);
        goto pasid_test_failure;
    }

  }

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 0));
  return;

pasid_test_failure:
  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));;
  return;

}

uint32_t
p010_entry(uint32_t num_pe)
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
