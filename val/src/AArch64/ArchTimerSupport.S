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
.align 2

GCC_ASM_EXPORT(ArmReadCntFrq)
GCC_ASM_EXPORT(ArmReadCntPct)
GCC_ASM_EXPORT(ArmReadCntkCtl)
GCC_ASM_EXPORT(ArmWriteCntkCtl)
GCC_ASM_EXPORT(ArmReadCntpTval)
GCC_ASM_EXPORT(ArmWriteCntpTval)
GCC_ASM_EXPORT(ArmReadCntpCtl)
GCC_ASM_EXPORT(ArmWriteCntpCtl)
GCC_ASM_EXPORT(ArmReadCntvTval)
GCC_ASM_EXPORT(ArmWriteCntvTval)
GCC_ASM_EXPORT(ArmReadCntvCtl)
GCC_ASM_EXPORT(ArmWriteCntvCtl)
GCC_ASM_EXPORT(ArmReadCntvCt)
GCC_ASM_EXPORT(ArmReadCntpCval)
GCC_ASM_EXPORT(ArmWriteCntpCval)
GCC_ASM_EXPORT(ArmReadCntvCval)
GCC_ASM_EXPORT(ArmWriteCntvCval)
GCC_ASM_EXPORT(ArmReadCntvOff)
GCC_ASM_EXPORT(ArmWriteCntvOff)
GCC_ASM_EXPORT(ArmReadCnthpCtl)
GCC_ASM_EXPORT(ArmWriteCnthpCtl)
GCC_ASM_EXPORT(ArmReadCnthpTval)
GCC_ASM_EXPORT(ArmWriteCnthpTval)
GCC_ASM_EXPORT(ArmReadCnthvCtl)
GCC_ASM_EXPORT(ArmWriteCnthvCtl)
GCC_ASM_EXPORT(ArmReadCnthvTval)
GCC_ASM_EXPORT(ArmWriteCnthvTval)

ASM_PFX(ArmReadCntFrq):
  mrs   x0, cntfrq_el0           // Read CNTFRQ
  ret


ASM_PFX(ArmReadCntPct):
  mrs   x0, cntpct_el0           // Read CNTPCT (Physical counter register)
  ret


ASM_PFX(ArmReadCntkCtl):
  mrs   x0, cntkctl_el1          // Read CNTK_CTL (Timer PL1 Control Register)
  ret


ASM_PFX(ArmWriteCntkCtl):
  msr   cntkctl_el1, x0          // Write to CNTK_CTL (Timer PL1 Control Register)
  isb
  ret


ASM_PFX(ArmReadCntpTval):
  mrs   x0, cntp_tval_el0        // Read CNTP_TVAL (PL1 physical timer value register)
  ret


ASM_PFX(ArmWriteCntpTval):
  msr   cntp_tval_el0, x0        // Write to CNTP_TVAL (PL1 physical timer value register)
  isb
  ret


ASM_PFX(ArmReadCntpCtl):
  mrs   x0, cntp_ctl_el0         // Read CNTP_CTL (PL1 Physical Timer Control Register)
  ret


ASM_PFX(ArmWriteCntpCtl):
  msr   cntp_ctl_el0, x0         // Write to  CNTP_CTL (PL1 Physical Timer Control Register)
  isb
  ret


ASM_PFX(ArmReadCntvTval):
  mrs   x0, cntv_tval_el0        // Read CNTV_TVAL (Virtual Timer Value register)
  ret


ASM_PFX(ArmWriteCntvTval):
  msr   cntv_tval_el0, x0        // Write to CNTV_TVAL (Virtual Timer Value register)
  isb
  ret


ASM_PFX(ArmReadCntvCtl):
  mrs   x0, cntv_ctl_el0         // Read CNTV_CTL (Virtual Timer Control Register)
  ret


ASM_PFX(ArmWriteCntvCtl):
  msr   cntv_ctl_el0, x0         // Write to CNTV_CTL (Virtual Timer Control Register)
  isb
  ret


ASM_PFX(ArmReadCntvCt):
  mrs  x0, cntvct_el0            // Read CNTVCT  (Virtual Count Register)
  ret


ASM_PFX(ArmReadCntpCval):
  mrs   x0, cntp_cval_el0        // Read CNTP_CTVAL (Physical Timer Compare Value Register)
  ret


ASM_PFX(ArmWriteCntpCval):
  msr   cntp_cval_el0, x0        // Write to CNTP_CTVAL (Physical Timer Compare Value Register)
  isb
  ret


ASM_PFX(ArmReadCntvCval):
  mrs   x0, cntv_cval_el0        // Read CNTV_CTVAL (Virtual Timer Compare Value Register)
  ret


ASM_PFX(ArmWriteCntvCval):
  msr   cntv_cval_el0, x0        // write to  CNTV_CTVAL (Virtual Timer Compare Value Register)
  isb
  ret


ASM_PFX(ArmReadCntvOff):
  mrs   x0, cntvoff_el2          // Read CNTVOFF (virtual Offset register)
  ret


ASM_PFX(ArmWriteCntvOff):
  msr   cntvoff_el2, x0          // Write to CNTVOFF (Virtual Offset register)
  isb
  ret

ASM_PFX(ArmReadCnthpCtl):
  mrs   x0, cnthp_ctl_el2
  ret


ASM_PFX(ArmWriteCnthpCtl):
  msr   cnthp_ctl_el2, x0
  isb
  ret

ASM_PFX(ArmReadCnthpTval):
  mrs   x0, cnthp_tval_el2
  ret


ASM_PFX(ArmWriteCnthpTval):
  msr   cnthp_tval_el2, x0
  isb
  ret

ASM_PFX(ArmReadCnthvCtl):
  mrs   x0, cnthv_ctl_el2
  ret


ASM_PFX(ArmWriteCnthvCtl):
  msr   cnthv_ctl_el2, x0
  isb
  ret

ASM_PFX(ArmReadCnthvTval):
  mrs   x0, cnthv_tval_el2
  ret


ASM_PFX(ArmWriteCnthvTval):
  msr   cnthv_tval_el2, x0
  isb
  ret


ASM_FUNCTION_REMOVE_IF_UNREFERENCED
