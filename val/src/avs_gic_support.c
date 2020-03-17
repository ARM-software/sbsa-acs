/** @file
 * Copyright (c) 2016-2020, Arm Limited or its affiliates. All rights reserved.
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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_gic.h"
#include "include/sbsa_avs_gic_support.h"
#include "include/sbsa_avs_common.h"


#ifndef TARGET_LINUX
/**
  @brief   This API provides a 'C' interface to call GIC System register reads
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   reg_id  - the system register index for which data is returned
  @return  the value read from the system register.
**/
uint64_t
val_gic_reg_read(uint32_t reg_id)
{

  switch(reg_id) {
      case ICH_HCR_EL2:
          return GicReadIchHcr();
      case ICH_MISR_EL2:
          return GicReadIchMisr();
      default:
           val_report_status(val_pe_get_index_mpid(val_pe_get_mpid()), RESULT_FAIL(g_sbsa_level, 0, 0x78));
  }

  return 0x0;

}

/**
  @brief   This API provides a 'C' interface to call GIC System register writes
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   reg_id  - the system register index for which data is written
  @param   write_data - the 64-bit data to write to the system register
  @return  None
**/
void
val_gic_reg_write(uint32_t reg_id, uint64_t write_data)
{

  switch(reg_id) {
      case ICH_HCR_EL2:
          GicWriteIchHcr(write_data);
          break;
      case ICC_IGRPEN1_EL1:
          GicWriteIccIgrpen1(write_data);
          break;
      case ICC_BPR1_EL1:
          GicWriteIccBpr1(write_data);
          break;
      case ICC_PMR_EL1:
          GicWriteIccPmr(write_data);
          break;
      default:
           val_report_status(val_pe_get_index_mpid(val_pe_get_mpid()), RESULT_FAIL(g_sbsa_level, 0, 0x78));
  }

}
#endif

/**
  @brief   This function is installs the ISR pointed by the function pointer
           the input Interrupt ID.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID to install the ISR
  @param   isr    Function pointer of the ISR
  @return  status
**/
uint32_t
val_gic_install_isr(uint32_t int_id, void (*isr)(void))
{
  uint32_t      ret_val;
#ifndef TARGET_LINUX
  uint32_t      reg_offset = int_id / 32;
  uint32_t      reg_shift  = int_id % 32;

  if ((int_id > val_get_max_intid()) || (int_id == 0)) {
      val_print(AVS_PRINT_ERR, "\n    Invalid Interrupt ID number %d ", int_id);
      return AVS_STATUS_ERR;
  }
#endif

  ret_val = pal_gic_install_isr(int_id, isr);

#ifndef TARGET_LINUX
  if (int_id > 31) {
      /**** UEFI GIC code is not enabling interrupt in the Distributor ***/
      /**** So, do this here as a fail-safe. Remove if PAL guarantees this ***/
      val_mmio_write(val_get_gicd_base() + GICD_ISENABLER + (4 * reg_offset), 1 << reg_shift);
  }
#endif

  return ret_val;
}

/**
 * @brief   This function registers the specified interrupt
 * @param   irq_num         hardware irq number
 * @param   mapped_irq_num  Mapped irq number
 * @param   isr             Interrupt service routine
 * @return  status
 */
uint32_t val_gic_request_irq(uint32_t irq_num, uint32_t mapped_irq_num, void *isr)
{
  return pal_gic_request_irq(irq_num, mapped_irq_num, isr);
}

/**
 * @brief   This function free the registered interrupt line
 * @param   irq_num         hardware irq number
 * @param   mapped_irq_num  Mapped irq number
 * @return  none
 */
void val_gic_free_irq(uint32_t irq_num, uint32_t mapped_irq_num)
{
    pal_gic_free_irq(irq_num, mapped_irq_num);
}

/**
  @brief   This function writes to end of interrupt register for relevant
           interrupt group.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID for which to disable the interrupt
  @return  status
**/
uint32_t val_gic_end_of_interrupt(uint32_t int_id)
{
  pal_gic_end_of_interrupt(int_id);

  return 0;
}

/**
  @brief   This function clear the MSI related mappings.

  @param   bdf          B:D:F for the device
  @param   IntID        Interrupt ID
  @param   msi_index    msi index in the table

  @return  status
**/
void val_gic_free_msi(uint32_t bdf, uint32_t IntID, uint32_t msi_index)
{
    pal_gic_free_msi(bdf, IntID, msi_index);
}

/**
  @brief   This function creates the MSI mappings, and programs the MSI Table.

  @param   bdf          B:D:F for the device
  @param   IntID        Interrupt ID
  @param   msi_index    msi index in the table

  @return  status
**/
uint32_t val_gic_request_msi(uint32_t bdf, uint32_t IntID, uint32_t msi_index)
{
  return pal_gic_request_msi(bdf, IntID, msi_index);
}
