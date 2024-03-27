/** @file
 * Copyright (c) 2018-2024, Arm Limited or its affiliates. All rights reserved.
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

#include "val/common/include/acs_val.h"
#include "val/sbsa/include/sbsa_val_interface.h"

#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_smmu.h"
#include "val/common/include/acs_pgt.h"
#include "val/common/include/acs_pe.h"
#include "val/sbsa/include/sbsa_acs_iovirt.h"
#include "val/common/include/acs_iovirt.h"
#include "val/sbsa/include/sbsa_acs_memory.h"
#include "val/common/include/acs_pcie_enumeration.h"
#include "val/sbsa/include/sbsa_acs_pcie.h"
#include "val/sbsa/include/sbsa_acs_exerciser.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 3)
#define TEST_DESC  "ATS Functionality Check           "
#define TEST_RULE  "RE_SMU_2"

#define TEST_DATA_NUM_PAGES  1
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
clear_dram_buf(void *buf, uint32_t size)
{

  uint32_t index;

  for (index = 0; index < size; index++) {
    *((char8_t *)buf + index) = 0;
  }

  val_data_cache_ops_by_va((addr_t)buf, CLEAN_AND_INVALIDATE);
}

static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t dma_len;
  uint32_t instance;
  uint32_t e_bdf;
  uint32_t erp_bdf;
  uint32_t rc_index;
  uint32_t cap_base;
  void *dram_buf_in_virt;
  void *dram_buf_out_virt;
  uint64_t dram_buf_in_phys;
  uint64_t dram_buf_out_phys;
  uint64_t dram_buf_in_iova;
  uint64_t dram_buf_out_iova;
  uint64_t m_vir_addr;
  uint32_t num_exercisers, num_smmus;
  uint32_t device_id, its_id;
  uint32_t page_size = val_memory_page_size();
  memory_region_descriptor_t mem_desc_array[2], *mem_desc;
  pgt_descriptor_t pgt_desc;
  smmu_master_attributes_t master;
  uint64_t ttbr;
  uint32_t test_data_blk_size = page_size * TEST_DATA_NUM_PAGES;
  uint64_t *pgt_base_array;
  uint64_t translated_addr;
  uint32_t test_skip = 1;
  uint32_t reg_value = 0;

  /* Initialize DMA master and memory descriptors */
  val_memory_set(&master, sizeof(master), 0);
  val_memory_set(mem_desc_array, sizeof(mem_desc_array), 0);
  mem_desc = &mem_desc_array[0];
  dram_buf_in_phys = 0;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  num_exercisers = val_exerciser_get_info(EXERCISER_NUM_CARDS);
  num_smmus = val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0);

  /* Allocate an array to store base addresses of page tables allocated for
   * all exercisers
   */
  pgt_base_array = val_aligned_alloc(MEM_ALIGN_4K, sizeof(uint64_t) * num_exercisers);
  if (!pgt_base_array) {
      val_print(ACS_PRINT_ERR, "\n       mem alloc failure", 0);
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 02));
      return;
  }

  val_memory_set(pgt_base_array, sizeof(uint64_t) * num_exercisers, 0);

  /* Allocate a buffer to perform DMA tests on */
  dram_buf_in_virt = val_memory_alloc_pages(TEST_DATA_NUM_PAGES);
  if (!dram_buf_in_virt) {
      val_print(ACS_PRINT_ERR, "\n       Cacheable mem alloc failure", 0);
      val_memory_free_aligned(pgt_base_array);
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 03));
      return;
  }

  /* Set the virtual and physical addresses for test buffers */
  dram_buf_in_phys = (uint64_t)val_memory_virt_to_phys(dram_buf_in_virt);
  dram_buf_out_virt = dram_buf_in_virt + (test_data_blk_size / 2);
  dram_buf_out_phys = dram_buf_in_phys + (test_data_blk_size / 2);
  dma_len = test_data_blk_size / 2;

  /* Get translation attributes via TCR and translation table base via TTBR */
  if (val_pe_reg_read_tcr(0 /*for TTBR0*/,
                          &pgt_desc.tcr)) {
    val_print(ACS_PRINT_ERR, "\n       TCR read failure", 0);
    goto test_fail;
  }

  if (val_pe_reg_read_ttbr(0 /*for TTBR0*/,
                           &ttbr)) {
    val_print(ACS_PRINT_ERR, "\n       TTBR0 read failure", 0);
    goto test_fail;
  }

  /* Enable all SMMUs */
  for (instance = 0; instance < num_smmus; ++instance)
     val_smmu_enable(instance);

  for (instance = 0; instance < num_exercisers; ++instance) {

    /* if init fail moves to next exerciser */
    if (val_exerciser_init(instance))
        continue;

    /* Get exerciser bdf */
    e_bdf = val_exerciser_get_bdf(instance);
    val_print(ACS_PRINT_DEBUG, "\n       Exerciser BDF - 0x%x", e_bdf);

    /* If ATS Capability Not Present, Skip. */
    if (val_pcie_find_capability(e_bdf, PCIE_ECAP, ECID_ATS, &cap_base) != PCIE_SUCCESS)
        continue;

    /* Get RP of the exerciser */
    if (val_pcie_get_rootport(e_bdf, &erp_bdf))
        continue;

    /* Get index for RC in IOVIRT mapping*/
    rc_index = val_iovirt_get_rc_index(PCIE_EXTRACT_BDF_SEG(erp_bdf));
    if (rc_index == ACS_INVALID_INDEX)
        continue;

    /* Continue further only if RC supports ATS - this is not standard but information
     * based on the SOC design */
    if (!val_iovirt_get_pcie_rc_info(RC_ATS_ATTRIBUTE, rc_index))
        continue;

    val_pcie_read_cfg(e_bdf, cap_base + ATS_CTRL, &reg_value);
    reg_value |= ATS_CACHING_EN;
    val_pcie_write_cfg(e_bdf, cap_base + ATS_CTRL, reg_value);

    pgt_desc.pgt_base = (ttbr & AARCH64_TTBR_ADDR_MASK);
    pgt_desc.mair = val_pe_reg_read(MAIR_ELx);
    pgt_desc.stage = PGT_STAGE1;

    /* Get memory attributes of the test buffer, we'll use the same attibutes to create
     * our own page table later.
     */
    if (val_pgt_get_attributes(pgt_desc, (uint64_t)dram_buf_in_virt, &mem_desc->attributes)) {
        val_print(ACS_PRINT_ERR, "\n       Unable to get memory attributes of the test buffer", 0);
        goto test_fail;
    }

    /* Get SMMU node index for this exerciser instance */
    master.smmu_index = val_iovirt_get_rc_smmu_index(PCIE_EXTRACT_BDF_SEG(e_bdf),
                                                     PCIE_CREATE_BDF_PACKED(e_bdf));

    clear_dram_buf(dram_buf_in_virt, test_data_blk_size);

    dram_buf_in_iova = dram_buf_in_phys;
    dram_buf_out_iova = dram_buf_out_phys;
    if (master.smmu_index != ACS_INVALID_INDEX &&
        val_iovirt_get_smmu_info(SMMU_CTRL_ARCH_MAJOR_REV, master.smmu_index) == 3) {
        if (val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(e_bdf),
                                       PCIE_EXTRACT_BDF_SEG(e_bdf),
                                       &device_id, &master.streamid,
                                       &its_id))
            continue;

        /* Each exerciser instance accesses a unique IOVA, which, because of SMMU translations,
         * will point to the same physical address. We create the requisite page tables and
         * configure the SMMU for each exerciser as such.
         */

        mem_desc->virtual_address = (uint64_t)dram_buf_in_virt + instance * test_data_blk_size;
        mem_desc->physical_address = dram_buf_in_phys;
        mem_desc->length = test_data_blk_size;
        mem_desc->attributes |= PGT_STAGE1_AP_RW;

        /* Need to know input and output address sizes before creating page table */
        pgt_desc.ias = val_smmu_get_info(SMMU_IN_ADDR_SIZE, master.smmu_index);
        if ((pgt_desc.ias) == 0) {
            val_print(ACS_PRINT_ERR,
                          "\n       Input address size of SMMU %d is 0", master.smmu_index);
            goto test_fail;
        }

        pgt_desc.oas = val_smmu_get_info(SMMU_OUT_ADDR_SIZE, master.smmu_index);
        if ((pgt_desc.oas) == 0) {
            val_print(ACS_PRINT_ERR,
                          "\n       Output address size of SMMU %d is 0", master.smmu_index);
            goto test_fail;
        }

        /* set pgt_desc.pgt_base to NULL to create new translation table, val_pgt_create
           will update pgt_desc.pgt_base to point to created translation table */
        pgt_desc.pgt_base = (uint64_t) NULL;
        if (val_pgt_create(mem_desc, &pgt_desc)) {
            val_print(ACS_PRINT_ERR,
                      "\n       Unable to create page table with given attributes", 0);
            goto test_fail;
        }

        pgt_base_array[instance] = pgt_desc.pgt_base;

        /* Configure the SMMU tables for this exerciser to use this page table
           for VA to PA translations*/
        if (val_smmu_map(master, pgt_desc))
        {
            val_print(ACS_PRINT_ERR, "\n       SMMU mapping failed (%x)     ", e_bdf);
            goto test_fail;
        }

        dram_buf_in_iova = mem_desc->virtual_address;
        dram_buf_out_iova = dram_buf_in_iova + (test_data_blk_size / 2);
    }

    test_skip = 0;

    val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)dram_buf_in_virt +
                            instance * test_data_blk_size, dma_len, instance);

    /* Send an ATS Translation Request for the VA */
    if (val_exerciser_ops(ATS_TXN_REQ, (uint64_t)dram_buf_in_virt + instance * test_data_blk_size,
                          instance)) {
        val_print(ACS_PRINT_ERR, "\n       ATS Translation Req Failed exerciser %4x", instance);
        goto test_fail;
    }

    /* Get ATS Translation Response */
    m_vir_addr = (uint64_t)dram_buf_in_virt + instance * test_data_blk_size;
    if (val_exerciser_get_param(ATS_RES_ATTRIBUTES, &translated_addr, &m_vir_addr, instance)) {
        val_print(ACS_PRINT_ERR, "\n       ATS Response failure %4x", instance);
        goto test_fail;
    }

    /* Compare Translated Addr with Physical Address from the Mappings */
    if (translated_addr != dram_buf_in_phys) {
        val_print(ACS_PRINT_ERR, "\n       ATS Translation failure %4x", instance);
        goto test_fail;
    }

    /* Initialize the sender buffer with test specific data */
    write_test_data(dram_buf_in_virt, dma_len);

    /* Configure Exerciser to issue subsequent DMA transactions with Address Translated bit Set */
    val_exerciser_set_param(CFG_TXN_ATTRIBUTES, TXN_ADDR_TYPE, AT_TRANSLATED, instance);

    if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_in_phys, dma_len, instance)) {
        val_print(ACS_PRINT_ERR, "\n       DMA attributes setting failure %4x", instance);
        goto test_fail;
    }

    /* Trigger DMA from input buffer to exerciser memory */
    val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance);

    if (val_exerciser_set_param(DMA_ATTRIBUTES, dram_buf_out_iova, dma_len, instance)) {
        val_print(ACS_PRINT_ERR, "\n       DMA attributes setting failure %4x", instance);
        goto test_fail;
    }

    /* Trigger DMA from exerciser memory to output buffer*/
    val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance);

    if (val_memory_compare(dram_buf_in_virt, dram_buf_out_virt, dma_len)) {
        val_print(ACS_PRINT_ERR, "\n       Data Comparasion failure for Exerciser %4x", instance);
        goto test_fail;
    }


    clear_dram_buf(dram_buf_in_virt, test_data_blk_size);

  }

  if (test_skip)
    val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  else
    val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));

  goto test_clean;

test_fail:
  val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));

test_clean:
  /* Return the pages to the heap manager */
  val_memory_free_pages(dram_buf_in_virt, TEST_DATA_NUM_PAGES);

  /* Remove all address mappings for each exerciser */
  for (instance = 0; instance < num_exercisers; ++instance)
  {
    e_bdf = val_exerciser_get_bdf(instance);
    master.smmu_index = val_iovirt_get_rc_smmu_index(PCIE_EXTRACT_BDF_SEG(e_bdf),
                                                     PCIE_CREATE_BDF_PACKED(e_bdf));
    if (val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(e_bdf),
                                   PCIE_EXTRACT_BDF_SEG(e_bdf),
                                   &device_id, &master.streamid,
                                   &its_id))
        continue;

    val_smmu_unmap(master);

    if (pgt_base_array[instance] != 0) {
      pgt_desc.pgt_base = pgt_base_array[instance];
      val_pgt_destroy(pgt_desc);
    }

    if (val_pcie_find_capability(e_bdf, PCIE_ECAP, ECID_ATS, &cap_base) == PCIE_SUCCESS)
    {
        val_pcie_read_cfg(e_bdf, cap_base + ATS_CTRL, &reg_value);
        reg_value &= ATS_CACHING_DIS;
        val_pcie_write_cfg(e_bdf, cap_base + ATS_CTRL, reg_value);
    }

  }

  /* Disable all SMMUs */
  for (instance = 0; instance < num_smmus; ++instance)
     val_smmu_disable(instance);

  val_memory_free_aligned(pgt_base_array);
}


uint32_t
e003_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
