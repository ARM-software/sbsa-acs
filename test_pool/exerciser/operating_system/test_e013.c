
/** @file
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
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

#include "val/common/include/acs_pgt.h"
#include "val/common/include/acs_pe.h"
#include "val/common/include/acs_iovirt.h"
#include "val/sbsa/include/sbsa_acs_memory.h"
#include "val/sbsa/include/sbsa_acs_pcie.h"
#include "val/sbsa/include/sbsa_acs_exerciser.h"
#include "val/sbsa/include/sbsa_acs_smmu.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 13)
#define TEST_DESC  "Enable and disable STE.DCP bit        "
#define TEST_RULE  "S_PCIe_10"

#define TEST_DATA_NUM_PAGES  4

static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t instance, test_skip;
  uint32_t e_bdf, dcp_bit;
  uint32_t num_exercisers, num_smmus;
  uint32_t device_id, its_id;
  uint32_t smmu_minor;
  memory_region_descriptor_t mem_desc_array[2], *mem_desc;
  pgt_descriptor_t pgt_desc;
  smmu_master_attributes_t master;
  uint64_t ttbr;
  uint64_t *pgt_base_array;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  num_exercisers = val_exerciser_get_info(EXERCISER_NUM_CARDS);
  num_smmus = val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0);
  test_skip = 1;

  /* Initialize DMA master and memory descriptors */
  val_memory_set(&master, sizeof(master), 0);
  val_memory_set(mem_desc_array, sizeof(mem_desc_array), 0);
  mem_desc = &mem_desc_array[0];

  /* Allocate an array to store base addresses of page tables allocated for
   * all exercisers
   */
  pgt_base_array = val_aligned_alloc(MEM_ALIGN_4K, sizeof(uint64_t) * num_exercisers);
  if (!pgt_base_array) {
      val_print(ACS_PRINT_ERR, "\n       mem alloc failure %x", 03);
      val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 03));
      return;
  }

  val_memory_set(pgt_base_array, sizeof(uint64_t) * num_exercisers, 0);

  /* Get translation attributes via TCR and translation table base via TTBR */
  if (val_pe_reg_read_tcr(0 /*for TTBR0*/, &pgt_desc.tcr)) {
    val_print(ACS_PRINT_ERR, "\n       Unable to get translation attributes via TCR", 0);
    goto test_fail;
  }

  if (val_pe_reg_read_ttbr(0 /*TTBR0*/, &ttbr)) {
    val_print(ACS_PRINT_ERR, "\n       Unable to get translation table via TBBR", 0);
    goto test_fail;
  }

  pgt_desc.pgt_base = (ttbr & AARCH64_TTBR_ADDR_MASK);
  pgt_desc.mair = val_pe_reg_read(MAIR_ELx);
  pgt_desc.stage = PGT_STAGE1;

  /* Enable all SMMUs */
  for (instance = 0; instance < num_smmus; ++instance)
     val_smmu_enable(instance);

  for (instance = 0; instance < num_exercisers; ++instance) {

    /* if init fail moves to next exerciser */
    if (val_exerciser_init(instance))
        continue;

    /* Get exerciser bdf */
    e_bdf = val_exerciser_get_bdf(instance);
    val_print(ACS_PRINT_DEBUG, "\n       Exercise BDF - 0x%x", e_bdf);

    /* Get SMMU node index for this exerciser instance */
    master.smmu_index = val_iovirt_get_rc_smmu_index(PCIE_EXTRACT_BDF_SEG(e_bdf),
                                                     PCIE_CREATE_BDF_PACKED(e_bdf));

    if (master.smmu_index != ACS_INVALID_INDEX &&
        val_iovirt_get_smmu_info(SMMU_CTRL_ARCH_MAJOR_REV, master.smmu_index) == 3) {

        /* DCP bit is RES0 for SMMUv3, hence check only for SMMU verion greater than 3.0 */
        smmu_minor = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_AIDR, master.smmu_index), 0, 3);
        if (smmu_minor == 0) {
            val_print(ACS_PRINT_DEBUG, "\n       SMMU version is v3.%d", smmu_minor);
            continue;
        }

        if (val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(e_bdf),
                                       PCIE_EXTRACT_BDF_SEG(e_bdf),
                                       &device_id, &master.streamid,
                                       &its_id))
            continue;

        test_skip = 0;

        /* Need to know input and output address sizes before creating page table */
        pgt_desc.ias = val_smmu_get_info(SMMU_IN_ADDR_SIZE, master.smmu_index);
        if (pgt_desc.ias == 0) {
          val_print(ACS_PRINT_ERR,
                    "\n       Input address size of SMMU %d is 0", master.smmu_index);
          goto test_fail;
        }

        pgt_desc.oas = val_smmu_get_info(SMMU_OUT_ADDR_SIZE, master.smmu_index);
        if (pgt_desc.oas == 0) {
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
           for VA to PA translations */
        if (val_smmu_map(master, pgt_desc))
        {
            val_print(ACS_PRINT_ERR,
                     "\n       SMMU mapping failed (%x)     ", e_bdf);
            goto test_fail;
        }

       /* Set the STE.DCP bit to enable Direct Cache Prefetch transactions
        * Fail if not set
       */
       dcp_bit = val_smmu_config_ste_dcp(master, 1);
       if (!dcp_bit) {
           val_print(ACS_PRINT_ERR, "\n      STE.DCP bit not set", 0);
           goto test_fail;
       }

       /* Set the STE.DCP bit to disable Direct Cache Prefetch transactions
        * Fail if set
       */
       dcp_bit = val_smmu_config_ste_dcp(master, 0);
       if (dcp_bit) {
           val_print(ACS_PRINT_ERR, "\n      STE.DCP bit set", 0);
           goto test_fail;
       }
    }

  }

  val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));
  goto test_clean;

test_fail:
  val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 02));

test_clean:

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));

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
  }

  /* Disable all SMMUs */
  for (instance = 0; instance < num_smmus; ++instance)
     val_smmu_disable(instance);

  val_memory_free_aligned(pgt_base_array);
}


uint32_t
e013_entry(void)
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
