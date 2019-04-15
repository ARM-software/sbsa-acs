/** @file
 * Copyright (c) 2019, Arm Limited or its affiliates. All rights reserved.
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

#include "include/platform_override.h"

PLATFORM_OVERRIDE_PE_INFO_TABLE platform_pe_cfg = {

    .num_of_pe = PLATFORM_OVERRIDE_PE_CNT,

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
    .pe_info[7].pmu_gsiv = PLATFORM_OVERRIDE_PE7_PMU_GSIV,

};

PLATFORM_OVERRIDE_GIC_INFO_TABLE platform_gic_cfg = {

    .gic_version = PLATFORM_OVERRIDE_GIC_VERSION,
    .num_gicc    = PLATFORM_OVERRIDE_GICC_COUNT,
    .num_gicd    = PLATFORM_OVERRIDE_GICD_COUNT,
    .num_gicrd   = PLATFORM_OVERRIDE_GICRD_COUNT,
    .num_gicits  = PLATFORM_OVERRIDE_GICITS_COUNT,

    .gicc_base[0]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[1]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[2]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[3]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[4]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[5]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[6]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[7]   = PLATFORM_OVERRIDE_GICC_BASE,

    .gicd_base[0]   = PLATFORM_OVERRIDE_GICD_BASE,

    .gicrd_base[0]  = PLATFORM_OVERRIDE_GICRD_BASE,
    .gicrd_base[1]  = PLATFORM_OVERRIDE_GICRD_BASE,
    .gicrd_base[2]  = PLATFORM_OVERRIDE_GICRD_BASE,
    .gicrd_base[3]  = PLATFORM_OVERRIDE_GICRD_BASE,
    .gicrd_base[4]  = PLATFORM_OVERRIDE_GICRD_BASE,
    .gicrd_base[5]  = PLATFORM_OVERRIDE_GICRD_BASE,
    .gicrd_base[6]  = PLATFORM_OVERRIDE_GICRD_BASE,
    .gicrd_base[7]  = PLATFORM_OVERRIDE_GICRD_BASE,

    .gicits_base[0] = PLATFORM_OVERRIDE_GICITS_BASE

};


PLATFORM_OVERRIDE_TIMER_INFO_TABLE platform_timer_cfg = {

    .header.s_el1_timer_flags   = PLATFORM_OVERRIDE_S_EL1_TIMER_FLAGS,
    .header.ns_el1_timer_flags  = PLATFORM_OVERRIDE_NS_EL1_TIMER_FLAGS,
    .header.el2_timer_flags     = PLATFORM_OVERRIDE_NS_EL2_TIMER_FLAGS,
    .header.s_el1_timer_gsiv    = PLATFORM_OVERRIDE_S_EL1_TIMER_GSIV,
    .header.ns_el1_timer_gsiv   = PLATFORM_OVERRIDE_NS_EL1_TIMER_GSIV,
    .header.el2_timer_gsiv      = PLATFORM_OVERRIDE_NS_EL2_TIMER_GSIV,
    .header.virtual_timer_flags = PLATFORM_OVERRIDE_VIRTUAL_TIMER_FLAGS,
    .header.virtual_timer_gsiv  = PLATFORM_OVERRIDE_VIRTUAL_TIMER_GSIV,
    .header.el2_virt_timer_gsiv = PLATFORM_OVERRIDE_EL2_VIR_TIMER_GSIV,
    .header.num_platform_timer  = PLATFORM_OVERRIDE_PLATFORM_TIMER_COUNT,

    .gt_info.type               = PLATFORM_OVERRIDE_TIMER_TYPE,
    .gt_info.timer_count        = PLATFORM_OVERRIDE_TIMER_COUNT,
    .gt_info.block_cntl_base    = PLATFORM_OVERRIDE_TIMER_CNTCTL_BASE,
    .gt_info.GtCntBase[0]       = PLATFORM_OVERRIDE_TIMER_CNTBASE_0,
    .gt_info.GtCntBase[1]       = PLATFORM_OVERRIDE_TIMER_CNTBASE_1,
    .gt_info.GtCntEl0Base[0]    = PLATFORM_OVERRIDE_TIMER_CNTEL0BASE_0,
    .gt_info.GtCntEl0Base[1]    = PLATFORM_OVERRIDE_TIMER_CNTEL0BASE_1,
    .gt_info.gsiv[0]            = PLATFORM_OVERRIDE_TIMER_GSIV_0,
    .gt_info.gsiv[1]            = PLATFORM_OVERRIDE_TIMER_GSIV_1,
    .gt_info.virt_gsiv[0]       = PLATFORM_OVERRIDE_TIMER_VIRT_GSIV_0,
    .gt_info.virt_gsiv[1]       = PLATFORM_OVERRIDE_TIMER_VIRT_GSIV_1,
    .gt_info.flags[0]           = PLATFORM_OVERRIDE_TIMER_FLAGS_0,
    .gt_info.flags[1]           = PLATFORM_OVERRIDE_TIMER_FLAGS_1

};

PLATFORM_OVERRIDE_WD_INFO_TABLE platform_wd_cfg = {
    .num_wd                     = PLATFORM_OVERRIDE_WD_TIMER_COUNT,
    .wd_info[0].wd_ctrl_base    = PLATFORM_OVERRIDE_WD_CTRL_BASE,
    .wd_info[0].wd_refresh_base = PLATFORM_OVERRIDE_WD_REFRESH_BASE,
    .wd_info[0].wd_gsiv         = PLATFORM_OVERRIDE_WD_GSIV,
    .wd_info[0].wd_flags        = PLATFORM_OVERRIDE_WD_FLAGS
};
