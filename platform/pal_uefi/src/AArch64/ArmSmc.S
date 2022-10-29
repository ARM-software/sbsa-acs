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

.text
.align 3

GCC_ASM_IMPORT(gPsciConduit)
.equ CONDUIT_SMC,        0
.equ CONDUIT_HVC,        1
.equ INVALID_PARAMETER, -3

GCC_ASM_EXPORT(ArmCallSmc)

ASM_PFX(ArmCallSmc):
  // Push x0 on the stack - The stack must always be quad-word aligned
  str   x0, [sp, #-16]!

  // Load the conduit into x10
  mov   x10, x1

  // Load the SMC arguments values into the appropriate registers
  ldp   x6, x7, [x0, #48]
  ldp   x4, x5, [x0, #32]
  ldp   x2, x3, [x0, #16]
  ldp   x0, x1, [x0, #0]

  // Check whether the conduit is SMC or HVC
  cmp   x10, CONDUIT_SMC
  beq   conduit_smc
  cmp   x10, CONDUIT_HVC
  beq   conduit_hvc

conduit_invalid:
  mov   x0, INVALID_PARAMETER
  b     finish

conduit_smc:
  smc   #0
  b     finish

conduit_hvc:
  hvc   #0
  b     finish

finish:
  // Pop the ARM_SMC_ARGS structure address from the stack into x9
  ldr   x9, [sp], #16

  // Store the SMC returned values into the ARM_SMC_ARGS structure.
  // A SMC call can return up to 4 values - we do not need to store back x4-x7.
  stp   x2, x3, [x9, #16]
  stp   x0, x1, [x9, #0]

  mov   x0, x9

  ret
