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


/* THESE values are provided as a HOOK to
     1. Override a value which is read from ACPI tables
     2. Fill-in a value which is not provided by any ACPI Table
 */

//Vexpress 0x1c090000
//Juno 0x7FF80000
#define PLATFORM_GENERIC_UART_BASE       0

//Juno - 115
#define PLATFORM_GENERIC_UART_INTID      0

/* Change OVERRIDE to 1 and define the Timer values to be used */
#define PLATFORM_OVERRIDE_PLATFORM_TIMER 0
#define PLATFORM_OVERRIDE_CNTCTL_BASE    0x2a810000
#define PLATFORM_OVERRIDE_CNTREAD_BASE   0x2a800000
#define PLATFORM_OVERRIDE_CNTBASE_N      0x2a830000
#define PLATFORM_OVERRIDE_PLATFORM_TIMER_GSIV 58


#define PLATFORM_OVERRIDE_EL2_VIR_TIMER_GSIV  28


/* Change OVERRIDE_WD to 1 and define the WD values to be used */
#define PLATFORM_OVERRIDE_WD               0
#define PLATFORM_OVERRIDE_WD_REFRESH_BASE  0x2A450000
#define PLATFORM_OVERRIDE_WD_CTRL_BASE     0x2A440000
#define PLATFORM_OVERRIDE_WD_GSIV          93


/* To use a different value from the MCFG Table, change this to Non-Zero */
#define PLATFORM_OVERRIDE_PCIE_ECAM_BASE       0x0 //0x40000000
#define PLATFORM_OVERRIDE_PCIE_START_BUS_NUM   0x0

/* Change OVERRIDE_SMMU_BASE to non-zero value for this to take effect */
#define PLATFORM_OVERRIDE_SMMU_BASE        0x0 //0x2B400000
#define PLATFORM_OVERRIDE_SMMU_ARCH_MAJOR  3

