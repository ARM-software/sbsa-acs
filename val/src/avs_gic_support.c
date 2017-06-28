/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

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
#include "include/sbsa_avs_gic_support.h"
#include "include/sbsa_avs_common.h"


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

