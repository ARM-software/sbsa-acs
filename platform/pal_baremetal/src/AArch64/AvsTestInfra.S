#/** @file
# Copyright (c) 2020, 2022 Arm Limited or its affiliates. All rights reserved.
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

/* Private worker functions for ASM_PFX() */
#define _CONCATENATE(a, b)  __CONCATENATE(a, b)
#define __CONCATENATE(a, b) a ## b

/* The __USER_LABEL_PREFIX__ macro predefined by GNUC represents
   the prefix on symbols in assembly language.*/
#define __USER_LABEL_PREFIX__

#define ASM_PFX(name) _CONCATENATE (__USER_LABEL_PREFIX__, name)

#define GCC_ASM_EXPORT(func__)  \
       .global  _CONCATENATE (__USER_LABEL_PREFIX__, func__)    ;\
       .type ASM_PFX(func__), %function

#define GCC_ASM_IMPORT(func__)  \
       .extern  _CONCATENATE (__USER_LABEL_PREFIX__, func__)

.text
.align 3

GCC_ASM_EXPORT(DataCacheCleanInvalidateVA)
GCC_ASM_EXPORT(DataCacheInvalidateVA)
GCC_ASM_EXPORT(DataCacheCleanVA)

ASM_PFX(DataCacheCleanInvalidateVA):
  dc  civac, x0
  dsb sy
  isb
  ret

ASM_PFX(DataCacheCleanVA):
  dc  cvac, x0
  dsb ish
  isb
  ret

ASM_PFX(DataCacheInvalidateVA):
  dc  ivac, x0
  dsb ish
  isb
  ret
