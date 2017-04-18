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

GCC_ASM_IMPORT(ArmReadMpidr)
GCC_ASM_IMPORT(PalGetSecondaryStackBase)
GCC_ASM_IMPORT(PalGetMaxMpidr)
GCC_ASM_EXPORT(ModuleEntryPoint)

StartupAddr:         .8byte ASM_PFX(val_test_entry)
ASM_PFX(StackSize):  .8byte 0x100

ASM_PFX(ModuleEntryPoint):
  // Get ID of this CPU in Multicore system
  bl    ASM_PFX(ArmReadMpidr)
  // Keep a copy of the MpId register value
  mov   x10, x0

  // With this function: CorePos = (Aff3 x maxAff2 x maxAff1 x maxAff0) +
  //                                 (Aff2 x maxAff1 x maxAff0) + (Aff1 x maxAff0) + Aff0
  and   x1, x0, 0xFF
  and   x2, x0, 0xFF00
  lsr   x2, x2, 8
  and   x3, x0, 0xFF0000
  lsr   x3, x3, 16
  and   x4, x0, 0xFF00000000
  lsr   x4, x4, 32

  bl    ASM_PFX(PalGetMaxMpidr)
  and   x5, x0, 0xFF
  add   x5, x5, 1
  and   x6, x0, 0xFF00
  lsr   x6, x6, 8
  add   x6, x6, 1
  and   x7, x0, 0xFF0000
  lsr   x7, x7, 16
  add   x7, x7, 1
  and   x8, x0, 0xFF00000000
  lsr   x8, x8, 32
  add   x8, x8, 1    // x8 has maxAff3, which is reserved for future use

  madd  x0, x2, x5, x1
  mul   x5, x5, x6
  madd  x0, x3, x5, x0
  mul   x5, x5, x7
  madd  x0, x4, x5, x0

  ldr   x3, StackSize
  mul   x3, x3, x0
_GetStackBase:
  mov   x0, 0
  //ldr   x4, GetStackMem
  //blr   x4
  //ASM_PFX(pal_allocate_stack)
  bl ASM_PFX(PalGetSecondaryStackBase)
  add   x0, x0, x3
  mov   sp, x0
_PrepareArguments:

  ldr   x4, StartupAddr

  blr   x4

_NeverReturn:
  b _NeverReturn
