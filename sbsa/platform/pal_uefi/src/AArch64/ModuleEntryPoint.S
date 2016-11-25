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
GCC_ASM_EXPORT(ModuleEntryPoint)

StartupAddr:         .8byte ASM_PFX(val_test_entry)
ASM_PFX(StackSize):  .8byte 0x100

ASM_PFX(ModuleEntryPoint):
  // Get ID of this CPU in Multicore system
  bl    ASM_PFX(ArmReadMpidr)
  // Keep a copy of the MpId register value
  mov   x10, x0

  // With this function: CorePos = (ClusterId * 4) + CoreId
  and   x1, x0, 0xFF
  and   x0, x0, 0xFF00
  add   x0, x1, x0, LSR #6

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
