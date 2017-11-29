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


.text
.align 3


GCC_ASM_EXPORT (ArmReadMpidr)
GCC_ASM_EXPORT (ArmReadIdPfr0)
GCC_ASM_EXPORT (ArmReadIdPfr1)
GCC_ASM_EXPORT (AA64ReadMmfr0)
GCC_ASM_EXPORT (AA64ReadMmfr1)
GCC_ASM_EXPORT (AA64ReadMmfr2)
GCC_ASM_EXPORT (AA64ReadCtr)
GCC_ASM_EXPORT (ArmReadMmfr0)
GCC_ASM_EXPORT (AA64ReadIsar0)
GCC_ASM_EXPORT (AA64ReadIsar1)
GCC_ASM_EXPORT (AA64ReadSctlr3)
GCC_ASM_EXPORT (AA64ReadSctlr2)
GCC_ASM_EXPORT (AA64ReadPmcr)
GCC_ASM_EXPORT (AA64ReadIdDfr0)
GCC_ASM_EXPORT (AA64ReadIdDfr1)
GCC_ASM_EXPORT (ArmReadHcr)
GCC_ASM_EXPORT (AA64ReadCurrentEL)
GCC_ASM_EXPORT (AA64ReadIdMdrar)
GCC_ASM_EXPORT (AA64ReadMdcr2)
GCC_ASM_EXPORT (AA64WriteMdcr2)
GCC_ASM_EXPORT (AA64ReadVbar2)
GCC_ASM_EXPORT (AA64WriteVbar2)
GCC_ASM_EXPORT (AA64WritePmcr)
GCC_ASM_EXPORT (AA64WritePmovsset)
GCC_ASM_EXPORT (AA64WritePmintenset)
GCC_ASM_EXPORT (AA64WritePmovsclr)
GCC_ASM_EXPORT (AA64WritePmintenclr)
GCC_ASM_EXPORT (AA64ReadCcsidr)
GCC_ASM_EXPORT (AA64ReadCsselr)
GCC_ASM_EXPORT (AA64WriteCsselr)
GCC_ASM_EXPORT (AA64ReadClidr)
GCC_ASM_EXPORT (ArmReadDfr0)
GCC_ASM_EXPORT (ArmReadIsar0)
GCC_ASM_EXPORT (ArmReadIsar1)
GCC_ASM_EXPORT (ArmReadIsar2)
GCC_ASM_EXPORT (ArmReadIsar3)
GCC_ASM_EXPORT (ArmReadIsar4)
GCC_ASM_EXPORT (ArmReadIsar5)
GCC_ASM_EXPORT (ArmReadMmfr0)
GCC_ASM_EXPORT (ArmReadMmfr1)
GCC_ASM_EXPORT (ArmReadMmfr2)
GCC_ASM_EXPORT (ArmReadMmfr3)
GCC_ASM_EXPORT (ArmReadMmfr4)
GCC_ASM_EXPORT (ArmReadPfr0)
GCC_ASM_EXPORT (ArmReadPfr1)
GCC_ASM_EXPORT (ArmReadMidr)
GCC_ASM_EXPORT (ArmReadMvfr0)
GCC_ASM_EXPORT (ArmReadMvfr1)
GCC_ASM_EXPORT (ArmReadMvfr2)
GCC_ASM_EXPORT (AA64ReadPmceid0)
GCC_ASM_EXPORT (AA64ReadPmceid1)
GCC_ASM_EXPORT (AA64ReadVmpidr)
GCC_ASM_EXPORT (AA64ReadVpidr)
GCC_ASM_EXPORT (AA64ReadPmbidr)
GCC_ASM_EXPORT (AA64ReadPmsidr)
GCC_ASM_EXPORT (AA64ReadLorid)
GCC_ASM_EXPORT (AA64ReadErridr)
GCC_ASM_EXPORT (AA64ReadErr0fr)
GCC_ASM_EXPORT (AA64ReadErr1fr)
GCC_ASM_EXPORT (AA64ReadErr2fr)
GCC_ASM_EXPORT (AA64ReadErr3fr)
GCC_ASM_EXPORT (AA64WritePmsirr)
GCC_ASM_EXPORT (AA64WritePmscr2)
GCC_ASM_EXPORT (AA64WritePmsfcr)
GCC_ASM_EXPORT (AA64WritePmbptr)
GCC_ASM_EXPORT (AA64WritePmblimitr)
GCC_ASM_EXPORT (AA64ReadEsr2)
GCC_ASM_EXPORT (AA64ReadSp)
GCC_ASM_EXPORT (AA64WriteSp)
GCC_ASM_EXPORT (AA64ReadFar2)

ASM_PFX(ArmReadMpidr):
  mrs   x0, mpidr_el1           // read EL1 MPIDR
  ret

ASM_PFX(ArmReadIdPfr0):
  mrs   x0, id_aa64pfr0_el1   // Read ID_AA64PFR0 Register
  ret

ASM_PFX(ArmReadIdPfr1):
  mrs   x0, id_aa64pfr1_el1   // Read ID_AA64PFR0 Register
  ret

ASM_PFX(AA64ReadMmfr0):
  mrs   x0, id_aa64mmfr0_el1
  ret

ASM_PFX(AA64ReadMmfr1):
  mrs   x0, id_aa64mmfr1_el1
  ret

ASM_PFX(AA64ReadMmfr2):
  mrs   x0, id_aa64mmfr2_el1
  ret

ASM_PFX(AA64ReadCtr):
  mrs   x0, ctr_el0
  ret

ASM_PFX(AA64ReadIsar0):
  mrs   x0, id_aa64isar0_el1
  ret

ASM_PFX(AA64ReadIsar1):
  mrs   x0, id_aa64isar1_el1
  ret

ASM_PFX(AA64ReadSctlr3):
  mrs   x0, sctlr_el3
  ret

ASM_PFX(AA64ReadSctlr2):
  mrs   x0, sctlr_el2
  ret

ASM_PFX(AA64ReadPmcr):
  mrs   x0, pmcr_el0
  ret

ASM_PFX(AA64ReadIdDfr0):
  mrs   x0, id_aa64dfr0_el1
  ret

ASM_PFX(AA64ReadIdDfr1):
  mrs   x0, id_aa64dfr1_el1
  ret

// UINTN ArmReadHcr(VOID)
ASM_PFX(ArmReadHcr):
  mrs   x0, hcr_el2
  ret

ASM_PFX(AA64ReadCurrentEL):
  mrs   x0, CurrentEL
  ret

ASM_PFX(AA64ReadMdcr2):
  mrs   x0, mdcr_el2
  ret

ASM_PFX(AA64WriteMdcr2):
  msr   mdcr_el2, x0
  isb
  ret

ASM_PFX(AA64ReadVbar2):
  mrs   x0, vbar_el2
  ret

ASM_PFX(AA64WriteVbar2):
  msr   vbar_el2, x0
  isb
  ret

ASM_PFX(AA64WritePmcr):
  msr   pmcr_el0, x0
  isb
  ret

ASM_PFX(AA64WritePmovsset):
  msr   pmovsset_el0, x0
  isb
  ret

ASM_PFX(AA64WritePmovsclr):
  msr   pmovsclr_el0, x0
  isb
  ret

ASM_PFX(AA64WritePmintenset):
  msr   pmintenset_el1, x0
  isb
  ret

ASM_PFX(AA64WritePmintenclr):
  msr   pmintenclr_el1, x0
  isb
  ret

ASM_PFX(AA64ReadCcsidr):
  mrs   x0, ccsidr_el1
  ret

ASM_PFX(AA64ReadCsselr):
  mrs   x0, csselr_el1
  ret

ASM_PFX(AA64WriteCsselr):
  msr   csselr_el1, x0
  isb
  ret

ASM_PFX(AA64ReadClidr):
  mrs   x0, clidr_el1
  ret

ASM_PFX(ArmReadDfr0):
  mrs   x0, id_dfr0_el1
  ret

ASM_PFX(ArmReadIsar0):
  mrs   x0, id_isar0_el1
  ret

ASM_PFX(ArmReadIsar1):
  mrs   x0, id_isar1_el1
  ret

ASM_PFX(ArmReadIsar2):
  mrs   x0, id_isar2_el1
  ret

ASM_PFX(ArmReadIsar3):
  mrs   x0, id_isar3_el1
  ret

ASM_PFX(ArmReadIsar4):
  mrs   x0, id_isar4_el1
  ret

ASM_PFX(ArmReadIsar5):
  mrs   x0, id_isar5_el1
  ret

ASM_PFX(ArmReadMmfr0):
  mrs   x0, id_mmfr0_el1
  ret

ASM_PFX(ArmReadMmfr1):
  mrs   x0, id_mmfr1_el1
  ret

ASM_PFX(ArmReadMmfr2):
  mrs   x0, id_mmfr2_el1
  ret

ASM_PFX(ArmReadMmfr3):
  mrs   x0, id_mmfr3_el1
  ret

ASM_PFX(ArmReadMmfr4):
  //mrs   x0, id_mmfr4_el1
  ret

ASM_PFX(ArmReadPfr0):
  mrs   x0, id_pfr0_el1
  ret

ASM_PFX(ArmReadPfr1):
  mrs   x0, id_pfr1_el1
  ret

ASM_PFX(ArmReadMidr):
  mrs   x0, midr_el1
  ret

ASM_PFX(ArmReadMvfr0):
  mrs   x0, mvfr0_el1
  ret

ASM_PFX(ArmReadMvfr1):
  mrs   x0, mvfr1_el1
  ret

ASM_PFX(ArmReadMvfr2):
  mrs   x0, mvfr2_el1
  ret

ASM_PFX(AA64ReadPmceid0):
  mrs   x0, pmceid0_el0
  ret

ASM_PFX(AA64ReadPmceid1):
  mrs   x0, pmceid1_el0
  ret

ASM_PFX(AA64ReadVmpidr):
  mrs   x0, vmpidr_el2
  ret

ASM_PFX(AA64ReadVpidr):
  mrs   x0, vpidr_el2
  ret

ASM_PFX(AA64ReadPmbidr):
  //mrs   x0, pmbidr_el1
  ret

ASM_PFX(AA64ReadPmsidr):
  //mrs   x0, pmsidr_el1
  ret

ASM_PFX(AA64ReadLorid):
  //mrs   x0, lorid_el1
  ret

ASM_PFX(AA64ReadErridr):
  //mrs   x0, erridr_el1
  ret

ASM_PFX(AA64ReadErr0fr):
 // mrs   x0, err0fr_el1
  ret

ASM_PFX(AA64ReadErr1fr):
  //mrs   x0, err1fr_el1
  ret

ASM_PFX(AA64ReadErr2fr):
  //mrs   x0, err2fr_el1
  ret

ASM_PFX(AA64ReadErr3fr):
  //mrs   x0, err3fr_el1
  ret

ASM_PFX(AA64WritePmsirr):
  //mrs   pmsirr_el1,x0
  isb
  ret

ASM_PFX(AA64WritePmscr2):
  //mrs   pmscr_el2,x0
  isb
  ret

ASM_PFX(AA64WritePmsfcr):
  //mrs   pmsfcr_el1,x0
  isb
  ret

ASM_PFX(AA64WritePmbptr):
  //mrs   pmbptr_el1,x0
  isb
  ret

ASM_PFX(AA64WritePmblimitr):
  //mrs   pmblimitr_el1,x0
  isb
  ret

ASM_PFX(AA64ReadEsr2):
  mrs   x0, esr_el2
  ret

ASM_PFX(AA64ReadSp):
  mov   x0, sp
  ret

ASM_PFX(AA64WriteSp):
  mov   sp, x0
  ret

ASM_PFX(AA64ReadFar2):
  mrs   x0, far_el2
  ret

