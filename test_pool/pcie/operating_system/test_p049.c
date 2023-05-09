/** @file
 * Copyright (c) 2020-2023, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 49)
#define TEST_DESC  "Check RootPort P Memory Access    "
#define TEST_RULE  "PCI_IN_13"

#define KNOWN_DATA  0xABABABAB

static void *branch_to_test;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t pe_index;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to return to test specified address */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(AVS_PRINT_INFO, "\n       Received exception of type: %d", interrupt_type);
  val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
}


static
uint32_t
check_bdf_under_rp(uint32_t rp_bdf)
{

  uint32_t reg_value;
  uint32_t rp_sec_bus, rp_sub_bus;
  uint32_t dev_bdf, dev_bus, dev_sec_bus;
  uint32_t rp_seg, dev_seg;
  uint32_t base_cc;
  uint32_t dev_num, func_num;

  rp_seg = PCIE_EXTRACT_BDF_SEG(rp_bdf);
  val_pcie_read_cfg(rp_bdf, TYPE1_PBN, &reg_value);
  rp_sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);
  rp_sub_bus = ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK);

  for (dev_sec_bus = rp_sec_bus; dev_sec_bus <= rp_sub_bus; dev_sec_bus++)
  {
      for (dev_num = 0; dev_num < PCIE_MAX_DEV; dev_num++)
      {
          for (func_num = 0; func_num < PCIE_MAX_FUNC; func_num++)
          {
              dev_bdf = PCIE_CREATE_BDF(rp_seg, dev_sec_bus, dev_num, func_num);
              val_pcie_read_cfg(dev_bdf, TYPE01_VIDR, &reg_value);
              if (reg_value == PCIE_UNKNOWN_RESPONSE)
                  continue;

              dev_bus = PCIE_EXTRACT_BDF_BUS(dev_bdf);
              dev_seg = PCIE_EXTRACT_BDF_SEG(dev_bdf);
              if ((dev_seg == rp_seg) && ((dev_bus >= rp_sec_bus) && (dev_bus <= rp_sub_bus)))
              {
                  val_pcie_read_cfg(dev_bdf, TYPE01_RIDR, &reg_value);
                  val_print(AVS_PRINT_DEBUG, "\n       Class code is %x", reg_value);
                  base_cc = reg_value >> TYPE01_BCC_SHIFT;
                  if ((base_cc == CNTRL_CC) || (base_cc == DP_CNTRL_CC) || (base_cc == MAS_CC))
                      return 1;
              }
           }
       }
  }

   return 0;
}

static
void
payload(void)
{

  uint32_t bdf;
  uint32_t dp_type;
  uint32_t pe_index;
  uint32_t tbl_index;
  uint32_t read_value, value;
  uint32_t old_value, new_value;
  uint32_t status;
  uint32_t test_skip = 1;
  uint32_t mem_offset = 0;
  uint64_t mem_base = 0, mem_base_upper = 0, ori_mem_base = 0;
  uint64_t mem_lim = 0, mem_lim_upper = 0, new_mem_lim;
  uint64_t updated_mem_base = 0, updated_mem_lim = 0;
  pcie_device_bdf_table *bdf_tbl_ptr;

  tbl_index = 0;
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Install sync and async handlers to handle exceptions.*/
  status = val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  status |= val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);
  if (status)
  {
      val_print(AVS_PRINT_ERR, "\n       Failed in installing the exception handler", 0);
      val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  branch_to_test = &&exception_return;

  /* Since this is a memory space access test.
   * Enable BME & MSE for all the BDFs.
  */
  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      /* Enable Bus Master Enable */
      val_pcie_enable_bme(bdf);
      /* Enable Memory Space Access */
      val_pcie_enable_msa(bdf);
  }

  tbl_index = 0;
  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      if (dp_type == iEP_RP) {

        /* Clearing UR in Device Status Register */
        val_pcie_clear_urd(bdf);

        /* Read Function's Memory Base Limit Register */
        val_pcie_read_cfg(bdf, TYPE1_P_MEM, &read_value);
        val_print(AVS_PRINT_DEBUG, "\n       BDF is 0x%x", bdf);
        if (read_value == 0)
          continue;

        mem_base = (read_value & MEM_BA_MASK) << MEM_BA_SHIFT;
        mem_lim = (read_value & MEM_LIM_MASK) | MEM_LIM_LOWER_BITS;

        /* If 64 Bit Prefetchable Address */
        if ((read_value & P_MEM_PAC_MASK) == 0x1) {
          val_pcie_read_cfg(bdf, TYPE1_P_MEM_BU, &read_value);
          mem_base_upper = read_value;
          val_pcie_read_cfg(bdf, TYPE1_P_MEM_LU, &read_value);
          mem_lim_upper = read_value;
        }

        mem_base |= (mem_base_upper << P_MEM_BU_SHIFT);
        mem_lim |= (mem_lim_upper << P_MEM_LU_SHIFT);

        val_print(AVS_PRINT_DEBUG, "\n       Memory base is 0x%llx", mem_base);
        val_print(AVS_PRINT_DEBUG, " Memory lim is  0x%llx", mem_lim);

        /* If Memory Limit is programmed with value less the Base, then Skip.*/
        if (mem_lim < mem_base) {
            val_print(AVS_PRINT_DEBUG,
                      "\n       Memory limit < Memory Base. Skipping Bdf - 0x%x", bdf);
            continue;
        }

        /* If test runs for atleast an endpoint */
        test_skip = 0;

        /* Check_1: Accessing address in range of P memory
         * mjust not cause any exception or data abort
         *
         * Write known value to an address which is in range
         * Base + offset must always be in the range.
         * Read the same
        */
        mem_offset = val_pcie_mem_get_offset(MEM_OFFSET_MEDIUM);

        if ((mem_base + mem_offset) > mem_lim)
        {
            val_print(AVS_PRINT_ERR, "\n       Memory offset + base 0x%x ", mem_base + mem_offset);
            val_print(AVS_PRINT_ERR, "exceeds the memory limit 0x%x", mem_lim);
            val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
            return;
        }

        old_value = (*(volatile addr_t *)(mem_base + mem_offset));
        *(volatile addr_t *)(mem_base + mem_offset) = KNOWN_DATA;
        read_value = (*(volatile addr_t *)(mem_base + mem_offset));

        if ((old_value != read_value && read_value == PCIE_UNKNOWN_RESPONSE) ||
             val_pcie_is_urd(bdf)) {
          val_print(AVS_PRINT_DEBUG, "\n       Value written into memory - 0x%x", KNOWN_DATA);
          val_print(AVS_PRINT_DEBUG, "\n       Value in memory after write - 0x%x", read_value);
          val_print(AVS_PRINT_ERR, "\n       Memory access check failed for BDF  0x%x", bdf);

          val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
          val_pcie_clear_urd(bdf);
          return;
        }

        /** Skip Check_2 if there is an Ethernet or Display controller
         * under the RP device
         **/
        if (check_bdf_under_rp(bdf))
        {
            val_print(AVS_PRINT_DEBUG, "\n       Skipping for RP BDF %x", bdf);
            continue;
        }

        /**Check_2: Accessing out of P memory limit range must return 0xFFFFFFFF
         *
         * If the limit exceeds 1MB then modify the range to be 1MB
         * and access out of the limit set
         **/
        ori_mem_base = mem_base;

        if ((mem_lim >> MEM_SHIFT) > (mem_base >> MEM_SHIFT))
        {
           val_print(AVS_PRINT_DEBUG, "\n       Entered Check_2 for bdf %x", bdf);
           new_mem_lim  = mem_base + MEM_OFFSET_LARGE;
           val_pcie_read_cfg(bdf, TYPE1_P_MEM, &new_value);

          if ((new_value & P_MEM_PAC_MASK) == 0x1)
               val_pcie_write_cfg(bdf, TYPE1_P_MEM_LU, (mem_base >> 32));

           mem_base = ((uint32_t)mem_base) | ((uint32_t)mem_base >> 16);
           val_print(AVS_PRINT_INFO, " mem_base new is 0x%llx", mem_base);
           val_pcie_write_cfg(bdf, TYPE1_P_MEM, mem_base);

           val_pcie_read_cfg(bdf, TYPE1_P_MEM, &read_value);
           updated_mem_base = (read_value & MEM_BA_MASK) << MEM_BA_SHIFT;
           updated_mem_lim = (read_value & MEM_LIM_MASK) | MEM_LIM_LOWER_BITS;

           /* If 64 Bit Prefetchable Address */
           if ((read_value & P_MEM_PAC_MASK) == 0x1) {
             val_pcie_read_cfg(bdf, TYPE1_P_MEM_BU, &read_value);
             mem_base_upper = read_value;
             val_pcie_read_cfg(bdf, TYPE1_P_MEM_LU, &read_value);
             mem_lim_upper = read_value;
           }

           updated_mem_base |= (mem_base_upper << P_MEM_BU_SHIFT);
           updated_mem_lim |= (mem_lim_upper << P_MEM_LU_SHIFT);

           value = (*(volatile uint32_t *)(new_mem_lim + MEM_OFFSET_SMALL));
           val_print(AVS_PRINT_DEBUG, "       Value read is 0x%llx", value);
           if (value != PCIE_UNKNOWN_RESPONSE)
           {
               val_print(AVS_PRINT_ERR, "\n       Memory range for bdf 0x%x", bdf);
               val_print(AVS_PRINT_ERR, " is 0x%llx", updated_mem_base);
               val_print(AVS_PRINT_ERR, " 0x%llx", updated_mem_lim);
               val_print(AVS_PRINT_ERR,
                        "\n      Out of range 0x%llx", (new_mem_lim + MEM_OFFSET_SMALL));
               val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
           }
        }

exception_return:
        /*Write back original value */
        if ((mem_lim >> MEM_SHIFT) > (ori_mem_base >> MEM_SHIFT))
        {
            val_pcie_write_cfg(bdf, TYPE1_P_MEM,
                                              ((mem_lim & MEM_LIM_MASK) | (ori_mem_base  >> 16)));
            val_pcie_write_cfg(bdf, TYPE1_P_MEM_LU, (mem_lim >> 32));
        }

        /* Memory Space might have constraint on RW/RO behaviour
         * So not checking for Read-Write Data mismatch.
        */
        if (IS_TEST_FAIL(val_get_status(pe_index))) {
          val_print(AVS_PRINT_ERR, "\n       Failed exception on Mem Access For Bdf: 0x%x", bdf);
          val_pcie_clear_urd(bdf);
          return;
        }

      }
  }

  if (test_skip == 1) {
      val_print(AVS_PRINT_DEBUG,
        "\n       No iEP_RP type device found with valid Memory Base/Limit Reg.", 0);
      val_print(AVS_PRINT_DEBUG, "\n       Skipping Test", 0);
      val_set_status(pe_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
  }
  else
      val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

uint32_t
p049_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
