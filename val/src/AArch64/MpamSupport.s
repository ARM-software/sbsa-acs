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

//
// Private worker functions for ASM_PFX()
//
#define _CONCATENATE(a, b)  __CONCATENATE(a, b)
#define __CONCATENATE(a, b) a ## b

#define __USER_LABEL_PREFIX__
//
// The __USER_LABEL_PREFIX__ macro predefined by GNUC represents the prefix
// on symbols in assembly language.
//
#define ASM_PFX(name) _CONCATENATE (__USER_LABEL_PREFIX__, name)

#define GCC_ASM_EXPORT(func__)  \
       .global  _CONCATENATE (__USER_LABEL_PREFIX__, func__)    ;\
       .type ASM_PFX(func__), %function

#define GCC_ASM_IMPORT(func__)  \
       .extern  _CONCATENATE (__USER_LABEL_PREFIX__, func__)

.text
.align 2

GCC_ASM_EXPORT(AA64ReadMpam1)
GCC_ASM_EXPORT(AA64WriteMpam1)
GCC_ASM_EXPORT(AA64ReadMpam2)
GCC_ASM_EXPORT(AA64WriteMpam2)
GCC_ASM_EXPORT(AA64ReadMpamidr)
GCC_ASM_EXPORT(AA64IssueDSB)


ASM_PFX(AA64ReadMpam1):
  mrs  x0, mpam1_el1
  ret

ASM_PFX(AA64WriteMpam1):
  msr   mpam1_el1, x0
  ret

ASM_PFX(AA64ReadMpam2):
  mrs  x0, mpam2_el2
  ret

ASM_PFX(AA64WriteMpam2):
  msr   mpam2_el2, x0
  ret

ASM_PFX(AA64ReadMpamidr):
  mrs  x0, mpamidr_el1
  ret

ASM_PFX(AA64IssueDSB):
  dsb sy
  ret

#ifndef TARGET_EMULATION
ASM_FUNCTION_REMOVE_IF_UNREFERENCED
#endif // TARGET_EMULATION
