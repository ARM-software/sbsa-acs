/** @file
 * Copyright (c) 2020, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

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

#include <stdio.h>
#include <stdint.h>

/** Begin config **/

/* PCIe BAR config parameters*/
#define PCIE_BAR64_VAL       0x500000000
#define PCIE_BAR32_VAL       0x70000000

/* PE platform config paramaters */
#define PLATFORM_OVERRIDE_PE_CNT        0x8
#define PLATFORM_OVERRIDE_PE0_INDEX     0x0
#define PLATFORM_OVERRIDE_PE0_MPIDR     0x0
#define PLATFORM_OVERRIDE_PE0_PMU_GSIV  0x17
#define PLATFORM_OVERRIDE_PE1_INDEX     0x1
#define PLATFORM_OVERRIDE_PE1_MPIDR     0x100
#define PLATFORM_OVERRIDE_PE1_PMU_GSIV  0x17
#define PLATFORM_OVERRIDE_PE2_INDEX     0x2
#define PLATFORM_OVERRIDE_PE2_MPIDR     0x200
#define PLATFORM_OVERRIDE_PE2_PMU_GSIV  0x17
#define PLATFORM_OVERRIDE_PE3_INDEX     0x3
#define PLATFORM_OVERRIDE_PE3_MPIDR     0x300
#define PLATFORM_OVERRIDE_PE3_PMU_GSIV  0x17
#define PLATFORM_OVERRIDE_PE4_INDEX     0x4
#define PLATFORM_OVERRIDE_PE4_MPIDR     0x10000
#define PLATFORM_OVERRIDE_PE4_PMU_GSIV  0x17
#define PLATFORM_OVERRIDE_PE5_INDEX     0x5
#define PLATFORM_OVERRIDE_PE5_MPIDR     0x10100
#define PLATFORM_OVERRIDE_PE5_PMU_GSIV  0x17
#define PLATFORM_OVERRIDE_PE6_INDEX     0x6
#define PLATFORM_OVERRIDE_PE6_MPIDR     0x10200
#define PLATFORM_OVERRIDE_PE6_PMU_GSIV  0x17
#define PLATFORM_OVERRIDE_PE7_INDEX     0x7
#define PLATFORM_OVERRIDE_PE7_MPIDR     0x10300
#define PLATFORM_OVERRIDE_PE7_PMU_GSIV  0x17

/* PCIE platform config parameters */
#define PLATFORM_OVERRIDE_NUM_ECAM                1

/* Platform config parameters for ECAM_0 */
#define PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_0   0x60000000
#define PLATFORM_OVERRIDE_PCIE_BAR_START_ADDR_0   0x60000010
#define PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_0  0x0
#define PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_0    0x0
#define PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_0      0xFF

/* Sample macros for ECAM_1
 * #define PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_1  0x00000000
 * #define PLATFORM_OVERRIDE_PCIE_BAR_START_ADDR_1  0x00000010
 * #define PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_1 0x0
 * #define PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_1   0x0
 * #define PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_1     0x0
 */

/** End config **/
