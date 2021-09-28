/** @file
 * Copyright (c) 2018-2021, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_smmu.h"
#include "val/include/sbsa_avs_pgt.h"
#include "val/include/sbsa_avs_iovirt.h"
#include "val/include/sbsa_avs_pcie_enumeration.h"
#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_exerciser.h"

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 5)
#define TEST_DESC  "Generate PASID PCIe transactions  "

#define TEST_DATA_NUM_PAGES  2
#define TEST_DATA 0xDE

#define MIN_PASID_BITS 16
#define MAX_PASID_BITS 20
#define TEST_PASID1 (0x1ul << (MIN_PASID_BITS - 1)) + (0x1ull << 8)
#define TEST_PASID2 (0x1ul << (MIN_PASID_BITS - 1)) + (0x2ull << 8)

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
clear_dram_buf(void *buf, uint32_t size)
{

  uint32_t index;

  for (index = 0; index < size; index++) {
    *((char8_t *)buf + index) = 0;
  }

  val_data_cache_ops_by_va((addr_t)buf, CLEAN_AND_INVALIDATE);
}

/*
 * For each exerciser behind an SMMU,
 * Create a mapping of 1 IOVA region to 2 PA regions, via SMMU
 * Each of the 2 mappings is identified by a distinct PASID
 * Confiugure exerciser DMA engine to access IOVA region base.
 * Configure exerciser to execute DMA with TEST_PASID1 in transactions, Accesses must PA region 1.
 * Configure exerciser to execute DMA with TEST_PASID2 in transactions, Accesses must PA region 2.
 */
static void
payload(void)
{
  uint32_t pe_index;
  uint32_t instance;
  uint64_t e_bdf;
  uint32_t e_valid_cnt;
  uint32_t dma_len;
  uint32_t smmu_ssid_bits;
  void *dram_buf_base_virt;
  void *dram_buf_pasid1_in_virt;
  void *dram_buf_pasid2_in_virt;
  void *dram_buf_pasid1_out_virt;
  void *dram_buf_pasid2_out_virt;
  uint64_t dram_buf_pasid1_base_phys;
  uint64_t dram_buf_pasid2_base_phys;
  uint64_t dram_buf_in_iova;
  uint64_t dram_buf_out_iova;
  uint32_t device_id, its_id;
  uint32_t page_size = val_memory_page_size();
  uint32_t test_data_blk_size = page_size * TEST_DATA_NUM_PAGES;
  memory_region_descriptor_t mem_desc_array[2], *mem_desc;
  smmu_master_attributes_t master = {0, 0, 0, 0, 0};
  pgt_descriptor_t pgt_desc;
  uint64_t ttbr;
  uint32_t exerciser_ssid_bits, status;
  uint64_t pgt_base_pasid1 = 0;
  uint64_t pgt_base_pasid2 = 0;

  /* Initialize DMA master and memory descriptors */
  val_memory_set(&master, sizeof(master), 0);
  val_memory_set(mem_desc_array, sizeof(mem_desc_array), 0);
  mem_desc = &mem_desc_array[0];
  e_valid_cnt = 0;
  pe_index = val_pe_get_index_mpid (val_pe_get_mpid());

  /* Allocate 2 test buffers, one for each pasid */
  dram_buf_base_virt = val_memory_alloc_pages(TEST_DATA_NUM_PAGES * 2);
  if (!dram_buf_base_virt) {
      val_print(AVS_PRINT_ERR, "\n      Cacheable mem alloc failure %x", 02);
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
      return;
  }

  /* Setup virtual and physical addresses for test buffers for each pasid */
  dram_buf_pasid1_in_virt = dram_buf_base_virt;
  dram_buf_pasid2_in_virt = dram_buf_base_virt + test_data_blk_size;
  dram_buf_pasid1_out_virt = dram_buf_pasid1_in_virt + test_data_blk_size/2;
  dram_buf_pasid2_out_virt = dram_buf_pasid2_in_virt + test_data_blk_size/2;

  dram_buf_pasid1_base_phys = (uint64_t)val_memory_virt_to_phys(dram_buf_pasid1_in_virt);
  dram_buf_pasid2_base_phys = (uint64_t)val_memory_virt_to_phys(dram_buf_pasid2_in_virt);

  dma_len = test_data_blk_size/2;

  /* Get translation attributes via TCR and translation table base via TTBR */
  if (val_pe_reg_read_tcr(0 /*for TTBR0*/, &pgt_desc.tcr))
  {
    val_print(AVS_PRINT_ERR, "\n      TCR read failure %x", 03);
    val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
    return;
  }
  if (val_pe_reg_read_ttbr(0 /*TTBR0*/, &ttbr))
  {
    val_print(AVS_PRINT_ERR, "\n      TTBR0 read failure %x", 04);
    val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 04));
    return;
  }
  pgt_desc.pgt_base = (ttbr & AARCH64_TTBR_ADDR_MASK);
  pgt_desc.mair = val_pe_reg_read(MAIR_ELx);
  pgt_desc.stage = PGT_STAGE1;

  /* Get memory attributes of the test buffer, we'll use the same attibutes to create
   * our own page table later.
   */
  if (val_pgt_get_attributes(pgt_desc, (uint64_t)dram_buf_base_virt, &mem_desc->attributes))
    goto test_fail;

  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);
  while (instance-- != 0) {
    clear_dram_buf(dram_buf_base_virt, test_data_blk_size * 2);

    /* if init fail moves to next exerciser */
    if (val_exerciser_init(instance))
        continue;

    /* Get exerciser bdf */
    e_bdf = val_exerciser_get_bdf(instance);

    /* Find SMMU node index for this pcie endpoint */
    master.smmu_index = val_iovirt_get_rc_smmu_index(PCIE_EXTRACT_BDF_SEG(e_bdf), PCIE_CREATE_BDF_PACKED(e_bdf));
    if (master.smmu_index == AVS_INVALID_INDEX) {
        continue;
    }

    /* Check if SMMU supports substreamid (ssid), and number of ssid bits supported*/
    smmu_ssid_bits = val_smmu_get_info(SMMU_SSID_BITS, master.smmu_index);

    if (smmu_ssid_bits < MIN_PASID_BITS || smmu_ssid_bits > MAX_PASID_BITS)
    {
        val_print(AVS_PRINT_ERR, "SMMU substreamid support error %d\n", smmu_ssid_bits);
        goto test_fail;
    }

    /* We just want to test minimum pasid size (16-bits) functionality.
     * Make sure exerciser supports at least that
     */
    status = val_pcie_get_max_pasid_width(e_bdf, &exerciser_ssid_bits);
    if (status == PCIE_CAP_NOT_FOUND)
    {
        val_print(AVS_PRINT_ERR, "\n PASID extended capability not found for BDF: %x", e_bdf);
        goto test_fail;
    }
    else if (status)
    {
        val_print(AVS_PRINT_ERR, "\n Error in obtaining the PASID max width for BDF: %x", e_bdf);
        goto test_fail;
    }
    if (exerciser_ssid_bits < MIN_PASID_BITS)
    {
        val_print(AVS_PRINT_ERR, "exerciser substreamid support error %d\n", exerciser_ssid_bits);
        goto test_fail;
    }

    master.ssid_bits = MIN_PASID_BITS;

    val_smmu_enable(master.smmu_index);

    /* Increment the exerciser count with pasid support */
    e_valid_cnt++;

    if (master.smmu_index != AVS_INVALID_INDEX &&
        val_iovirt_get_smmu_info(SMMU_CTRL_ARCH_MAJOR_REV, master.smmu_index) == 3) {

        if (val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(e_bdf),
                                       PCIE_EXTRACT_BDF_SEG(e_bdf),
                                       &device_id, &master.streamid,
                                       &its_id))
            continue;

        /* Intent is to do DMA with PASID1 and PASID2 sequentially, with same IOVA.
         * SMMU does virtual to physical address translation using
         * tables configured for each pasid.
         * Here we setup memory descriptor for creating page tables for pasid1.
         */
        mem_desc->virtual_address = (uint64_t)dram_buf_base_virt;
        mem_desc->physical_address = dram_buf_pasid1_base_phys;
        mem_desc->length = test_data_blk_size;
        mem_desc->attributes |= PGT_STAGE1_AP_RW;

        /* Need to know input and output address sizes before creating page table */
        if ((pgt_desc.ias = val_smmu_get_info(SMMU_IN_ADDR_SIZE, master.smmu_index)) == 0)
            goto test_fail;

        if ((pgt_desc.oas = val_smmu_get_info(SMMU_OUT_ADDR_SIZE, master.smmu_index)) == 0)
            goto test_fail;

        if (val_pgt_create(mem_desc, &pgt_desc))
            goto test_fail;

        pgt_base_pasid1 = pgt_desc.pgt_base;

        master.substreamid = TEST_PASID1;
        if (val_smmu_map(master, pgt_desc))
        {
            val_print(AVS_PRINT_ERR, "\n      SMMU mapping failed (%d)     ", master.substreamid);
            goto test_fail;
        }

        dram_buf_in_iova = mem_desc->virtual_address;
        dram_buf_out_iova = dram_buf_in_iova + (test_data_blk_size / 2);
    }

    write_test_data(dram_buf_pasid1_in_virt, dma_len);

    /* Program exerciser to start sending TLPs
     * with PASID TLP Prefixes. This includes setting PASID Enable bit
     * in exerciser PASID Control register and the implementation specific
     * PASID Enable bit of the Root Port.
     */
    if (val_exerciser_ops(PASID_TLP_START, (uint64_t)master.substreamid, instance)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %x PASID TLP Prefix enable error", instance);
        goto test_fail;
    }

    if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_in_iova, dma_len, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA attributes setting failure %4x", instance);
        goto test_fail;
    }

    /* Trigger DMA from input buffer to exerciser memory */
    if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA write failure to exerciser %4x", instance);
        goto test_fail;
    }

    if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_out_iova, dma_len, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA attributes setting failure %4x", instance);
        goto test_fail;
    }

    /* Trigger DMA from exerciser memory to output buffer*/
    if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA read failure from exerciser %4x", instance);
        goto test_fail;
    }

    if (val_memory_compare(dram_buf_pasid1_in_virt, dram_buf_pasid1_out_virt, dma_len)) {
        val_print(AVS_PRINT_ERR, "\n        Data Comparision failure for Exerciser %4x", instance);
        goto test_fail;
    }

    if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_in_iova, dma_len, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA attributes setting failure %4x", instance);
        goto test_fail;
    }

    /*
     * Disable exerciser to stop sending TLPs
     * with PASID TLP Prefixes. This includes re-setting PASID Enable bit
     * in exerciser PASID Control register.
     */
    if(val_exerciser_ops(PASID_TLP_STOP, (uint64_t)master.substreamid, instance)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %x PASID TLP Prefix disable error", instance);
        goto test_fail;
    }

    /* Now setup memory descriptor for creating page tables for pasid2. */
    mem_desc->virtual_address = (uint64_t)dram_buf_base_virt;
    mem_desc->physical_address = dram_buf_pasid2_base_phys;
    mem_desc->length = test_data_blk_size;
    mem_desc->attributes |= PGT_STAGE1_AP_RW;

    if (val_pgt_create(mem_desc, &pgt_desc))
        goto test_fail;

    pgt_base_pasid2 = pgt_desc.pgt_base;

    master.substreamid = TEST_PASID2;
    if (val_smmu_map(master, pgt_desc))
    {
        val_print(AVS_PRINT_ERR, "\n      SMMU mapping failed (%d)     ", master.substreamid);
        goto test_fail;
    }

    dram_buf_in_iova = mem_desc->virtual_address;
    dram_buf_out_iova = dram_buf_in_iova + (test_data_blk_size / 2);

    write_test_data(dram_buf_pasid2_in_virt, dma_len);

    /* Program exerciser and it's root port to start sending TLPs
     * with PASID TLP Prefixes. This includes setting PASID Enable bit
     * in exerciser PASID Control register and the implementation specific
     * PASID Enable bit of the Root Port.
     */
    if(val_exerciser_ops(PASID_TLP_START, (uint64_t)master.substreamid, instance)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %x PASID TLP Prefix enable error", instance);
        goto test_fail;
    }

    if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_in_iova, dma_len, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA attributes setting failure %4x", instance);
        goto test_fail;
    }

    /* Trigger DMA from input buffer to exerciser memory */
    if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA write failure to exerciser %4x", instance);
        goto test_fail;
    }

    if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_out_iova, dma_len, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA attributes setting failure %4x", instance);
        goto test_fail;
    }

    /* Trigger DMA from exerciser memory to output buffer */
    if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance)) {
        val_print(AVS_PRINT_ERR, "\n      DMA read failure from exerciser %4x", instance);
        goto test_fail;
    }

    if(val_exerciser_ops(PASID_TLP_STOP, (uint64_t)master.substreamid, instance)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %x PASID TLP Prefix disable error", instance);
        goto test_fail;
    }

    if (val_memory_compare(dram_buf_pasid2_in_virt, dram_buf_pasid2_out_virt, dma_len)) {
        val_print(AVS_PRINT_ERR, "\n        Data Comparasion failure for Exerciser %4x", instance);
        goto test_fail;
    }

    val_smmu_unmap(master);
    val_smmu_disable(master.smmu_index);
  }
  if (e_valid_cnt) {
    val_set_status (pe_index, RESULT_PASS (g_sbsa_level, TEST_NUM, 01));
  } else {
    val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 00));
  }
  goto test_clean;

test_fail:
  val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));;

test_clean:
  val_memory_free_pages(dram_buf_base_virt, TEST_DATA_NUM_PAGES * 2);
  if ((pgt_base_pasid1 != 0) || (pgt_base_pasid2 != 0))
  {
    val_pgt_destroy(pgt_desc);
  }
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
