#/** @file
# Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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
#include "SbsaAvs.h"

GCC_ASM_EXPORT(EnableMMU)
GCC_ASM_EXPORT(DisableMMU)

ASM_PFX(DisableMMU):
  MRS X0, SCTLR_EL1             // Read System Control Register configuration data
  AND X0, X0, #DISABLE_MMU_BIT  // Set [M] bit and enable the MMU.
  MSR SCTLR_EL1, X0             // Write System Control Register configuration data
  ISB                           // The ISB forces these changes to be seen by the next instruction

  RET

ASM_PFX(EnableMMU):

  // Enable MMU.

  MSR TTBR0_EL1, X0         // Set TTBR0
  MSR TTBR1_EL1, X1         // Set TTBR1
  MSR TCR_EL1, X2           // Set TCR
  ISB                       // The ISB forces these changes to be seen before the MMU is enabled

  MRS X0, SCTLR_EL1         // Read System Control Register configuration data
  ORR X0, X0, #1            // Set [M] bit and enable the MMU.
  MSR SCTLR_EL1, X0         // Write System Control Register configuration data
  ISB                       // The ISB forces these changes to be seen by the next instruction

  RET