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

#ifndef __ARM_STD_SMC_H__
#define __ARM_STD_SMC_H__

/*
 * SMC function IDs for Standard Service queries
 */

#define ARM_SMC_ID_STD_CALL_COUNT     0x8400ff00
#define ARM_SMC_ID_STD_UID            0x8400ff01
/*                                    0x8400ff02 is reserved */
#define ARM_SMC_ID_STD_REVISION       0x8400ff03

/*
 * The 'Standard Service Call UID' is supposed to return the Standard
 * Service UUID. This is a 128-bit value.
 */
#define ARM_SMC_STD_UUID0       0x108d905b
#define ARM_SMC_STD_UUID1       0x47e8f863
#define ARM_SMC_STD_UUID2       0xfbc02dae
#define ARM_SMC_STD_UUID3       0xe2f64156

/*
 * ARM Standard Service Calls revision numbers
 * The current revision is:  0.1
 */
#define ARM_SMC_STD_REVISION_MAJOR    0x0
#define ARM_SMC_STD_REVISION_MINOR    0x1

/*
 * Power State Coordination Interface (PSCI) calls cover a subset of the
 * Standard Service Call range.
 * The list below is not exhaustive.
 */
#define ARM_SMC_ID_PSCI_VERSION                0x84000000
#define ARM_SMC_ID_PSCI_CPU_SUSPEND_AARCH64    0xc4000001
#define ARM_SMC_ID_PSCI_CPU_SUSPEND_AARCH32    0x84000001
#define ARM_SMC_ID_PSCI_CPU_OFF                0x84000002
#define ARM_SMC_ID_PSCI_CPU_ON_AARCH64         0xc4000003
#define ARM_SMC_ID_PSCI_CPU_ON_AARCH32         0x84000003
#define ARM_SMC_ID_PSCI_AFFINITY_INFO_AARCH64  0xc4000004
#define ARM_SMC_ID_PSCI_AFFINITY_INFO_AARCH32  0x84000004
#define ARM_SMC_ID_PSCI_MIGRATE_AARCH64        0xc4000005
#define ARM_SMC_ID_PSCI_MIGRATE_AARCH32        0x84000005
#define ARM_SMC_ID_PSCI_SYSTEM_OFF             0x84000008
#define ARM_SMC_ID_PSCI_SYSTEM_RESET           0x84000009

/* The current PSCI version is:  0.2 */
#define ARM_SMC_PSCI_VERSION_MAJOR  0
#define ARM_SMC_PSCI_VERSION_MINOR  2
#define ARM_SMC_PSCI_VERSION  \
  ((ARM_SMC_PSCI_VERSION_MAJOR << 16) | ARM_SMC_PSCI_VERSION_MINOR)

/* PSCI return error codes */
#define ARM_SMC_PSCI_RET_SUCCESS            0
#define ARM_SMC_PSCI_RET_NOT_SUPPORTED      -1
#define ARM_SMC_PSCI_RET_INVALID_PARAMS     -2
#define ARM_SMC_PSCI_RET_DENIED             -3
#define ARM_SMC_PSCI_RET_ALREADY_ON         -4
#define ARM_SMC_PSCI_RET_ON_PENDING         -5
#define ARM_SMC_PSCI_RET_INTERN_FAIL        -6
#define ARM_SMC_PSCI_RET_NOT_PRESENT        -7
#define ARM_SMC_PSCI_RET_DISABLED           -8

#define ARM_SMC_PSCI_TARGET_CPU32(Aff2, Aff1, Aff0) \
  ((((Aff2) & 0xFF) << 16) | (((Aff1) & 0xFF) << 8) | ((Aff0) & 0xFF))

#define ARM_SMC_PSCI_TARGET_CPU64(Aff3, Aff2, Aff1, Aff0) \
  ((((Aff3) & 0xFFULL) << 32) | (((Aff2) & 0xFF) << 16) | (((Aff1) & 0xFF) << 8) | ((Aff0) & 0xFF))

#define ARM_SMC_PSCI_TARGET_GET_AFF0(TargetId)  ((TargetId) & 0xFF)
#define ARM_SMC_PSCI_TARGET_GET_AFF1(TargetId)  (((TargetId) >> 8) & 0xFF)

#define ARM_SMC_ID_PSCI_AFFINITY_LEVEL_0    0
#define ARM_SMC_ID_PSCI_AFFINITY_LEVEL_1    1
#define ARM_SMC_ID_PSCI_AFFINITY_LEVEL_2    2
#define ARM_SMC_ID_PSCI_AFFINITY_LEVEL_3    3

#define ARM_SMC_ID_PSCI_AFFINITY_INFO_ON          0
#define ARM_SMC_ID_PSCI_AFFINITY_INFO_OFF         1
#define ARM_SMC_ID_PSCI_AFFINITY_INFO_ON_PENDING  2

/**
  Trigger an SMC call

  SMC calls can take up to 7 arguments and return up to 4 return values.
  Therefore, the 4 first fields in the ARM_SMC_ARGS structure are used
  for both input and output values.

**/
void
ArmCallSmc (
  ARM_SMC_ARGS *Args
  );


#endif
