#/** @file
# Copyright (c) 2016-2018, Arm Limited or its affiliates. All rights reserved.
# SPDX-License-Identifier : Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#**/

#include "gcc_types.h"

.text
.align 2

GCC_ASM_EXPORT(GicReadIchHcr)
GCC_ASM_EXPORT(GicWriteIchHcr)
GCC_ASM_EXPORT(GicReadIchMisr)
GCC_ASM_EXPORT(GicWriteIccIgrpen1)
GCC_ASM_EXPORT(GicWriteIccBpr1)
GCC_ASM_EXPORT(GicWriteIccPmr)
GCC_ASM_EXPORT(GicClearDaif)
GCC_ASM_EXPORT(GicWriteHcr)
GCC_ASM_EXPORT(TestExecuteBarrier)

ASM_PFX(GicReadIchHcr):
  //mrs_s   x0, ich_hcr_el2
  .inst 0xd53ccb00
  ret

ASM_PFX(GicWriteIchHcr):
  //msr   ich_hcr_el2, x0
  .inst 0xd51ccb00
  isb
  ret

ASM_PFX(GicReadIchMisr):
  //mrs   x0, ich_misr_el2
  .inst 0xd53ccb40
  ret

ASM_PFX(GicWriteIccIgrpen1):
  //msr   icc_igrpen1_el1, x0
  .inst 0xd518cce0
  isb
  ret

ASM_PFX(GicWriteIccBpr1):
  //msr   icc_brp1_el1, x0
  .inst 0xd518cc60
  isb
  ret

ASM_PFX(GicWriteIccPmr):
  //msr   icc_pmr_el1, x0
  .inst 0xd5184600
  isb
  ret

ASM_PFX(GicClearDaif):
  msr      daifclr, 0x7
  isb
  ret

ASM_PFX(GicWriteHcr):
  msr   hcr_el2, x0
  ret

ASM_PFX(TestExecuteBarrier):
  dsb sy
  isb
  ret

