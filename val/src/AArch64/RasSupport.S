#/** @file
# Copyright (c) 2023 Arm Limited or its affiliates. All rights reserved.
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
.align 2

GCC_ASM_EXPORT(AA64ReadErrIdr1)
GCC_ASM_EXPORT(AA64WriteErrIdr1)
GCC_ASM_EXPORT(AA64ReadErrAddr1)
GCC_ASM_EXPORT(AA64WriteErrAddr1)
GCC_ASM_EXPORT(AA64ReadErrCtlr1)
GCC_ASM_EXPORT(AA64WriteErrCtlr1)
GCC_ASM_EXPORT(AA64ReadErrFr1)
GCC_ASM_EXPORT(AA64ReadErrStatus1)
GCC_ASM_EXPORT(AA64WriteErrStatus1)
GCC_ASM_EXPORT(AA64ReadErrSelr1)
GCC_ASM_EXPORT(AA64WriteErrSelr1)
GCC_ASM_EXPORT(AA64ReadErrPfgf1)
GCC_ASM_EXPORT(AA64ReadErrPfgctl1)
GCC_ASM_EXPORT(AA64ReadErrPfgcdn1)
GCC_ASM_EXPORT(AA64WriteErrPfgf1)
GCC_ASM_EXPORT(AA64WriteErrPfgctl1)
GCC_ASM_EXPORT(AA64WriteErrPfgcdn1)


ASM_PFX(AA64ReadErrIdr1):
  mrs  x0, erridr_el1
  ret

ASM_PFX(AA64WriteErrIdr1):
  msr   erridr_el1, x0
  ret

ASM_PFX(AA64ReadErrAddr1):
  mrs  x0, erxaddr_el1
  ret

ASM_PFX(AA64WriteErrAddr1):
  msr   erxaddr_el1, x0
  ret

ASM_PFX(AA64ReadErrCtlr1):
  mrs  x0, erxctlr_el1
  ret

ASM_PFX(AA64WriteErrCtlr1):
  msr   erxctlr_el1, x0
  ret

ASM_PFX(AA64ReadErrFr1):
  mrs  x0, erxfr_el1
  ret

ASM_PFX(AA64ReadErrStatus1):
  mrs  x0, erxstatus_el1
  ret

ASM_PFX(AA64WriteErrStatus1):
  msr   erxstatus_el1, x0
  ret

ASM_PFX(AA64ReadErrSelr1):
  mrs  x0, errselr_el1
  ret

ASM_PFX(AA64WriteErrSelr1):
  msr   errselr_el1, x0
  ret

ASM_PFX(AA64ReadErrPfgf1):
  mrs  x0, erxpfgf_el1
  ret

ASM_PFX(AA64WriteErrPfgf1):
  msr   erxpfgf_el1, x0
  ret

ASM_PFX(AA64ReadErrPfgctl1):
  mrs  x0, erxpfgctl_el1
  ret

ASM_PFX(AA64WriteErrPfgctl1):
  msr   erxpfgctl_el1, x0
  ret

ASM_PFX(AA64ReadErrPfgcdn1):
  mrs  x0, erxpfgcdn_el1
  ret

ASM_PFX(AA64WriteErrPfgcdn1):
  msr   erxpfgcdn_el1, x0
  ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
