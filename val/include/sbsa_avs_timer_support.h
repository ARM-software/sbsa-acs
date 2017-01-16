/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#ifndef __ARM_ARCH_TIMER_H__
#define __ARM_ARCH_TIMER_H__

#define ARM_ARCH_TIMER_ENABLE           (1 << 0)
#define ARM_ARCH_TIMER_IMASK            (1 << 1)
#define ARM_ARCH_TIMER_ISTATUS          (1 << 2)

typedef enum {
  CntFrq = 0,
  CntPct,
  CntkCtl,
  CntpTval,
  CntpCtl,
  CntvTval,
  CntvCtl,
  CntvCt,
  CntpCval,
  CntvCval,
  CntvOff,
  CnthCtl,
  CnthpTval,
  CnthpCtl,
  CnthpCval,
  CnthvTval,
  CnthvCtl,
  CnthvCval,
  RegMaximum
} ARM_ARCH_TIMER_REGS;


uint64_t
ArmArchTimerReadReg (
  ARM_ARCH_TIMER_REGS   Reg
  );


void
ArmArchTimerWriteReg (
    ARM_ARCH_TIMER_REGS   Reg,
    uint64_t              *data_buf
  );

uint64_t
ArmReadCntFrq (
  void
  );

void
ArmWriteCntFrq (
  uint64_t   FreqInHz
  );

uint64_t
ArmReadCntPct (
  void
  );

uint64_t
ArmReadCntkCtl (
  void
  );

void
ArmWriteCntkCtl (
  uint64_t   Val
  );

uint64_t
ArmReadCntpTval (
  void
  );

void
ArmWriteCntpTval (
  uint64_t   Val
  );

UINTN
ArmReadCntpCtl (
  void
  );

void
ArmWriteCntpCtl (
  uint64_t   Val
  );

UINTN
ArmReadCntvTval (
  void
  );

void
ArmWriteCntvTval (
  uint64_t   Val
  );

UINTN
ArmReadCntvCtl (
  void
  );

void ArmWriteCntvCtl (uint64_t   Val);
uint64_t ArmReadCntvCt (void);
uint64_t ArmReadCntpCval (void);
void ArmWriteCntpCval (uint64_t   Val);

uint64_t ArmReadCntvCval (void);
void ArmWriteCntvCval (uint64_t   Val);
uint64_t ArmReadCntvOff (void);
void ArmWriteCntvOff (uint64_t   Val);

uint64_t ArmReadCnthpCtl (void);
void ArmWriteCnthpCtl (uint64_t Val);
uint64_t ArmReadCnthpTval (void);
void ArmWriteCnthpTval (uint64_t Val);

uint64_t ArmReadCnthvCtl (void);
void ArmWriteCnthvCtl (uint64_t Val);
uint64_t ArmReadCnthvTval (void);
void ArmWriteCnthvTval (uint64_t Val);

#endif // __ARM_ARCH_TIMER_H__
