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

GCC_ASM_EXPORT (ArmCallWFI)
GCC_ASM_EXPORT (SpeProgramUnderProfiling)
GCC_ASM_EXPORT (DisableSpe)
GCC_ASM_EXPORT (BigEndianCheck)

ASM_PFX(ArmCallWFI):
  wfi
  ret

ASM_PFX(SpeProgramUnderProfiling):
  mov   x2,#12    // No of instructions in the loop
  udiv  x2,x0,x2  //iteration count = interval/(no of instructions in loop)
  add   x2,x2,#6  //add a tolerance above which profiler is guaranteed to generate event
ASM_PFX(label_if_not_zero):
ASM_PFX(loop):
  ldr   x0,[x1],#8
  str   x0,[x1],#8
  cmp   x0,#0
  bne   ASM_PFX(label_if_not_zero)
  ldr   x0,[x1],#8
  str   x0,[x1],#8
  cmp   x0,#0
  bne   ASM_PFX(label_if_not_zero)
  ldr   x0,[x1],#8
  str   x0,[x1],#8
  cmp   x0,#0
  bne   ASM_PFX(label_if_not_zero)
  sub   x2,x2,#1
  cbnz  x2,ASM_PFX(loop)
  ret

ASM_PFX(DisableSpe):
  //mrs   x0,pmscr_el2
  bic   x0,x0,#1
  //msr   pmscr_el2,x0
  isb
  //psb   csync
  //dci   0xD503223F  // opcode of psb csync
  dsb   sy
  //mrs   x0,pmblimitr_el1
  bic   x0,x0,#1
  //msr   pmblimitr_el1,x0
  isb

  ret

ASM_PFX(BigEndianCheck):
  ldr   x1,[x0]
  ubfx  x1,x1,#24,#8
  mrs   x2,sctlr_el2
  ldr   x3,=0x02000000
  eor   x2,x2,x3
  msr   sctlr_el2,x2  // Flip the endianness
  dsb   sy
  isb
  ldr   x4,[x0]
  ubfx  x4,x4,#32,#8
  cmp   x1,x4
  cset  x0,eq

  eor   x2,x2,x3
  msr   sctlr_el2,x2  // Restore the endianness
  dsb   sy
  isb

  ret

