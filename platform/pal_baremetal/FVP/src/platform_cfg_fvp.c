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

#include "include/pal_common_support.h"
#include "include/platform_override_fvp.h"

PE_INFO_TABLE platform_pe_cfg = {

    .header.num_of_pe = PLATFORM_OVERRIDE_PE_CNT,

    .pe_info[0].pe_num   = PLATFORM_OVERRIDE_PE0_INDEX,
    .pe_info[0].mpidr    = PLATFORM_OVERRIDE_PE0_MPIDR,
    .pe_info[0].pmu_gsiv = PLATFORM_OVERRIDE_PE0_PMU_GSIV,

    .pe_info[1].pe_num   = PLATFORM_OVERRIDE_PE1_INDEX,
    .pe_info[1].mpidr    = PLATFORM_OVERRIDE_PE1_MPIDR,
    .pe_info[1].pmu_gsiv = PLATFORM_OVERRIDE_PE1_PMU_GSIV,

    .pe_info[2].pe_num   = PLATFORM_OVERRIDE_PE2_INDEX,
    .pe_info[2].mpidr    = PLATFORM_OVERRIDE_PE2_MPIDR,
    .pe_info[2].pmu_gsiv = PLATFORM_OVERRIDE_PE2_PMU_GSIV,

    .pe_info[3].pe_num   = PLATFORM_OVERRIDE_PE3_INDEX,
    .pe_info[3].mpidr    = PLATFORM_OVERRIDE_PE3_MPIDR,
    .pe_info[3].pmu_gsiv = PLATFORM_OVERRIDE_PE3_PMU_GSIV,

    .pe_info[4].pe_num   = PLATFORM_OVERRIDE_PE4_INDEX,
    .pe_info[4].mpidr    = PLATFORM_OVERRIDE_PE4_MPIDR,
    .pe_info[4].pmu_gsiv = PLATFORM_OVERRIDE_PE4_PMU_GSIV,

    .pe_info[5].pe_num   = PLATFORM_OVERRIDE_PE5_INDEX,
    .pe_info[5].mpidr    = PLATFORM_OVERRIDE_PE5_MPIDR,
    .pe_info[5].pmu_gsiv = PLATFORM_OVERRIDE_PE5_PMU_GSIV,

    .pe_info[6].pe_num   = PLATFORM_OVERRIDE_PE6_INDEX,
    .pe_info[6].mpidr    = PLATFORM_OVERRIDE_PE6_MPIDR,
    .pe_info[6].pmu_gsiv = PLATFORM_OVERRIDE_PE6_PMU_GSIV,

    .pe_info[7].pe_num   = PLATFORM_OVERRIDE_PE7_INDEX,
    .pe_info[7].mpidr    = PLATFORM_OVERRIDE_PE7_MPIDR,
    .pe_info[7].pmu_gsiv = PLATFORM_OVERRIDE_PE7_PMU_GSIV

};

PCIE_INFO_TABLE platform_pcie_cfg = {
    .num_entries             = PLATFORM_OVERRIDE_NUM_ECAM,
    .block[0].ecam_base      = PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_0,
    .block[0].bar_start_addr = PLATFORM_OVERRIDE_PCIE_BAR_START_ADDR_0,
    .block[0].segment_num    = PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_0,
    .block[0].start_bus_num  = PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_0,
    .block[0].end_bus_num    = PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_0

/** Configure more PCIe info details as per specification for more than 1 ECAM
    Refer to platform_override_fvp.h file for an example
**/
};
