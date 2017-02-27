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

#ifndef __GIC_SYS_REGS_H__
#define __GIC_SYS_REGS_H__

typedef enum {
  ICH_HCR_EL2 = 0,
  ICH_MISR_EL2,
  ICC_IGRPEN1_EL1,
  ICC_BPR1_EL1,
  ICC_PMR_EL1
}SBSA_AVS_GIC_REGS;

uint64_t val_gic_reg_read(uint32_t reg_id);
void     val_gic_reg_write(uint32_t reg_id, uint64_t write_data);

uint64_t GicReadIchHcr(void);
uint64_t GicReadIchMisr(void);

void GicWriteIchHcr(uint64_t write_data);
void GicWriteIccIgrpen1(uint64_t write_data);
void GicWriteIccBpr1(uint64_t write_data);
void GicWriteIccPmr(uint64_t write_data);


#endif // __GIC_SYS_REGS_H__
