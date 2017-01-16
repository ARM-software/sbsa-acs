#/** @file
# Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.
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

.text
.align 3

GCC_ASM_EXPORT(UpdateElr)
GCC_ASM_EXPORT(DataCacheCleanInvalidateVA)

#define FP_CONTEXT_SIZE   0x200

ASM_PFX(DataCacheCleanInvalidateVA):
  dc civac, x0
  dsb ish
  ret

ASM_PFX(UpdateElr):
  // This function returns the stacked address of ELR in UEFI exception path
  // The sequence of context save before the control is handed over
  // to test-specific handler includes:
  // 1. General Purpose registers, occupying 0x100 bytes (32*8)
  // 2. Floating Point registers, occupying 0x200 bytes (32*16)
  // 3. System Registers, occupying 0x30 (6*8) - ELR,SPSR,FPSR,ESR,FAR
  // X28 points to start of FP context, and an offset of 0x200 should be
  // added to make it point to system register context and
  // ELR is the first register stacked there.
  // For more info on the calculation of stacked address, please refer following file:
  // <EDK2_PATH>/ArmPkg/Library/ArmExceptionLib/AArch64/ExceptionSupport.S

  add   x1, x28, #FP_CONTEXT_SIZE
  str   x0, [x1]    // Update the stacked location with user address

  ret
