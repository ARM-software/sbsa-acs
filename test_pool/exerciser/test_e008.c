/** @file
 * Copyright (c) 2019-2023, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_pcie_enumeration.h"
#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_smmu.h"
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_exerciser.h"

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 8)
#define TEST_DESC  "Check BME functionality of RP     "
#define TEST_RULE  ""

#define TEST_DATA_NUM_PAGES  1

static void *branch_to_test;

/*
 * Execption is not expected in this test scenario.
 * The handler is present just as a fail-safe mechanism.
 */
static
void
esr(uint64_t interrupt_type, void *context)
{

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(AVS_PRINT_INFO, "\n       Received exception of type: %d", interrupt_type);
}



static
void
payload(void)
{

  uint32_t pe_index;
  uint32_t e_bdf;
  uint32_t erp_bdf;
  uint32_t instance;
  uint64_t bar_base;
  uint32_t fail_cnt;
  uint32_t smmu_index;
  uint32_t dma_len;
  uint32_t status;
  uint32_t reg_value;
  void *dram_buf_virt;
  void *dram_buf_phys;
  void *dram_buf_iova;
  uint32_t page_size = val_memory_page_size();

  fail_cnt = 0;
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  /* Install sync and async handlers to handle exceptions.*/
  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status)
  {
      val_print(AVS_PRINT_ERR, "\n      Failed in installing the exception handler", 0);
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  branch_to_test = &&exception_return;

  /* Create a buffer of size TEST_DMA_SIZE in DRAM */
  dram_buf_virt = val_memory_alloc_pages(TEST_DATA_NUM_PAGES);

  if (!dram_buf_virt)
  {
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
      return;
  }

  dram_buf_phys = val_memory_virt_to_phys(dram_buf_virt);
  dma_len = page_size * TEST_DATA_NUM_PAGES;;

  while (instance-- != 0) {

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

     e_bdf = val_exerciser_get_bdf(instance);

      /* Skip this exerciser if it doesn't have mmio BAR */
      val_pcie_get_mmio_bar(e_bdf, &bar_base);
      if (!bar_base)
          continue;

      /*
       * Disable Bus Master Enable bit in Exierciser upstream Root
       * Port Command Register. This bit controls forwarding of
       * Memory Requests by a Root Port in the Upstream direction.
       * When this bit is 0b, Memory Requests received at a Root Port
       * must be handled as Unsupported Requests (UR).
       */
      if (!val_pcie_get_rootport(e_bdf, &erp_bdf))
          val_pcie_disable_bme(erp_bdf);
      else
          continue;

      /* Disable error reporting of Exerciser upstream Root Port */
      val_pcie_disable_eru(erp_bdf);

      /*
       * Clear unsupported request detected bit in Exerciser upstream
       * Rootport's Device Status Register to clear any pending urd status.
       */
      val_pcie_clear_urd(erp_bdf);

      /*
       * Get SMMU node index for this exerciser instance to convert
       * the dram physical addresses to IOVA addresses for DMA purposes.
       */
      smmu_index = val_iovirt_get_rc_smmu_index(PCIE_EXTRACT_BDF_SEG(e_bdf), PCIE_CREATE_BDF_PACKED(e_bdf));
      if (smmu_index == AVS_INVALID_INDEX)
          dram_buf_iova = dram_buf_phys;
      else
          dram_buf_iova = (void *) val_smmu_pa2iova(smmu_index, (uint64_t)dram_buf_phys);

      /*
       * Issue a Memory Read request from exerciser to cause unsupported
       * request detected bit set in exercise's Device Status Register.
       * Based on platform configuration, this may even cause a
       * sync/async exception.
       */
      val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)dram_buf_iova, dma_len, instance);
      val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance);

exception_return:
      /* Check if UR detected bit isn't set in the Root Port */
      if (val_pcie_is_urd(erp_bdf))
      {
          /* Clear urd bit in Device Status Register */
          val_pcie_clear_urd(erp_bdf);
      } else
      {
          val_print(AVS_PRINT_ERR, "\n      Root Port BDF 0x%x BME functionality failure", erp_bdf);
          fail_cnt++;
      }

      /* Restore Rootport Bus Master Enable */
      val_pcie_enable_bme(erp_bdf);


      /* Check if Received Master Abort bit is set in the Exerciser */
      val_pcie_read_cfg(e_bdf, COMMAND_REG_OFFSET, &reg_value);
      if (!((reg_value & MASTER_ABORT_MASK) >> MASTER_ABORT_SHIFT))
      {
          val_print(AVS_PRINT_ERR, "\n      Exerciser BDF 0x%x BME functionality failure", e_bdf);
          fail_cnt++;
      }


  }

  /* Return the buffer to the heap manager */
  val_memory_free_pages(dram_buf_virt, TEST_DATA_NUM_PAGES);

  if (fail_cnt)
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, fail_cnt));
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

  return;

}

uint32_t
e008_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
