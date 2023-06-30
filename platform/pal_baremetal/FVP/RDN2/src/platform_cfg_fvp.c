/** @file
 * Copyright (c) 2020-2023, Arm Limited or its affiliates. All rights reserved.
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
#include "include/platform_override_struct.h"

/*
    To run a specific modules:
      - Give the module base numbers in the g_module_array.
      - All module base numbers can be found in val/include/bsa_acs_common.h
      - Example - if g_module_array = {0}, only PE tests will be run while skipping other modules

    To run a specific tests:
      - Give the test numbers in the g_test_array.
      - Test numbers can be found in test_pool/<module>/test.c.
      - For example, if g_test_array = {801}, only first test in PCIe will be run.
        All other tests in all other modules will be skipped.
      - If g_single_module is also given, then single test + tests under single module will be run.

    To skip tests/modules:
      - Give test numbers or module test bases as the entries of g_skip_array.
      - This will only skip the tests or modules given in the array and runs all other tests.

    Tests run = g_test_array + g_module_array - Tests under skip array
*/
uint32_t  g_skip_array[]   = {10000, 10000, 10000, 10000};
uint32_t  g_test_array[]   = {};
uint32_t  g_module_array[] = {};

uint32_t  g_num_skip         = sizeof(g_skip_array)/sizeof(g_skip_array[0]);
uint32_t  g_num_tests        = sizeof(g_test_array)/sizeof(g_test_array[0]);
uint32_t  g_num_modules      = sizeof(g_module_array)/sizeof(g_module_array[0]);

PE_INFO_TABLE platform_pe_cfg = {

    .header.num_of_pe = PLATFORM_OVERRIDE_PE_CNT,

    .pe_info[0].pe_num      = PLATFORM_OVERRIDE_PE0_INDEX,
    .pe_info[0].mpidr       = PLATFORM_OVERRIDE_PE0_MPIDR,
    .pe_info[0].pmu_gsiv    = PLATFORM_OVERRIDE_PE0_PMU_GSIV,
    .pe_info[0].gmain_gsiv  = PLATFORM_OVERRIDE_PE0_GMAIN_GSIV,

    .pe_info[1].pe_num      = PLATFORM_OVERRIDE_PE1_INDEX,
    .pe_info[1].mpidr       = PLATFORM_OVERRIDE_PE1_MPIDR,
    .pe_info[1].pmu_gsiv    = PLATFORM_OVERRIDE_PE1_PMU_GSIV,
    .pe_info[1].gmain_gsiv  = PLATFORM_OVERRIDE_PE1_GMAIN_GSIV,

    .pe_info[2].pe_num      = PLATFORM_OVERRIDE_PE2_INDEX,
    .pe_info[2].mpidr       = PLATFORM_OVERRIDE_PE2_MPIDR,
    .pe_info[2].pmu_gsiv    = PLATFORM_OVERRIDE_PE2_PMU_GSIV,
    .pe_info[2].gmain_gsiv  = PLATFORM_OVERRIDE_PE2_GMAIN_GSIV,

    .pe_info[3].pe_num      = PLATFORM_OVERRIDE_PE3_INDEX,
    .pe_info[3].mpidr       = PLATFORM_OVERRIDE_PE3_MPIDR,
    .pe_info[3].pmu_gsiv    = PLATFORM_OVERRIDE_PE3_PMU_GSIV,
    .pe_info[3].gmain_gsiv  = PLATFORM_OVERRIDE_PE3_GMAIN_GSIV,

    .pe_info[4].pe_num      = PLATFORM_OVERRIDE_PE4_INDEX,
    .pe_info[4].mpidr       = PLATFORM_OVERRIDE_PE4_MPIDR,
    .pe_info[4].pmu_gsiv    = PLATFORM_OVERRIDE_PE4_PMU_GSIV,
    .pe_info[4].gmain_gsiv  = PLATFORM_OVERRIDE_PE4_GMAIN_GSIV,

    .pe_info[5].pe_num      = PLATFORM_OVERRIDE_PE5_INDEX,
    .pe_info[5].mpidr       = PLATFORM_OVERRIDE_PE5_MPIDR,
    .pe_info[5].pmu_gsiv    = PLATFORM_OVERRIDE_PE5_PMU_GSIV,
    .pe_info[5].gmain_gsiv  = PLATFORM_OVERRIDE_PE5_GMAIN_GSIV,

    .pe_info[6].pe_num      = PLATFORM_OVERRIDE_PE6_INDEX,
    .pe_info[6].mpidr       = PLATFORM_OVERRIDE_PE6_MPIDR,
    .pe_info[6].pmu_gsiv    = PLATFORM_OVERRIDE_PE6_PMU_GSIV,
    .pe_info[6].gmain_gsiv  = PLATFORM_OVERRIDE_PE6_GMAIN_GSIV,

    .pe_info[7].pe_num      = PLATFORM_OVERRIDE_PE7_INDEX,
    .pe_info[7].mpidr       = PLATFORM_OVERRIDE_PE7_MPIDR,
    .pe_info[7].pmu_gsiv    = PLATFORM_OVERRIDE_PE7_PMU_GSIV,
    .pe_info[7].gmain_gsiv  = PLATFORM_OVERRIDE_PE7_GMAIN_GSIV,

    .pe_info[8].pe_num      = PLATFORM_OVERRIDE_PE8_INDEX,
    .pe_info[8].mpidr       = PLATFORM_OVERRIDE_PE8_MPIDR,
    .pe_info[8].pmu_gsiv    = PLATFORM_OVERRIDE_PE8_PMU_GSIV,
    .pe_info[8].gmain_gsiv  = PLATFORM_OVERRIDE_PE8_GMAIN_GSIV,

    .pe_info[9].pe_num      = PLATFORM_OVERRIDE_PE9_INDEX,
    .pe_info[9].mpidr       = PLATFORM_OVERRIDE_PE9_MPIDR,
    .pe_info[9].pmu_gsiv    = PLATFORM_OVERRIDE_PE9_PMU_GSIV,
    .pe_info[9].gmain_gsiv  = PLATFORM_OVERRIDE_PE9_GMAIN_GSIV,

    .pe_info[10].pe_num     = PLATFORM_OVERRIDE_PE10_INDEX,
    .pe_info[10].mpidr      = PLATFORM_OVERRIDE_PE10_MPIDR,
    .pe_info[10].pmu_gsiv   = PLATFORM_OVERRIDE_PE10_PMU_GSIV,
    .pe_info[10].gmain_gsiv = PLATFORM_OVERRIDE_PE10_GMAIN_GSIV,

    .pe_info[11].pe_num     = PLATFORM_OVERRIDE_PE11_INDEX,
    .pe_info[11].mpidr      = PLATFORM_OVERRIDE_PE11_MPIDR,
    .pe_info[11].pmu_gsiv   = PLATFORM_OVERRIDE_PE11_PMU_GSIV,
    .pe_info[11].gmain_gsiv = PLATFORM_OVERRIDE_PE11_GMAIN_GSIV,

    .pe_info[12].pe_num     = PLATFORM_OVERRIDE_PE12_INDEX,
    .pe_info[12].mpidr      = PLATFORM_OVERRIDE_PE12_MPIDR,
    .pe_info[12].pmu_gsiv   = PLATFORM_OVERRIDE_PE12_PMU_GSIV,
    .pe_info[12].gmain_gsiv = PLATFORM_OVERRIDE_PE12_GMAIN_GSIV,

    .pe_info[13].pe_num     = PLATFORM_OVERRIDE_PE13_INDEX,
    .pe_info[13].mpidr      = PLATFORM_OVERRIDE_PE13_MPIDR,
    .pe_info[13].pmu_gsiv   = PLATFORM_OVERRIDE_PE13_PMU_GSIV,
    .pe_info[13].gmain_gsiv = PLATFORM_OVERRIDE_PE13_GMAIN_GSIV,

    .pe_info[14].pe_num     = PLATFORM_OVERRIDE_PE14_INDEX,
    .pe_info[14].mpidr      = PLATFORM_OVERRIDE_PE14_MPIDR,
    .pe_info[14].pmu_gsiv   = PLATFORM_OVERRIDE_PE14_PMU_GSIV,
    .pe_info[14].gmain_gsiv = PLATFORM_OVERRIDE_PE14_GMAIN_GSIV,

    .pe_info[15].pe_num     = PLATFORM_OVERRIDE_PE15_INDEX,
    .pe_info[15].mpidr      = PLATFORM_OVERRIDE_PE15_MPIDR,
    .pe_info[15].pmu_gsiv   = PLATFORM_OVERRIDE_PE15_PMU_GSIV,
    .pe_info[15].gmain_gsiv = PLATFORM_OVERRIDE_PE15_GMAIN_GSIV,

};


PLATFORM_OVERRIDE_GIC_INFO_TABLE platform_gic_cfg = {

    .gic_version   = PLATFORM_OVERRIDE_GIC_VERSION,
    .num_gicc      = PLATFORM_OVERRIDE_GICC_COUNT,
    .num_gicd      = PLATFORM_OVERRIDE_GICD_COUNT,
    .num_gicrd     = PLATFORM_OVERRIDE_GICRD_COUNT,
    .num_gicits    = PLATFORM_OVERRIDE_GICITS_COUNT,
    .num_gich      = PLATFORM_OVERRIDE_GICH_COUNT,
    .num_msiframes = PLATFORM_OVERRIDE_GICMSIFRAME_COUNT,

    .gicrd_length = PLATFORM_OVERRIDE_GICIRD_LENGTH,

    .gicc_base[0]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[1]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[2]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[3]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[4]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[5]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[6]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[7]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[8]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[9]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[10]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[11]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[12]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[13]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[14]   = PLATFORM_OVERRIDE_GICC_BASE,
    .gicc_base[15]   = PLATFORM_OVERRIDE_GICC_BASE,

    .gicd_base[0]   = PLATFORM_OVERRIDE_GICD_BASE,
    .gicrd_base[0]  = PLATFORM_OVERRIDE_GICRD_BASE,
    .gicits_base[0] = PLATFORM_OVERRIDE_GICITS0_BASE,
    .gicits_id[0]   = PLATFORM_OVERRIDE_GICITS0_ID,
    .gicits_base[1] = PLATFORM_OVERRIDE_GICITS1_BASE,
    .gicits_id[1]   = PLATFORM_OVERRIDE_GICITS1_ID,
    .gicits_base[2] = PLATFORM_OVERRIDE_GICITS2_BASE,
    .gicits_id[2]   = PLATFORM_OVERRIDE_GICITS2_ID,
    .gicits_base[3] = PLATFORM_OVERRIDE_GICITS3_BASE,
    .gicits_id[3]   = PLATFORM_OVERRIDE_GICITS3_ID,
    .gicits_base[4] = PLATFORM_OVERRIDE_GICITS4_BASE,
    .gicits_id[4]   = PLATFORM_OVERRIDE_GICITS4_ID,
    .gicits_base[5] = PLATFORM_OVERRIDE_GICITS5_BASE,
    .gicits_id[5]   = PLATFORM_OVERRIDE_GICITS5_ID,
    .gich_base[0]   = PLATFORM_OVERRIDE_GICH_BASE

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

WD_INFO_TABLE platform_wd_cfg = {
    .header.num_wd              = PLATFORM_OVERRIDE_WD_TIMER_COUNT,
    .wd_info[0].wd_ctrl_base    = PLATFORM_OVERRIDE_WD_CTRL_BASE,
    .wd_info[0].wd_refresh_base = PLATFORM_OVERRIDE_WD_REFRESH_BASE,
    .wd_info[0].wd_gsiv         = PLATFORM_OVERRIDE_WD_GSIV_0,
    .wd_info[0].wd_flags        = PLATFORM_OVERRIDE_WD_FLAGS_0,
    .wd_info[1].wd_ctrl_base    = PLATFORM_OVERRIDE_WD_CTRL_BASE,
    .wd_info[1].wd_refresh_base = PLATFORM_OVERRIDE_WD_REFRESH_BASE,
    .wd_info[1].wd_gsiv         = PLATFORM_OVERRIDE_WD_GSIV_1,
    .wd_info[1].wd_flags        = PLATFORM_OVERRIDE_WD_FLAGS_1

};

PCIE_INFO_TABLE platform_pcie_cfg = {
    .num_entries             = PLATFORM_OVERRIDE_NUM_ECAM,
    .block[0].ecam_base      = PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_0,
    .block[0].segment_num    = PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_0,
    .block[0].start_bus_num  = PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_0,
    .block[0].end_bus_num    = PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_0,

    .block[1].ecam_base      = PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_1,
    .block[1].segment_num    = PLATFORM_OVERRIDE_PCIE_SEGMENT_GRP_NUM_1,
    .block[1].start_bus_num  = PLATFORM_OVERRIDE_PCIE_START_BUS_NUM_1,
    .block[1].end_bus_num    = PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_1
/** Configure more PCIe info details as per specification for more than 1 ECAM
    Refer to platform_override_fvp.h file for an example
**/
};

PLATFORM_OVERRIDE_IOVIRT_INFO_TABLE platform_iovirt_cfg = {
    .Address              = IOVIRT_ADDRESS,
    .node_count           = IORT_NODE_COUNT,
    .type[0]              = IOVIRT_NODE_ITS_GROUP,
    .type[1]              = IOVIRT_NODE_ITS_GROUP,
    .type[2]              = IOVIRT_NODE_ITS_GROUP,
    .type[3]              = IOVIRT_NODE_ITS_GROUP,
    .type[4]              = IOVIRT_NODE_ITS_GROUP,
    .type[5]              = IOVIRT_NODE_SMMU_V3,
    .type[6]              = IOVIRT_NODE_SMMU_V3,
    .type[7]              = IOVIRT_NODE_SMMU_V3,
    .type[8]              = IOVIRT_NODE_SMMU_V3,
    .type[9]              = IOVIRT_NODE_SMMU_V3,
    .type[10]             = IOVIRT_NODE_PCI_ROOT_COMPLEX,
    .type[11]             = IOVIRT_NODE_NAMED_COMPONENT,
    .type[12]             = IOVIRT_NODE_NAMED_COMPONENT,
    .num_map[5]           = IOVIRT_SMMUV3_0_NUM_MAP,
    .num_map[6]           = IOVIRT_SMMUV3_1_NUM_MAP,
    .num_map[7]           = IOVIRT_SMMUV3_2_NUM_MAP,
    .num_map[8]           = IOVIRT_SMMUV3_3_NUM_MAP,
    .num_map[9]           = IOVIRT_SMMUV3_4_NUM_MAP,
    .num_map[10]          = IOVIRT_RC_NUM_MAP,
    .num_map[11]          = IOVIRT_NAMED_COMP0_NUM_MAP,
    .num_map[12]          = IOVIRT_NAMED_COMP1_NUM_MAP,


    .map[5].input_base[0] = SMMUV3_0_ID_MAP0_INPUT_BASE,
    .map[5].id_count[0]   = SMMUV3_0_ID_MAP0_ID_COUNT,
    .map[5].output_base[0]= SMMUV3_0_ID_MAP0_OUTPUT_BASE,
    .map[5].output_ref[0] = SMMUV3_0_ID_MAP0_OUTPUT_REF,
    .map[5].input_base[1] = SMMUV3_0_ID_MAP1_INPUT_BASE,
    .map[5].id_count[1]   = SMMUV3_0_ID_MAP1_ID_COUNT,
    .map[5].output_base[1]= SMMUV3_0_ID_MAP1_OUTPUT_BASE,
    .map[5].output_ref[1] = SMMUV3_0_ID_MAP1_OUTPUT_REF,

    .map[6].input_base[0] = SMMUV3_1_ID_MAP0_INPUT_BASE,
    .map[6].id_count[0]   = SMMUV3_1_ID_MAP0_ID_COUNT,
    .map[6].output_base[0]= SMMUV3_1_ID_MAP0_OUTPUT_BASE,
    .map[6].output_ref[0] = SMMUV3_1_ID_MAP0_OUTPUT_REF,
    .map[6].input_base[1] = SMMUV3_1_ID_MAP1_INPUT_BASE,
    .map[6].id_count[1]   = SMMUV3_1_ID_MAP1_ID_COUNT,
    .map[6].output_base[1]= SMMUV3_1_ID_MAP1_OUTPUT_BASE,
    .map[6].output_ref[1] = SMMUV3_1_ID_MAP1_OUTPUT_REF,


    .map[7].input_base[0] = SMMUV3_2_ID_MAP0_INPUT_BASE,
    .map[7].id_count[0]   = SMMUV3_2_ID_MAP0_ID_COUNT,
    .map[7].output_base[0]= SMMUV3_2_ID_MAP0_OUTPUT_BASE,
    .map[7].output_ref[0] = SMMUV3_2_ID_MAP0_OUTPUT_REF,
    .map[7].input_base[1] = SMMUV3_2_ID_MAP1_INPUT_BASE,
    .map[7].id_count[1]   = SMMUV3_2_ID_MAP1_ID_COUNT,
    .map[7].output_base[1]= SMMUV3_2_ID_MAP1_OUTPUT_BASE,
    .map[7].output_ref[1] = SMMUV3_2_ID_MAP1_OUTPUT_REF,

    .map[8].input_base[0] = SMMUV3_3_ID_MAP0_INPUT_BASE,
    .map[8].id_count[0]   = SMMUV3_3_ID_MAP0_ID_COUNT,
    .map[8].output_base[0]= SMMUV3_3_ID_MAP0_OUTPUT_BASE,
    .map[8].output_ref[0] = SMMUV3_3_ID_MAP0_OUTPUT_REF,
    .map[8].input_base[1] = SMMUV3_3_ID_MAP1_INPUT_BASE,
    .map[8].id_count[1]   = SMMUV3_3_ID_MAP1_ID_COUNT,
    .map[8].output_base[1]= SMMUV3_3_ID_MAP1_OUTPUT_BASE,
    .map[8].output_ref[1] = SMMUV3_3_ID_MAP1_OUTPUT_REF,

    .map[9].input_base[0] = SMMUV3_4_ID_MAP0_INPUT_BASE,
    .map[9].id_count[0]   = SMMUV3_4_ID_MAP0_ID_COUNT,
    .map[9].output_base[0]= SMMUV3_4_ID_MAP0_OUTPUT_BASE,
    .map[9].output_ref[0] = SMMUV3_4_ID_MAP0_OUTPUT_REF,
    .map[9].input_base[1] = SMMUV3_4_ID_MAP1_INPUT_BASE,
    .map[9].id_count[1]   = SMMUV3_4_ID_MAP1_ID_COUNT,
    .map[9].output_base[1]= SMMUV3_4_ID_MAP1_OUTPUT_BASE,
    .map[9].output_ref[1] = SMMUV3_4_ID_MAP1_OUTPUT_REF,
    .map[9].input_base[2] = SMMUV3_4_ID_MAP2_INPUT_BASE,
    .map[9].id_count[2]   = SMMUV3_4_ID_MAP2_ID_COUNT,
    .map[9].output_base[2]= SMMUV3_4_ID_MAP2_OUTPUT_BASE,
    .map[9].output_ref[2] = SMMUV3_4_ID_MAP2_OUTPUT_REF,

    .map[10].input_base[0] = RC_MAP0_INPUT_BASE,
    .map[10].id_count[0]   = RC_MAP0_ID_COUNT,
    .map[10].output_base[0]= RC_MAP0_OUTPUT_BASE,
    .map[10].output_ref[0] = RC_MAP0_OUTPUT_REF,
    .map[10].input_base[1] = RC_MAP1_INPUT_BASE,
    .map[10].id_count[1]   = RC_MAP1_ID_COUNT,
    .map[10].output_base[1]= RC_MAP1_OUTPUT_BASE,
    .map[10].output_ref[1] = RC_MAP1_OUTPUT_REF,
    .map[10].input_base[2] = RC_MAP2_INPUT_BASE,
    .map[10].id_count[2]   = RC_MAP2_ID_COUNT,
    .map[10].output_base[2]= RC_MAP2_OUTPUT_BASE,
    .map[10].output_ref[2] = RC_MAP2_OUTPUT_REF,
    .map[10].input_base[3] = RC_MAP3_INPUT_BASE,
    .map[10].id_count[3]   = RC_MAP3_ID_COUNT,
    .map[10].output_base[3]= RC_MAP3_OUTPUT_BASE,
    .map[10].output_ref[3] = RC_MAP3_OUTPUT_REF,

    .map[11].input_base[0] = NAMED_COMP0_MAP0_INPUT_BASE,
    .map[11].id_count[0]   = NAMED_COMP0_MAP0_ID_COUNT,
    .map[11].output_base[0]= NAMED_COMP0_MAP0_OUTPUT_BASE,
    .map[11].output_ref[0] = NAMED_COMP0_MAP0_OUTPUT_REF,
    .map[11].input_base[1] = NAMED_COMP0_MAP1_INPUT_BASE,
    .map[11].id_count[1]   = NAMED_COMP0_MAP1_ID_COUNT,
    .map[11].output_base[1]= NAMED_COMP0_MAP1_OUTPUT_BASE,
    .map[11].output_ref[1] = NAMED_COMP0_MAP1_OUTPUT_REF,
    .map[11].input_base[2] = NAMED_COMP0_MAP2_INPUT_BASE,
    .map[11].id_count[2]   = NAMED_COMP0_MAP2_ID_COUNT,
    .map[11].output_base[2]= NAMED_COMP0_MAP2_OUTPUT_BASE,
    .map[11].output_ref[2] = NAMED_COMP0_MAP2_OUTPUT_REF,
    .map[11].input_base[3] = NAMED_COMP0_MAP3_INPUT_BASE,
    .map[11].id_count[3]   = NAMED_COMP0_MAP3_ID_COUNT,
    .map[11].output_base[3]= NAMED_COMP0_MAP3_OUTPUT_BASE,
    .map[11].output_ref[3] = NAMED_COMP0_MAP3_OUTPUT_REF,
    .map[11].input_base[4] = NAMED_COMP0_MAP4_INPUT_BASE,
    .map[11].id_count[4]   = NAMED_COMP0_MAP4_ID_COUNT,
    .map[11].output_base[4]= NAMED_COMP0_MAP4_OUTPUT_BASE,
    .map[11].output_ref[4] = NAMED_COMP0_MAP4_OUTPUT_REF,
    .map[11].input_base[5] = NAMED_COMP0_MAP5_INPUT_BASE,
    .map[11].id_count[5]   = NAMED_COMP0_MAP5_ID_COUNT,
    .map[11].output_base[5]= NAMED_COMP0_MAP5_OUTPUT_BASE,
    .map[11].output_ref[5] = NAMED_COMP0_MAP5_OUTPUT_REF,
    .map[11].input_base[6] = NAMED_COMP0_MAP6_INPUT_BASE,
    .map[11].id_count[6]   = NAMED_COMP0_MAP6_ID_COUNT,
    .map[11].output_base[6]= NAMED_COMP0_MAP6_OUTPUT_BASE,
    .map[11].output_ref[6] = NAMED_COMP0_MAP6_OUTPUT_REF,
    .map[11].input_base[7] = NAMED_COMP0_MAP7_INPUT_BASE,
    .map[11].id_count[7]   = NAMED_COMP0_MAP7_ID_COUNT,
    .map[11].output_base[7]= NAMED_COMP0_MAP7_OUTPUT_BASE,
    .map[11].output_ref[7] = NAMED_COMP0_MAP7_OUTPUT_REF,
    .map[11].input_base[8] = NAMED_COMP0_MAP8_INPUT_BASE,
    .map[11].id_count[8]   = NAMED_COMP0_MAP8_ID_COUNT,
    .map[11].output_base[8]= NAMED_COMP0_MAP8_OUTPUT_BASE,
    .map[11].output_ref[8] = NAMED_COMP0_MAP8_OUTPUT_REF,

    .map[12].input_base[0] = NAMED_COMP1_MAP0_INPUT_BASE,
    .map[12].id_count[0]   = NAMED_COMP1_MAP0_ID_COUNT,
    .map[12].output_base[0]= NAMED_COMP1_MAP0_OUTPUT_BASE,
    .map[12].output_ref[0] = NAMED_COMP1_MAP0_OUTPUT_REF,
    .map[12].input_base[1] = NAMED_COMP1_MAP1_INPUT_BASE,
    .map[12].id_count[1]   = NAMED_COMP1_MAP1_ID_COUNT,
    .map[12].output_base[1]= NAMED_COMP1_MAP1_OUTPUT_BASE,
    .map[12].output_ref[1] = NAMED_COMP1_MAP1_OUTPUT_REF,
    .map[12].input_base[2] = NAMED_COMP1_MAP2_INPUT_BASE,
    .map[12].id_count[2]   = NAMED_COMP1_MAP2_ID_COUNT,
    .map[12].output_base[2]= NAMED_COMP1_MAP2_OUTPUT_BASE,
    .map[12].output_ref[2] = NAMED_COMP1_MAP2_OUTPUT_REF,
    .map[12].input_base[3] = NAMED_COMP1_MAP3_INPUT_BASE,
    .map[12].id_count[3]   = NAMED_COMP1_MAP3_ID_COUNT,
    .map[12].output_base[3]= NAMED_COMP1_MAP3_OUTPUT_BASE,
    .map[12].output_ref[3] = NAMED_COMP1_MAP3_OUTPUT_REF,
    .map[12].input_base[4] = NAMED_COMP1_MAP4_INPUT_BASE,
    .map[12].id_count[4]   = NAMED_COMP1_MAP4_ID_COUNT,
    .map[12].output_base[4]= NAMED_COMP1_MAP4_OUTPUT_BASE,
    .map[12].output_ref[4] = NAMED_COMP1_MAP4_OUTPUT_REF,
    .map[12].input_base[5] = NAMED_COMP1_MAP5_INPUT_BASE,
    .map[12].id_count[5]   = NAMED_COMP1_MAP5_ID_COUNT,
    .map[12].output_base[5]= NAMED_COMP1_MAP5_OUTPUT_BASE,
    .map[12].output_ref[5] = NAMED_COMP1_MAP5_OUTPUT_REF,
    .map[12].input_base[6] = NAMED_COMP1_MAP6_INPUT_BASE,
    .map[12].id_count[6]   = NAMED_COMP1_MAP6_ID_COUNT,
    .map[12].output_base[6]= NAMED_COMP1_MAP6_OUTPUT_BASE,
    .map[12].output_ref[6] = NAMED_COMP1_MAP6_OUTPUT_REF,
    .map[12].input_base[7] = NAMED_COMP1_MAP7_INPUT_BASE,
    .map[12].id_count[7]   = NAMED_COMP1_MAP7_ID_COUNT,
    .map[12].output_base[7]= NAMED_COMP1_MAP7_OUTPUT_BASE,
    .map[12].output_ref[7] = NAMED_COMP1_MAP7_OUTPUT_REF,
    .map[12].input_base[8] = NAMED_COMP1_MAP8_INPUT_BASE,
    .map[12].id_count[8]   = NAMED_COMP1_MAP8_ID_COUNT,
    .map[12].output_base[8]= NAMED_COMP1_MAP8_OUTPUT_BASE,
    .map[12].output_ref[8] = NAMED_COMP1_MAP8_OUTPUT_REF,

};

PLATFORM_OVERRIDE_NODE_DATA platform_node_type = {
    .its_count                        = IOVIRT_ITS_COUNT,
    .rc.segment                       = IOVIRT_RC_PCI_SEG_NUM,
    .rc.cca                           = IOVIRT_RC_MEMORY_PROPERTIES,
    .rc.ats_attr                      = IOVIRT_RC_ATS_ATTRIBUTE

};

PLATFORM_OVERRIDE_SMMU_NODE_DATA platform_smmu_node_data = {
    .smmu[0].base                     = IOVIRT_SMMUV3_0_BASE_ADDRESS,
    .smmu[1].base                     = IOVIRT_SMMUV3_1_BASE_ADDRESS,
    .smmu[2].base                     = IOVIRT_SMMUV3_2_BASE_ADDRESS,
    .smmu[3].base                     = IOVIRT_SMMUV3_3_BASE_ADDRESS,
    .smmu[4].base                     = IOVIRT_SMMUV3_4_BASE_ADDRESS,
    .smmu[0].context_interrupt_offset = IOVIRT_SMMU_CTX_INT_OFFSET,
    .smmu[0].context_interrupt_count  = IOVIRT_SMMU_CTX_INT_CNT,

};

PLATFORM_OVERRIDE_PMCG_NODE_DATA platform_pmcg_node_data = {
    /* Place holder Fill this as below if PMCG is present
    .pmcg[0].base          = IOVIRT_PMCG_0_BASE_ADDRESS,
    .pmcg[0].overflow_gsiv = IOVIRT_PMCG_0_OVERFLOW_GSIV,
    .pmcg[0].node_ref      = IOVIRT_PMCG_0_NODE_REFERENCE,
    .pmcg[0].smmu_base     = IOVIRT_PMCG_0_SMMU_BASE,*/
};

PLATFORM_OVERRIDE_NAMED_NODE_DATA platform_named_node_data = {
    .named[0].smmu_base         = IOVIRT_NAMED_0_SMMU_BASE,
    .named[0].memory_properties = IOVIRT_NAMED_0_MEM_PROP,
    .named[0].name              = IOVIRT_NAMED_0_DEVICE_NAME,
    .named[1].smmu_base         = IOVIRT_NAMED_1_SMMU_BASE,
    .named[1].memory_properties = IOVIRT_NAMED_1_MEM_PROP,
    .named[1].name              = IOVIRT_NAMED_1_DEVICE_NAME,
};

PLATFORM_OVERRIDE_CS_COMP_NODE_DATA platform_cs_comp_node_data = {
    .component[0].identifier    = CS_COMPONENT_0_IDENTIFIER,
    .component[0].dev_name      = CS_COMPONENT_0_DEVICE_NAME,
};

PLATFORM_OVERRIDE_UART_INFO_TABLE platform_uart_cfg = {
    .Address               = UART_ADDRESS,
    .BaseAddress.Address   = BASE_ADDRESS_ADDRESS,
    .GlobalSystemInterrupt = UART_GLOBAL_SYSTEM_INTERRUPT,
    .PciDeviceId           = UART_PCI_DEVICE_ID,
    .PciVendorId           = UART_PCI_VENDOR_ID,
    .PciBusNumber          = UART_PCI_BUS_NUMBER,
    .PciDeviceNumber       = UART_PCI_DEV_NUMBER,
    .PciFunctionNumber     = UART_PCI_FUNC_NUMBER,
    .PciFlags              = UART_PCI_FLAGS,
    .PciSegment            = UART_PCI_SEGMENT
};

DMA_INFO_TABLE platform_dma_cfg = {
    .num_dma_ctrls = PLATFORM_OVERRIDE_DMA_CNT

    /** Place holder
    .info[0].target = TARGET,
    .info[0].port = PORT,
    .info[0].host = HOST,
    .info[0].flags = FLAGS,
    .info[0].type = TYPE**/

};

PLATFORM_OVERRIDE_MEMORY_INFO_TABLE platform_mem_cfg = {
    .count                   = PLATFORM_OVERRIDE_MEMORY_ENTRY_COUNT,
    .info[0].phy_addr        = PLATFORM_OVERRIDE_MEMORY_ENTRY0_PHY_ADDR,
    .info[0].virt_addr       = PLATFORM_OVERRIDE_MEMORY_ENTRY0_VIRT_ADDR,
    .info[0].size            = PLATFORM_OVERRIDE_MEMORY_ENTRY0_SIZE,
    .info[0].type            = PLATFORM_OVERRIDE_MEMORY_ENTRY0_TYPE,
    .info[1].phy_addr        = PLATFORM_OVERRIDE_MEMORY_ENTRY1_PHY_ADDR,
    .info[1].virt_addr       = PLATFORM_OVERRIDE_MEMORY_ENTRY1_VIRT_ADDR,
    .info[1].size            = PLATFORM_OVERRIDE_MEMORY_ENTRY1_SIZE,
    .info[1].type            = PLATFORM_OVERRIDE_MEMORY_ENTRY1_TYPE,
    .info[2].phy_addr        = PLATFORM_OVERRIDE_MEMORY_ENTRY2_PHY_ADDR,
    .info[2].virt_addr       = PLATFORM_OVERRIDE_MEMORY_ENTRY2_VIRT_ADDR,
    .info[2].size            = PLATFORM_OVERRIDE_MEMORY_ENTRY2_SIZE,
    .info[2].type            = PLATFORM_OVERRIDE_MEMORY_ENTRY2_TYPE,
};

PCIE_READ_TABLE platform_pcie_device_hierarchy = {
    .num_entries             = PLATFORM_PCIE_NUM_ENTRIES,

    .device[0].class_code    = PLATFORM_PCIE_DEV0_CLASSCODE,
    .device[0].vendor_id     = PLATFORM_PCIE_DEV0_VENDOR_ID,
    .device[0].device_id     = PLATFORM_PCIE_DEV0_DEV_ID,
    .device[0].bus           = PLATFORM_PCIE_DEV0_BUS_NUM,
    .device[0].dev           = PLATFORM_PCIE_DEV0_DEV_NUM,
    .device[0].func          = PLATFORM_PCIE_DEV0_FUNC_NUM,
    .device[0].seg           = PLATFORM_PCIE_DEV0_SEG_NUM,
    .device[0].dma_support   = PLATFORM_PCIE_DEV0_DMA_SUPPORT,
    .device[0].dma_coherent  = PLATFORM_PCIE_DEV0_DMA_COHERENT,
    .device[0].p2p_support   = PLATFORM_PCIE_DEV0_P2P_SUPPORT,
    .device[0].dma_64bit     = PLATFORM_PCIE_DEV0_DMA_64BIT,
    .device[0].behind_smmu   = PLATFORM_PCIE_DEV0_BEHIND_SMMU,
    .device[0].atc_present   = PLATFORM_PCIE_DEV0_ATC_SUPPORT,

    .device[1].class_code    = PLATFORM_PCIE_DEV1_CLASSCODE,
    .device[1].vendor_id     = PLATFORM_PCIE_DEV1_VENDOR_ID,
    .device[1].device_id     = PLATFORM_PCIE_DEV1_DEV_ID,
    .device[1].bus           = PLATFORM_PCIE_DEV1_BUS_NUM,
    .device[1].dev           = PLATFORM_PCIE_DEV1_DEV_NUM,
    .device[1].func          = PLATFORM_PCIE_DEV1_FUNC_NUM,
    .device[1].seg           = PLATFORM_PCIE_DEV1_SEG_NUM,
    .device[1].dma_support   = PLATFORM_PCIE_DEV1_DMA_SUPPORT,
    .device[1].dma_coherent  = PLATFORM_PCIE_DEV1_DMA_COHERENT,
    .device[1].p2p_support   = PLATFORM_PCIE_DEV1_P2P_SUPPORT,
    .device[1].dma_64bit     = PLATFORM_PCIE_DEV1_DMA_64BIT,
    .device[1].behind_smmu   = PLATFORM_PCIE_DEV1_BEHIND_SMMU,
    .device[1].atc_present   = PLATFORM_PCIE_DEV1_ATC_SUPPORT,

    .device[2].class_code    = PLATFORM_PCIE_DEV2_CLASSCODE,
    .device[2].vendor_id     = PLATFORM_PCIE_DEV2_VENDOR_ID,
    .device[2].device_id     = PLATFORM_PCIE_DEV2_DEV_ID,
    .device[2].bus           = PLATFORM_PCIE_DEV2_BUS_NUM,
    .device[2].dev           = PLATFORM_PCIE_DEV2_DEV_NUM,
    .device[2].func          = PLATFORM_PCIE_DEV2_FUNC_NUM,
    .device[2].seg           = PLATFORM_PCIE_DEV2_SEG_NUM,
    .device[2].dma_support   = PLATFORM_PCIE_DEV2_DMA_SUPPORT,
    .device[2].dma_coherent  = PLATFORM_PCIE_DEV2_DMA_COHERENT,
    .device[2].p2p_support   = PLATFORM_PCIE_DEV2_P2P_SUPPORT,
    .device[2].dma_64bit     = PLATFORM_PCIE_DEV2_DMA_64BIT,
    .device[2].behind_smmu   = PLATFORM_PCIE_DEV2_BEHIND_SMMU,
    .device[2].atc_present   = PLATFORM_PCIE_DEV2_ATC_SUPPORT,

};

PLATFORM_OVERRIDE_CACHE_INFO_TABLE platform_cache_cfg = {

    .num_of_cache                     = PLATFORM_OVERRIDE_CACHE_CNT,

    .cache_info[0].flags              = PLATFORM_CACHE0_FLAGS,
    .cache_info[0].offset             = PLATFORM_CACHE0_OFFSET,
    .cache_info[0].next_level_index   = PLATFORM_CACHE0_NEXT_LEVEL_INDEX,
    .cache_info[0].size               = PLATFORM_CACHE0_SIZE,
    .cache_info[0].cache_id           = PLATFORM_CACHE0_CACHE_ID,
    .cache_info[0].is_private         = PLATFORM_CACHE0_IS_PRIVATE,
    .cache_info[0].cache_type         = PLATFORM_CACHE0_TYPE,

    .cache_info[1].flags              = PLATFORM_CACHE1_FLAGS,
    .cache_info[1].offset             = PLATFORM_CACHE1_OFFSET,
    .cache_info[1].next_level_index   = PLATFORM_CACHE1_NEXT_LEVEL_INDEX,
    .cache_info[1].size               = PLATFORM_CACHE1_SIZE,
    .cache_info[1].cache_id           = PLATFORM_CACHE1_CACHE_ID,
    .cache_info[1].is_private         = PLATFORM_CACHE1_IS_PRIVATE,
    .cache_info[1].cache_type         = PLATFORM_CACHE1_TYPE,

    .cache_info[2].flags              = PLATFORM_CACHE2_FLAGS,
    .cache_info[2].offset             = PLATFORM_CACHE2_OFFSET,
    .cache_info[2].next_level_index   = PLATFORM_CACHE2_NEXT_LEVEL_INDEX,
    .cache_info[2].size               = PLATFORM_CACHE2_SIZE,
    .cache_info[2].cache_id           = PLATFORM_CACHE2_CACHE_ID,
    .cache_info[2].is_private         = PLATFORM_CACHE2_IS_PRIVATE,
    .cache_info[2].cache_type         = PLATFORM_CACHE2_TYPE,

    .cache_info[3].flags              = PLATFORM_CACHE3_FLAGS,
    .cache_info[3].offset             = PLATFORM_CACHE3_OFFSET,
    .cache_info[3].next_level_index   = PLATFORM_CACHE3_NEXT_LEVEL_INDEX,
    .cache_info[3].size               = PLATFORM_CACHE3_SIZE,
    .cache_info[3].cache_id           = PLATFORM_CACHE3_CACHE_ID,
    .cache_info[3].is_private         = PLATFORM_CACHE3_IS_PRIVATE,
    .cache_info[3].cache_type         = PLATFORM_CACHE3_TYPE,

    .cache_info[4].flags              = PLATFORM_CACHE4_FLAGS,
    .cache_info[4].offset             = PLATFORM_CACHE4_OFFSET,
    .cache_info[4].next_level_index   = PLATFORM_CACHE4_NEXT_LEVEL_INDEX,
    .cache_info[4].size               = PLATFORM_CACHE4_SIZE,
    .cache_info[4].cache_id           = PLATFORM_CACHE4_CACHE_ID,
    .cache_info[4].is_private         = PLATFORM_CACHE4_IS_PRIVATE,
    .cache_info[4].cache_type         = PLATFORM_CACHE4_TYPE,

    .cache_info[5].flags              = PLATFORM_CACHE5_FLAGS,
    .cache_info[5].offset             = PLATFORM_CACHE5_OFFSET,
    .cache_info[5].next_level_index   = PLATFORM_CACHE5_NEXT_LEVEL_INDEX,
    .cache_info[5].size               = PLATFORM_CACHE5_SIZE,
    .cache_info[5].cache_id           = PLATFORM_CACHE5_CACHE_ID,
    .cache_info[5].is_private         = PLATFORM_CACHE5_IS_PRIVATE,
    .cache_info[5].cache_type         = PLATFORM_CACHE5_TYPE,

    .cache_info[6].flags              = PLATFORM_CACHE6_FLAGS,
    .cache_info[6].offset             = PLATFORM_CACHE6_OFFSET,
    .cache_info[6].next_level_index   = PLATFORM_CACHE6_NEXT_LEVEL_INDEX,
    .cache_info[6].size               = PLATFORM_CACHE6_SIZE,
    .cache_info[6].cache_id           = PLATFORM_CACHE6_CACHE_ID,
    .cache_info[6].is_private         = PLATFORM_CACHE6_IS_PRIVATE,
    .cache_info[6].cache_type         = PLATFORM_CACHE6_TYPE,

    .cache_info[7].flags              = PLATFORM_CACHE7_FLAGS,
    .cache_info[7].offset             = PLATFORM_CACHE7_OFFSET,
    .cache_info[7].next_level_index   = PLATFORM_CACHE7_NEXT_LEVEL_INDEX,
    .cache_info[7].size               = PLATFORM_CACHE7_SIZE,
    .cache_info[7].cache_id           = PLATFORM_CACHE7_CACHE_ID,
    .cache_info[7].is_private         = PLATFORM_CACHE7_IS_PRIVATE,
    .cache_info[7].cache_type         = PLATFORM_CACHE7_TYPE,

    .cache_info[8].flags              = PLATFORM_CACHE8_FLAGS,
    .cache_info[8].offset             = PLATFORM_CACHE8_OFFSET,
    .cache_info[8].next_level_index   = PLATFORM_CACHE8_NEXT_LEVEL_INDEX,
    .cache_info[8].size               = PLATFORM_CACHE8_SIZE,
    .cache_info[8].cache_id           = PLATFORM_CACHE8_CACHE_ID,
    .cache_info[8].is_private         = PLATFORM_CACHE8_IS_PRIVATE,
    .cache_info[8].cache_type         = PLATFORM_CACHE8_TYPE,

    .cache_info[9].flags              = PLATFORM_CACHE9_FLAGS,
    .cache_info[9].offset             = PLATFORM_CACHE9_OFFSET,
    .cache_info[9].next_level_index   = PLATFORM_CACHE9_NEXT_LEVEL_INDEX,
    .cache_info[9].size               = PLATFORM_CACHE9_SIZE,
    .cache_info[9].cache_id           = PLATFORM_CACHE9_CACHE_ID,
    .cache_info[9].is_private         = PLATFORM_CACHE9_IS_PRIVATE,
    .cache_info[9].cache_type         = PLATFORM_CACHE9_TYPE,

    .cache_info[10].flags              = PLATFORM_CACHE10_FLAGS,
    .cache_info[10].offset             = PLATFORM_CACHE10_OFFSET,
    .cache_info[10].next_level_index   = PLATFORM_CACHE10_NEXT_LEVEL_INDEX,
    .cache_info[10].size               = PLATFORM_CACHE10_SIZE,
    .cache_info[10].cache_id           = PLATFORM_CACHE10_CACHE_ID,
    .cache_info[10].is_private         = PLATFORM_CACHE10_IS_PRIVATE,
    .cache_info[10].cache_type         = PLATFORM_CACHE10_TYPE,

    .cache_info[11].flags              = PLATFORM_CACHE11_FLAGS,
    .cache_info[11].offset             = PLATFORM_CACHE11_OFFSET,
    .cache_info[11].next_level_index   = PLATFORM_CACHE11_NEXT_LEVEL_INDEX,
    .cache_info[11].size               = PLATFORM_CACHE11_SIZE,
    .cache_info[11].cache_id           = PLATFORM_CACHE11_CACHE_ID,
    .cache_info[11].is_private         = PLATFORM_CACHE11_IS_PRIVATE,
    .cache_info[11].cache_type         = PLATFORM_CACHE11_TYPE,

    .cache_info[12].flags              = PLATFORM_CACHE12_FLAGS,
    .cache_info[12].offset             = PLATFORM_CACHE12_OFFSET,
    .cache_info[12].next_level_index   = PLATFORM_CACHE12_NEXT_LEVEL_INDEX,
    .cache_info[12].size               = PLATFORM_CACHE12_SIZE,
    .cache_info[12].cache_id           = PLATFORM_CACHE12_CACHE_ID,
    .cache_info[12].is_private         = PLATFORM_CACHE12_IS_PRIVATE,
    .cache_info[12].cache_type         = PLATFORM_CACHE12_TYPE,

    .cache_info[13].flags              = PLATFORM_CACHE13_FLAGS,
    .cache_info[13].offset             = PLATFORM_CACHE13_OFFSET,
    .cache_info[13].next_level_index   = PLATFORM_CACHE13_NEXT_LEVEL_INDEX,
    .cache_info[13].size               = PLATFORM_CACHE13_SIZE,
    .cache_info[13].cache_id           = PLATFORM_CACHE13_CACHE_ID,
    .cache_info[13].is_private         = PLATFORM_CACHE13_IS_PRIVATE,
    .cache_info[13].cache_type         = PLATFORM_CACHE13_TYPE,

    .cache_info[14].flags              = PLATFORM_CACHE14_FLAGS,
    .cache_info[14].offset             = PLATFORM_CACHE14_OFFSET,
    .cache_info[14].next_level_index   = PLATFORM_CACHE14_NEXT_LEVEL_INDEX,
    .cache_info[14].size               = PLATFORM_CACHE14_SIZE,
    .cache_info[14].cache_id           = PLATFORM_CACHE14_CACHE_ID,
    .cache_info[14].is_private         = PLATFORM_CACHE14_IS_PRIVATE,
    .cache_info[14].cache_type         = PLATFORM_CACHE14_TYPE,

    .cache_info[15].flags              = PLATFORM_CACHE15_FLAGS,
    .cache_info[15].offset             = PLATFORM_CACHE15_OFFSET,
    .cache_info[15].next_level_index   = PLATFORM_CACHE15_NEXT_LEVEL_INDEX,
    .cache_info[15].size               = PLATFORM_CACHE15_SIZE,
    .cache_info[15].cache_id           = PLATFORM_CACHE15_CACHE_ID,
    .cache_info[15].is_private         = PLATFORM_CACHE15_IS_PRIVATE,
    .cache_info[15].cache_type         = PLATFORM_CACHE15_TYPE,

    .cache_info[16].flags              = PLATFORM_CACHE16_FLAGS,
    .cache_info[16].offset             = PLATFORM_CACHE16_OFFSET,
    .cache_info[16].next_level_index   = PLATFORM_CACHE16_NEXT_LEVEL_INDEX,
    .cache_info[16].size               = PLATFORM_CACHE16_SIZE,
    .cache_info[16].cache_id           = PLATFORM_CACHE16_CACHE_ID,
    .cache_info[16].is_private         = PLATFORM_CACHE16_IS_PRIVATE,
    .cache_info[16].cache_type         = PLATFORM_CACHE16_TYPE,

    .cache_info[17].flags              = PLATFORM_CACHE17_FLAGS,
    .cache_info[17].offset             = PLATFORM_CACHE17_OFFSET,
    .cache_info[17].next_level_index   = PLATFORM_CACHE17_NEXT_LEVEL_INDEX,
    .cache_info[17].size               = PLATFORM_CACHE17_SIZE,
    .cache_info[17].cache_id           = PLATFORM_CACHE17_CACHE_ID,
    .cache_info[17].is_private         = PLATFORM_CACHE17_IS_PRIVATE,
    .cache_info[17].cache_type         = PLATFORM_CACHE17_TYPE,

    .cache_info[18].flags              = PLATFORM_CACHE18_FLAGS,
    .cache_info[18].offset             = PLATFORM_CACHE18_OFFSET,
    .cache_info[18].next_level_index   = PLATFORM_CACHE18_NEXT_LEVEL_INDEX,
    .cache_info[18].size               = PLATFORM_CACHE18_SIZE,
    .cache_info[18].cache_id           = PLATFORM_CACHE18_CACHE_ID,
    .cache_info[18].is_private         = PLATFORM_CACHE18_IS_PRIVATE,
    .cache_info[18].cache_type         = PLATFORM_CACHE18_TYPE,

    .cache_info[19].flags              = PLATFORM_CACHE19_FLAGS,
    .cache_info[19].offset             = PLATFORM_CACHE19_OFFSET,
    .cache_info[19].next_level_index   = PLATFORM_CACHE19_NEXT_LEVEL_INDEX,
    .cache_info[19].size               = PLATFORM_CACHE19_SIZE,
    .cache_info[19].cache_id           = PLATFORM_CACHE19_CACHE_ID,
    .cache_info[19].is_private         = PLATFORM_CACHE19_IS_PRIVATE,
    .cache_info[19].cache_type         = PLATFORM_CACHE19_TYPE,

    .cache_info[20].flags              = PLATFORM_CACHE20_FLAGS,
    .cache_info[20].offset             = PLATFORM_CACHE20_OFFSET,
    .cache_info[20].next_level_index   = PLATFORM_CACHE20_NEXT_LEVEL_INDEX,
    .cache_info[20].size               = PLATFORM_CACHE20_SIZE,
    .cache_info[20].cache_id           = PLATFORM_CACHE20_CACHE_ID,
    .cache_info[20].is_private         = PLATFORM_CACHE20_IS_PRIVATE,
    .cache_info[20].cache_type         = PLATFORM_CACHE20_TYPE,

    .cache_info[21].flags              = PLATFORM_CACHE21_FLAGS,
    .cache_info[21].offset             = PLATFORM_CACHE21_OFFSET,
    .cache_info[21].next_level_index   = PLATFORM_CACHE21_NEXT_LEVEL_INDEX,
    .cache_info[21].size               = PLATFORM_CACHE21_SIZE,
    .cache_info[21].cache_id           = PLATFORM_CACHE21_CACHE_ID,
    .cache_info[21].is_private         = PLATFORM_CACHE21_IS_PRIVATE,
    .cache_info[21].cache_type         = PLATFORM_CACHE21_TYPE,

    .cache_info[22].flags              = PLATFORM_CACHE22_FLAGS,
    .cache_info[22].offset             = PLATFORM_CACHE22_OFFSET,
    .cache_info[22].next_level_index   = PLATFORM_CACHE22_NEXT_LEVEL_INDEX,
    .cache_info[22].size               = PLATFORM_CACHE22_SIZE,
    .cache_info[22].cache_id           = PLATFORM_CACHE22_CACHE_ID,
    .cache_info[22].is_private         = PLATFORM_CACHE22_IS_PRIVATE,
    .cache_info[22].cache_type         = PLATFORM_CACHE22_TYPE,

    .cache_info[23].flags              = PLATFORM_CACHE23_FLAGS,
    .cache_info[23].offset             = PLATFORM_CACHE23_OFFSET,
    .cache_info[23].next_level_index   = PLATFORM_CACHE23_NEXT_LEVEL_INDEX,
    .cache_info[23].size               = PLATFORM_CACHE23_SIZE,
    .cache_info[23].cache_id           = PLATFORM_CACHE23_CACHE_ID,
    .cache_info[23].is_private         = PLATFORM_CACHE23_IS_PRIVATE,
    .cache_info[23].cache_type         = PLATFORM_CACHE23_TYPE,

    .cache_info[24].flags              = PLATFORM_CACHE24_FLAGS,
    .cache_info[24].offset             = PLATFORM_CACHE24_OFFSET,
    .cache_info[24].next_level_index   = PLATFORM_CACHE24_NEXT_LEVEL_INDEX,
    .cache_info[24].size               = PLATFORM_CACHE24_SIZE,
    .cache_info[24].cache_id           = PLATFORM_CACHE24_CACHE_ID,
    .cache_info[24].is_private         = PLATFORM_CACHE24_IS_PRIVATE,
    .cache_info[24].cache_type         = PLATFORM_CACHE24_TYPE,

    .cache_info[25].flags              = PLATFORM_CACHE25_FLAGS,
    .cache_info[25].offset             = PLATFORM_CACHE25_OFFSET,
    .cache_info[25].next_level_index   = PLATFORM_CACHE25_NEXT_LEVEL_INDEX,
    .cache_info[25].size               = PLATFORM_CACHE25_SIZE,
    .cache_info[25].cache_id           = PLATFORM_CACHE25_CACHE_ID,
    .cache_info[25].is_private         = PLATFORM_CACHE25_IS_PRIVATE,
    .cache_info[25].cache_type         = PLATFORM_CACHE25_TYPE,

    .cache_info[26].flags              = PLATFORM_CACHE26_FLAGS,
    .cache_info[26].offset             = PLATFORM_CACHE26_OFFSET,
    .cache_info[26].next_level_index   = PLATFORM_CACHE26_NEXT_LEVEL_INDEX,
    .cache_info[26].size               = PLATFORM_CACHE26_SIZE,
    .cache_info[26].cache_id           = PLATFORM_CACHE26_CACHE_ID,
    .cache_info[26].is_private         = PLATFORM_CACHE26_IS_PRIVATE,
    .cache_info[26].cache_type         = PLATFORM_CACHE26_TYPE,

    .cache_info[27].flags              = PLATFORM_CACHE27_FLAGS,
    .cache_info[27].offset             = PLATFORM_CACHE27_OFFSET,
    .cache_info[27].next_level_index   = PLATFORM_CACHE27_NEXT_LEVEL_INDEX,
    .cache_info[27].size               = PLATFORM_CACHE27_SIZE,
    .cache_info[27].cache_id           = PLATFORM_CACHE27_CACHE_ID,
    .cache_info[27].is_private         = PLATFORM_CACHE27_IS_PRIVATE,
    .cache_info[27].cache_type         = PLATFORM_CACHE27_TYPE,

    .cache_info[28].flags              = PLATFORM_CACHE28_FLAGS,
    .cache_info[28].offset             = PLATFORM_CACHE28_OFFSET,
    .cache_info[28].next_level_index   = PLATFORM_CACHE28_NEXT_LEVEL_INDEX,
    .cache_info[28].size               = PLATFORM_CACHE28_SIZE,
    .cache_info[28].cache_id           = PLATFORM_CACHE28_CACHE_ID,
    .cache_info[28].is_private         = PLATFORM_CACHE28_IS_PRIVATE,
    .cache_info[28].cache_type         = PLATFORM_CACHE28_TYPE,

    .cache_info[29].flags              = PLATFORM_CACHE29_FLAGS,
    .cache_info[29].offset             = PLATFORM_CACHE29_OFFSET,
    .cache_info[29].next_level_index   = PLATFORM_CACHE29_NEXT_LEVEL_INDEX,
    .cache_info[29].size               = PLATFORM_CACHE29_SIZE,
    .cache_info[29].cache_id           = PLATFORM_CACHE29_CACHE_ID,
    .cache_info[29].is_private         = PLATFORM_CACHE29_IS_PRIVATE,
    .cache_info[29].cache_type         = PLATFORM_CACHE29_TYPE,

    .cache_info[30].flags              = PLATFORM_CACHE30_FLAGS,
    .cache_info[30].offset             = PLATFORM_CACHE30_OFFSET,
    .cache_info[30].next_level_index   = PLATFORM_CACHE30_NEXT_LEVEL_INDEX,
    .cache_info[30].size               = PLATFORM_CACHE30_SIZE,
    .cache_info[30].cache_id           = PLATFORM_CACHE30_CACHE_ID,
    .cache_info[30].is_private         = PLATFORM_CACHE30_IS_PRIVATE,
    .cache_info[30].cache_type         = PLATFORM_CACHE30_TYPE,

    .cache_info[31].flags              = PLATFORM_CACHE31_FLAGS,
    .cache_info[31].offset             = PLATFORM_CACHE31_OFFSET,
    .cache_info[31].next_level_index   = PLATFORM_CACHE31_NEXT_LEVEL_INDEX,
    .cache_info[31].size               = PLATFORM_CACHE31_SIZE,
    .cache_info[31].cache_id           = PLATFORM_CACHE31_CACHE_ID,
    .cache_info[31].is_private         = PLATFORM_CACHE31_IS_PRIVATE,
    .cache_info[31].cache_type         = PLATFORM_CACHE31_TYPE,

    .cache_info[32].flags              = PLATFORM_CACHE32_FLAGS,
    .cache_info[32].offset             = PLATFORM_CACHE32_OFFSET,
    .cache_info[32].next_level_index   = PLATFORM_CACHE32_NEXT_LEVEL_INDEX,
    .cache_info[32].size               = PLATFORM_CACHE32_SIZE,
    .cache_info[32].cache_id           = PLATFORM_CACHE32_CACHE_ID,
    .cache_info[32].is_private         = PLATFORM_CACHE32_IS_PRIVATE,
    .cache_info[32].cache_type         = PLATFORM_CACHE32_TYPE,

    .cache_info[33].flags              = PLATFORM_CACHE33_FLAGS,
    .cache_info[33].offset             = PLATFORM_CACHE33_OFFSET,
    .cache_info[33].next_level_index   = PLATFORM_CACHE33_NEXT_LEVEL_INDEX,
    .cache_info[33].size               = PLATFORM_CACHE33_SIZE,
    .cache_info[33].cache_id           = PLATFORM_CACHE33_CACHE_ID,
    .cache_info[33].is_private         = PLATFORM_CACHE33_IS_PRIVATE,
    .cache_info[33].cache_type         = PLATFORM_CACHE33_TYPE,

    .cache_info[34].flags              = PLATFORM_CACHE34_FLAGS,
    .cache_info[34].offset             = PLATFORM_CACHE34_OFFSET,
    .cache_info[34].next_level_index   = PLATFORM_CACHE34_NEXT_LEVEL_INDEX,
    .cache_info[34].size               = PLATFORM_CACHE34_SIZE,
    .cache_info[34].cache_id           = PLATFORM_CACHE34_CACHE_ID,
    .cache_info[34].is_private         = PLATFORM_CACHE34_IS_PRIVATE,
    .cache_info[34].cache_type         = PLATFORM_CACHE34_TYPE,

    .cache_info[35].flags              = PLATFORM_CACHE35_FLAGS,
    .cache_info[35].offset             = PLATFORM_CACHE35_OFFSET,
    .cache_info[35].next_level_index   = PLATFORM_CACHE35_NEXT_LEVEL_INDEX,
    .cache_info[35].size               = PLATFORM_CACHE35_SIZE,
    .cache_info[35].cache_id           = PLATFORM_CACHE35_CACHE_ID,
    .cache_info[35].is_private         = PLATFORM_CACHE35_IS_PRIVATE,
    .cache_info[35].cache_type         = PLATFORM_CACHE35_TYPE,

    .cache_info[36].flags              = PLATFORM_CACHE36_FLAGS,
    .cache_info[36].offset             = PLATFORM_CACHE36_OFFSET,
    .cache_info[36].next_level_index   = PLATFORM_CACHE36_NEXT_LEVEL_INDEX,
    .cache_info[36].size               = PLATFORM_CACHE36_SIZE,
    .cache_info[36].cache_id           = PLATFORM_CACHE36_CACHE_ID,
    .cache_info[36].is_private         = PLATFORM_CACHE36_IS_PRIVATE,
    .cache_info[36].cache_type         = PLATFORM_CACHE36_TYPE,

    .cache_info[37].flags              = PLATFORM_CACHE37_FLAGS,
    .cache_info[37].offset             = PLATFORM_CACHE37_OFFSET,
    .cache_info[37].next_level_index   = PLATFORM_CACHE37_NEXT_LEVEL_INDEX,
    .cache_info[37].size               = PLATFORM_CACHE37_SIZE,
    .cache_info[37].cache_id           = PLATFORM_CACHE37_CACHE_ID,
    .cache_info[37].is_private         = PLATFORM_CACHE37_IS_PRIVATE,
    .cache_info[37].cache_type         = PLATFORM_CACHE37_TYPE,

    .cache_info[38].flags              = PLATFORM_CACHE38_FLAGS,
    .cache_info[38].offset             = PLATFORM_CACHE38_OFFSET,
    .cache_info[38].next_level_index   = PLATFORM_CACHE38_NEXT_LEVEL_INDEX,
    .cache_info[38].size               = PLATFORM_CACHE38_SIZE,
    .cache_info[38].cache_id           = PLATFORM_CACHE38_CACHE_ID,
    .cache_info[38].is_private         = PLATFORM_CACHE38_IS_PRIVATE,
    .cache_info[38].cache_type         = PLATFORM_CACHE38_TYPE,

    .cache_info[39].flags              = PLATFORM_CACHE39_FLAGS,
    .cache_info[39].offset             = PLATFORM_CACHE39_OFFSET,
    .cache_info[39].next_level_index   = PLATFORM_CACHE39_NEXT_LEVEL_INDEX,
    .cache_info[39].size               = PLATFORM_CACHE39_SIZE,
    .cache_info[39].cache_id           = PLATFORM_CACHE39_CACHE_ID,
    .cache_info[39].is_private         = PLATFORM_CACHE39_IS_PRIVATE,
    .cache_info[39].cache_type         = PLATFORM_CACHE39_TYPE,

    .cache_info[40].flags              = PLATFORM_CACHE40_FLAGS,
    .cache_info[40].offset             = PLATFORM_CACHE40_OFFSET,
    .cache_info[40].next_level_index   = PLATFORM_CACHE40_NEXT_LEVEL_INDEX,
    .cache_info[40].size               = PLATFORM_CACHE40_SIZE,
    .cache_info[40].cache_id           = PLATFORM_CACHE40_CACHE_ID,
    .cache_info[40].is_private         = PLATFORM_CACHE40_IS_PRIVATE,
    .cache_info[40].cache_type         = PLATFORM_CACHE40_TYPE,

    .cache_info[41].flags              = PLATFORM_CACHE41_FLAGS,
    .cache_info[41].offset             = PLATFORM_CACHE41_OFFSET,
    .cache_info[41].next_level_index   = PLATFORM_CACHE41_NEXT_LEVEL_INDEX,
    .cache_info[41].size               = PLATFORM_CACHE41_SIZE,
    .cache_info[41].cache_id           = PLATFORM_CACHE41_CACHE_ID,
    .cache_info[41].is_private         = PLATFORM_CACHE41_IS_PRIVATE,
    .cache_info[41].cache_type         = PLATFORM_CACHE41_TYPE,

    .cache_info[42].flags              = PLATFORM_CACHE42_FLAGS,
    .cache_info[42].offset             = PLATFORM_CACHE42_OFFSET,
    .cache_info[42].next_level_index   = PLATFORM_CACHE42_NEXT_LEVEL_INDEX,
    .cache_info[42].size               = PLATFORM_CACHE42_SIZE,
    .cache_info[42].cache_id           = PLATFORM_CACHE42_CACHE_ID,
    .cache_info[42].is_private         = PLATFORM_CACHE42_IS_PRIVATE,
    .cache_info[42].cache_type         = PLATFORM_CACHE42_TYPE,

    .cache_info[43].flags              = PLATFORM_CACHE43_FLAGS,
    .cache_info[43].offset             = PLATFORM_CACHE43_OFFSET,
    .cache_info[43].next_level_index   = PLATFORM_CACHE43_NEXT_LEVEL_INDEX,
    .cache_info[43].size               = PLATFORM_CACHE43_SIZE,
    .cache_info[43].cache_id           = PLATFORM_CACHE43_CACHE_ID,
    .cache_info[43].is_private         = PLATFORM_CACHE43_IS_PRIVATE,
    .cache_info[43].cache_type         = PLATFORM_CACHE43_TYPE,

    .cache_info[44].flags              = PLATFORM_CACHE44_FLAGS,
    .cache_info[44].offset             = PLATFORM_CACHE44_OFFSET,
    .cache_info[44].next_level_index   = PLATFORM_CACHE44_NEXT_LEVEL_INDEX,
    .cache_info[44].size               = PLATFORM_CACHE44_SIZE,
    .cache_info[44].cache_id           = PLATFORM_CACHE44_CACHE_ID,
    .cache_info[44].is_private         = PLATFORM_CACHE44_IS_PRIVATE,
    .cache_info[44].cache_type         = PLATFORM_CACHE44_TYPE,

    .cache_info[45].flags              = PLATFORM_CACHE45_FLAGS,
    .cache_info[45].offset             = PLATFORM_CACHE45_OFFSET,
    .cache_info[45].next_level_index   = PLATFORM_CACHE45_NEXT_LEVEL_INDEX,
    .cache_info[45].size               = PLATFORM_CACHE45_SIZE,
    .cache_info[45].cache_id           = PLATFORM_CACHE45_CACHE_ID,
    .cache_info[45].is_private         = PLATFORM_CACHE45_IS_PRIVATE,
    .cache_info[45].cache_type         = PLATFORM_CACHE45_TYPE,

    .cache_info[46].flags              = PLATFORM_CACHE46_FLAGS,
    .cache_info[46].offset             = PLATFORM_CACHE46_OFFSET,
    .cache_info[46].next_level_index   = PLATFORM_CACHE46_NEXT_LEVEL_INDEX,
    .cache_info[46].size               = PLATFORM_CACHE46_SIZE,
    .cache_info[46].cache_id           = PLATFORM_CACHE46_CACHE_ID,
    .cache_info[46].is_private         = PLATFORM_CACHE46_IS_PRIVATE,
    .cache_info[46].cache_type         = PLATFORM_CACHE46_TYPE,

    .cache_info[47].flags              = PLATFORM_CACHE47_FLAGS,
    .cache_info[47].offset             = PLATFORM_CACHE47_OFFSET,
    .cache_info[47].next_level_index   = PLATFORM_CACHE47_NEXT_LEVEL_INDEX,
    .cache_info[47].size               = PLATFORM_CACHE47_SIZE,
    .cache_info[47].cache_id           = PLATFORM_CACHE47_CACHE_ID,
    .cache_info[47].is_private         = PLATFORM_CACHE47_IS_PRIVATE,
    .cache_info[47].cache_type         = PLATFORM_CACHE47_TYPE,

};

PLATFORM_OVERRIDE_PPTT_INFO_TABLE platform_pptt_cfg = {

    .pptt_info[0].cache_id[0]     = PLATFORM_PPTT0_CACHEID0,
    .pptt_info[0].cache_id[1]     = PLATFORM_PPTT0_CACHEID1,

    .pptt_info[1].cache_id[0]     = PLATFORM_PPTT1_CACHEID0,
    .pptt_info[1].cache_id[1]     = PLATFORM_PPTT1_CACHEID1,

    .pptt_info[2].cache_id[0]     = PLATFORM_PPTT2_CACHEID0,
    .pptt_info[2].cache_id[1]     = PLATFORM_PPTT2_CACHEID1,

    .pptt_info[3].cache_id[0]     = PLATFORM_PPTT3_CACHEID0,
    .pptt_info[3].cache_id[1]     = PLATFORM_PPTT3_CACHEID1,

    .pptt_info[4].cache_id[0]     = PLATFORM_PPTT4_CACHEID0,
    .pptt_info[4].cache_id[1]     = PLATFORM_PPTT4_CACHEID1,

    .pptt_info[5].cache_id[0]     = PLATFORM_PPTT5_CACHEID0,
    .pptt_info[5].cache_id[1]     = PLATFORM_PPTT5_CACHEID1,

    .pptt_info[6].cache_id[0]     = PLATFORM_PPTT6_CACHEID0,
    .pptt_info[6].cache_id[1]     = PLATFORM_PPTT6_CACHEID1,

    .pptt_info[7].cache_id[0]     = PLATFORM_PPTT7_CACHEID0,
    .pptt_info[7].cache_id[1]     = PLATFORM_PPTT7_CACHEID1,

    .pptt_info[8].cache_id[0]     = PLATFORM_PPTT8_CACHEID0,
    .pptt_info[8].cache_id[1]     = PLATFORM_PPTT8_CACHEID1,

    .pptt_info[9].cache_id[0]     = PLATFORM_PPTT9_CACHEID0,
    .pptt_info[9].cache_id[1]     = PLATFORM_PPTT9_CACHEID1,

    .pptt_info[10].cache_id[0]    = PLATFORM_PPTT10_CACHEID0,
    .pptt_info[10].cache_id[1]    = PLATFORM_PPTT10_CACHEID1,

    .pptt_info[11].cache_id[0]    = PLATFORM_PPTT11_CACHEID0,
    .pptt_info[11].cache_id[1]    = PLATFORM_PPTT11_CACHEID1,

    .pptt_info[12].cache_id[0]    = PLATFORM_PPTT12_CACHEID0,
    .pptt_info[12].cache_id[1]    = PLATFORM_PPTT12_CACHEID1,

    .pptt_info[13].cache_id[0]    = PLATFORM_PPTT13_CACHEID0,
    .pptt_info[13].cache_id[1]    = PLATFORM_PPTT13_CACHEID1,

    .pptt_info[14].cache_id[0]    = PLATFORM_PPTT14_CACHEID0,
    .pptt_info[14].cache_id[1]    = PLATFORM_PPTT14_CACHEID1,

    .pptt_info[15].cache_id[0]    = PLATFORM_PPTT15_CACHEID0,
    .pptt_info[15].cache_id[1]    = PLATFORM_PPTT15_CACHEID1,

};

SRAT_INFO_TABLE platform_srat_cfg = {

    .num_of_srat_entries  = PLATFORM_OVERRIDE_NUM_SRAT_ENTRIES,

    .srat_info[0].node_type    = SRAT_NODE_MEM_AFF,
    .srat_info[1].node_type    = SRAT_NODE_GICC_AFF,
    .srat_info[2].node_type    = SRAT_NODE_GICC_AFF,
    .srat_info[3].node_type    = SRAT_NODE_GICC_AFF,
    .srat_info[4].node_type    = SRAT_NODE_GICC_AFF,
    .srat_info[5].node_type    = SRAT_NODE_GICC_AFF,
    .srat_info[6].node_type    = SRAT_NODE_GICC_AFF,
    .srat_info[7].node_type    = SRAT_NODE_GICC_AFF,
    .srat_info[8].node_type    = SRAT_NODE_GICC_AFF,
    .srat_info[9].node_type    = SRAT_NODE_GICC_AFF,
    .srat_info[10].node_type   = SRAT_NODE_GICC_AFF,
    .srat_info[11].node_type   = SRAT_NODE_GICC_AFF,
    .srat_info[12].node_type   = SRAT_NODE_GICC_AFF,
    .srat_info[13].node_type   = SRAT_NODE_GICC_AFF,
    .srat_info[14].node_type   = SRAT_NODE_GICC_AFF,
    .srat_info[15].node_type   = SRAT_NODE_GICC_AFF,
    .srat_info[16].node_type   = SRAT_NODE_GICC_AFF,
};

PLATFORM_OVERRIDE_SRAT_NODE_INFO_TABLE platform_srat_node_type = {

    .mem_aff[0].prox_domain     = PLATFORM_SRAT_MEM0_PROX_DOMAIN,
    .mem_aff[0].flags           = PLATFORM_SRAT_MEM0_FLAGS,
    .mem_aff[0].addr_base       = PLATFORM_SRAT_MEM0_ADDR_BASE,
    .mem_aff[0].addr_len        = PLATFORM_SRAT_MEM0_ADDR_LEN,

    .gicc_aff[0].prox_domain    = PLATFORM_SRAT_GICC0_PROX_DOMAIN,
    .gicc_aff[0].proc_uid       = PLATFORM_SRAT_GICC0_PROC_UID,
    .gicc_aff[0].flags          = PLATFORM_SRAT_GICC0_FLAGS,
    .gicc_aff[0].clk_domain     = PLATFORM_SRAT_GICC0_CLK_DOMAIN,

    .gicc_aff[1].prox_domain    = PLATFORM_SRAT_GICC1_PROX_DOMAIN,
    .gicc_aff[1].proc_uid       = PLATFORM_SRAT_GICC1_PROC_UID,
    .gicc_aff[1].flags          = PLATFORM_SRAT_GICC1_FLAGS,
    .gicc_aff[1].clk_domain     = PLATFORM_SRAT_GICC1_CLK_DOMAIN,

    .gicc_aff[2].prox_domain    = PLATFORM_SRAT_GICC2_PROX_DOMAIN,
    .gicc_aff[2].proc_uid       = PLATFORM_SRAT_GICC2_PROC_UID,
    .gicc_aff[2].flags          = PLATFORM_SRAT_GICC2_FLAGS,
    .gicc_aff[2].clk_domain     = PLATFORM_SRAT_GICC2_CLK_DOMAIN,

    .gicc_aff[3].prox_domain    = PLATFORM_SRAT_GICC3_PROX_DOMAIN,
    .gicc_aff[3].proc_uid       = PLATFORM_SRAT_GICC3_PROC_UID,
    .gicc_aff[3].flags          = PLATFORM_SRAT_GICC3_FLAGS,
    .gicc_aff[3].clk_domain     = PLATFORM_SRAT_GICC3_CLK_DOMAIN,

    .gicc_aff[4].prox_domain    = PLATFORM_SRAT_GICC4_PROX_DOMAIN,
    .gicc_aff[4].proc_uid       = PLATFORM_SRAT_GICC4_PROC_UID,
    .gicc_aff[4].flags          = PLATFORM_SRAT_GICC4_FLAGS,
    .gicc_aff[4].clk_domain     = PLATFORM_SRAT_GICC4_CLK_DOMAIN,

    .gicc_aff[5].prox_domain    = PLATFORM_SRAT_GICC5_PROX_DOMAIN,
    .gicc_aff[5].proc_uid       = PLATFORM_SRAT_GICC5_PROC_UID,
    .gicc_aff[5].flags          = PLATFORM_SRAT_GICC5_FLAGS,
    .gicc_aff[5].clk_domain     = PLATFORM_SRAT_GICC5_CLK_DOMAIN,

    .gicc_aff[6].prox_domain    = PLATFORM_SRAT_GICC6_PROX_DOMAIN,
    .gicc_aff[6].proc_uid       = PLATFORM_SRAT_GICC6_PROC_UID,
    .gicc_aff[6].flags          = PLATFORM_SRAT_GICC6_FLAGS,
    .gicc_aff[6].clk_domain     = PLATFORM_SRAT_GICC6_CLK_DOMAIN,

    .gicc_aff[7].prox_domain    = PLATFORM_SRAT_GICC7_PROX_DOMAIN,
    .gicc_aff[7].proc_uid       = PLATFORM_SRAT_GICC7_PROC_UID,
    .gicc_aff[7].flags          = PLATFORM_SRAT_GICC7_FLAGS,
    .gicc_aff[7].clk_domain     = PLATFORM_SRAT_GICC7_CLK_DOMAIN,

    .gicc_aff[8].prox_domain    = PLATFORM_SRAT_GICC8_PROX_DOMAIN,
    .gicc_aff[8].proc_uid       = PLATFORM_SRAT_GICC8_PROC_UID,
    .gicc_aff[8].flags          = PLATFORM_SRAT_GICC8_FLAGS,
    .gicc_aff[8].clk_domain     = PLATFORM_SRAT_GICC8_CLK_DOMAIN,

    .gicc_aff[9].prox_domain    = PLATFORM_SRAT_GICC9_PROX_DOMAIN,
    .gicc_aff[9].proc_uid       = PLATFORM_SRAT_GICC9_PROC_UID,
    .gicc_aff[9].flags          = PLATFORM_SRAT_GICC9_FLAGS,
    .gicc_aff[9].clk_domain     = PLATFORM_SRAT_GICC9_CLK_DOMAIN,

    .gicc_aff[10].prox_domain   = PLATFORM_SRAT_GICC10_PROX_DOMAIN,
    .gicc_aff[10].proc_uid      = PLATFORM_SRAT_GICC10_PROC_UID,
    .gicc_aff[10].flags         = PLATFORM_SRAT_GICC10_FLAGS,
    .gicc_aff[10].clk_domain    = PLATFORM_SRAT_GICC10_CLK_DOMAIN,

    .gicc_aff[11].prox_domain   = PLATFORM_SRAT_GICC11_PROX_DOMAIN,
    .gicc_aff[11].proc_uid      = PLATFORM_SRAT_GICC11_PROC_UID,
    .gicc_aff[11].flags         = PLATFORM_SRAT_GICC11_FLAGS,
    .gicc_aff[11].clk_domain    = PLATFORM_SRAT_GICC11_CLK_DOMAIN,

    .gicc_aff[12].prox_domain   = PLATFORM_SRAT_GICC12_PROX_DOMAIN,
    .gicc_aff[12].proc_uid      = PLATFORM_SRAT_GICC12_PROC_UID,
    .gicc_aff[12].flags         = PLATFORM_SRAT_GICC12_FLAGS,
    .gicc_aff[12].clk_domain    = PLATFORM_SRAT_GICC12_CLK_DOMAIN,

    .gicc_aff[13].prox_domain   = PLATFORM_SRAT_GICC13_PROX_DOMAIN,
    .gicc_aff[13].proc_uid      = PLATFORM_SRAT_GICC13_PROC_UID,
    .gicc_aff[13].flags         = PLATFORM_SRAT_GICC13_FLAGS,
    .gicc_aff[13].clk_domain    = PLATFORM_SRAT_GICC13_CLK_DOMAIN,

    .gicc_aff[14].prox_domain   = PLATFORM_SRAT_GICC14_PROX_DOMAIN,
    .gicc_aff[14].proc_uid      = PLATFORM_SRAT_GICC14_PROC_UID,
    .gicc_aff[14].flags         = PLATFORM_SRAT_GICC14_FLAGS,
    .gicc_aff[14].clk_domain    = PLATFORM_SRAT_GICC14_CLK_DOMAIN,

    .gicc_aff[15].prox_domain   = PLATFORM_SRAT_GICC15_PROX_DOMAIN,
    .gicc_aff[15].proc_uid      = PLATFORM_SRAT_GICC15_PROC_UID,
    .gicc_aff[15].flags         = PLATFORM_SRAT_GICC15_FLAGS,
    .gicc_aff[15].clk_domain    = PLATFORM_SRAT_GICC15_CLK_DOMAIN,

};

PLATFORM_OVERRIDE_HMAT_INFO_TABLE platform_hmat_cfg = {

    .num_of_prox_domain = PLATFORM_OVERRIDE_NUM_OF_HMAT_PROX_DOMAIN,

    .bw_info[0].type            = HMAT_NODE_MEM_SLLBIC,
    .bw_info[0].data_type       = HMAT_NODE_MEM_SLLBIC_DATA_TYPE,
    .bw_info[0].flags           = HMAT_NODE_MEM_SLLBIC_FLAGS,
    .bw_info[0].entry_base_unit = HMAT_NODE_MEM_SLLBIC_ENTRY_BASE_UNIT,

};


PLATFORM_OVERRIDE_HMAT_MEM_TABLE platform_hmat_mem_cfg = {

    .bw_mem_info[0].mem_prox_domain  = PLATFORM_HMAT_MEM0_PROX_DOMAIN,
    .bw_mem_info[0].max_write_bw     = PLATFORM_HMAT_MEM0_MAX_WRITE_BW,
    .bw_mem_info[0].max_read_bw      = PLATFORM_HMAT_MEM0_MAX_READ_BW,

    .bw_mem_info[1].mem_prox_domain  = PLATFORM_HMAT_MEM1_PROX_DOMAIN,
    .bw_mem_info[1].max_write_bw     = PLATFORM_HMAT_MEM1_MAX_WRITE_BW,
    .bw_mem_info[1].max_read_bw      = PLATFORM_HMAT_MEM1_MAX_READ_BW,

    .bw_mem_info[2].mem_prox_domain  = PLATFORM_HMAT_MEM2_PROX_DOMAIN,
    .bw_mem_info[2].max_write_bw     = PLATFORM_HMAT_MEM2_MAX_WRITE_BW,
    .bw_mem_info[2].max_read_bw      = PLATFORM_HMAT_MEM2_MAX_READ_BW,

    .bw_mem_info[3].mem_prox_domain  = PLATFORM_HMAT_MEM3_PROX_DOMAIN,
    .bw_mem_info[3].max_write_bw     = PLATFORM_HMAT_MEM3_MAX_WRITE_BW,
    .bw_mem_info[3].max_read_bw      = PLATFORM_HMAT_MEM3_MAX_READ_BW,

};

PLATFORM_OVERRIDE_PMU_INFO_TABLE platform_pmu_cfg = {

    .pmu_count = PLATFORM_OVERRIDE_PMU_NODE_CNT,

    .pmu_info[0].base0               = PLATFORM_PMU_NODE0_BASE0,
    .pmu_info[0].base1               = PLATFORM_PMU_NODE0_BASE1,
    .pmu_info[0].type                = PLATFORM_PMU_NODE0_TYPE,
    .pmu_info[0].primary_instance    = PLATFORM_PMU_NODE0_PRI_INSTANCE,
    .pmu_info[0].secondary_instance  = PLATFORM_PMU_NODE0_SEC_INSTANCE,
    .pmu_info[0].dual_page_extension = PLATFORM_PMU_NODE0_DUAL_PAGE_EXT,

};

RAS_INFO_TABLE platform_ras_cfg = {

    .num_nodes      = PLATFORM_OVERRIDE_NUM_RAS_NODES,
    .num_pe_node    = PLATFORM_OVERRIDE_NUM_PE_RAS_NODES,
    .num_mc_node    = PLATFORM_OVERRIDE_NUM_MC_RAS_NODES,

    .node[0].type               = NODE_TYPE_PE,
    .node[0].length             = 140,
    .node[0].num_intr_entries   = 1,

    /* Example : Memory Controller RAS Node to be filled */
    //.node[1].type         = NODE_TYPE_MC,
    //.node[1].length            = 0,
    //.node[1].num_intr_entries  = 2,

};

PLATFORM_OVERRIDE_RAS_NODE_DATA_INFO platform_ras_node_data = {

    .node_data[0].pe.processor_id      = PLATFORM_RAS_NODE0_PE_PROCESSOR_ID,
    .node_data[0].pe.resource_type     = PLATFORM_RAS_NODE0_PE_RES_TYPE,
    .node_data[0].pe.flags             = PLATFORM_RAS_NODE0_PE_FLAGS,
    .node_data[0].pe.affinity          = PLATFORM_RAS_NODE0_PE_AFF,
    .node_data[0].pe.res_specific_data = PLATFORM_RAS_NODE0_PE_RES_DATA,

    /* Example : Memory Controller RAS Node data to be filled */
    //.node_data[1].mc.proximity_domain = PLATFORM_RAS_NODE0_MC_PROX_DOMAIN,
};

PLATFORM_OVERRIDE_RAS_NODE_INTERFACE_INFO platform_ras_node_interface = {

    .intf_info[0].intf_type            = PLATFORM_RAS_NODE0_INTF_TYPE,
    .intf_info[0].flags                = PLATFORM_RAS_NODE0_INTF_FLAGS,
    .intf_info[0].base_addr            = PLATFORM_RAS_NODE0_INTF_BASE,
    .intf_info[0].start_rec_index      = PLATFORM_RAS_NODE0_INTF_START_REC,
    .intf_info[0].num_err_rec          = PLATFORM_RAS_NODE0_INTF_NUM_REC,
    .intf_info[0].err_rec_implement    = PLATFORM_RAS_NODE0_INTF_ERR_REC_IMP,
    .intf_info[0].err_status_reporting = PLATFORM_RAS_NODE0_INTF_ERR_STATUS,
    .intf_info[0].addressing_mode      = PLATFORM_RAS_NODE0_INTF_ADDR_MODE,

    /* Example : RAS Node interface info to be filled */
    /*
    .intf_info[1].intf_type            = PLATFORM_RAS_NODE1_INTF_TYPE,
    .intf_info[1].flags                = PLATFORM_RAS_NODE1_INTF_FLAGS,
    .intf_info[1].base_addr            = PLATFORM_RAS_NODE1_INTF_BASE,
    .intf_info[1].start_rec_index      = PLATFORM_RAS_NODE1_INTF_START_REC,
    .intf_info[1].num_err_rec          = PLATFORM_RAS_NODE1_INTF_NUM_REC,
    .intf_info[1].err_rec_implement    = PLATFORM_RAS_NODE1_INTF_ERR_REC_IMP,
    .intf_info[1].err_status_reporting = PLATFORM_RAS_NODE1_INTF_ERR_STATUS,
    .intf_info[1].addressing_mode      = PLATFORM_RAS_NODE1_INTF_ADDR_MODE,
    */
};

PLATFORM_OVERRIDE_RAS_NODE_INTERRUPT_INFO platform_ras_node_interrupt = {

    .intr_info[0][0].type          = PLATFORM_RAS_NODE0_INTR0_TYPE,
    .intr_info[0][0].flag          = PLATFORM_RAS_NODE0_INTR0_FLAG,
    .intr_info[0][0].gsiv          = PLATFORM_RAS_NODE0_INTR0_GSIV,
    .intr_info[0][0].its_grp_id    = PLATFORM_RAS_NODE0_INTR0_ITS_ID,

    /* Example : RAS Node 0 Interrupt 1 details needs to be filled */
    /*
    .intr_info[0][1].type          = PLATFORM_RAS_NODE0_INTR1_TYPE,
    .intr_info[0][1].flag          = PLATFORM_RAS_NODE0_INTR1_FLAG,
    .intr_info[0][1].gsiv          = PLATFORM_RAS_NODE0_INTR1_GSIV,
    .intr_info[0][1].its_grp_id    = PLATFORM_RAS_NODE0_INTR1_ITS_ID,
    */

    /* Example : RAS Node 1 Interrupt 0 info to be filled */
    /*
    .intr_info[1][0].type          = PLATFORM_RAS_NODE1_INTR0_TYPE,
    .intr_info[1][0].flag          = PLATFORM_RAS_NODE1_INTR0_FLAG,
    .intr_info[1][0].gsiv          = PLATFORM_RAS_NODE1_INTR0_GSIV,
    .intr_info[1][0].its_grp_id    = PLATFORM_RAS_NODE1_INTR0_ITS_ID,
    */
};

PLATFORM_OVERRIDE_RAS2_INFO_TABLE platform_ras2_cfg = {

    .num_all_block      = PLATFORM_OVERRIDE_NUM_RAS2_BLOCK,
    .num_of_mem_block   = PLATFORM_OVERRIDE_NUM_RAS2_MEM_BLOCK,

    .blocks[0].type                 = RAS2_TYPE_MEMORY,
    .blocks[0].proximity_domain     = PLATFORM_OVERRIDE_RAS2_BLOCK0_PROXIMITY,
    .blocks[0].patrol_scrub_support = PLATFORM_OVERRIDE_RAS2_BLOCK0_PATROL_SCRUB_SUPPORT,

    .blocks[1].type                 = RAS2_TYPE_MEMORY,
    .blocks[1].proximity_domain     = PLATFORM_OVERRIDE_RAS2_BLOCK1_PROXIMITY,
    .blocks[1].patrol_scrub_support = PLATFORM_OVERRIDE_RAS2_BLOCK1_PATROL_SCRUB_SUPPORT,

    .blocks[2].type                 = RAS2_TYPE_MEMORY,
    .blocks[2].proximity_domain     = PLATFORM_OVERRIDE_RAS2_BLOCK2_PROXIMITY,
    .blocks[2].patrol_scrub_support = PLATFORM_OVERRIDE_RAS2_BLOCK2_PATROL_SCRUB_SUPPORT,

    /* Example : RAS2 Blocks to be filled */
    /*
    .blocks[1].type                 = RAS2_TYPE_MEMORY,
    .blocks[1].proximity_domain     = PLATFORM_OVERRIDE_RAS2_BLOCK1_PROXIMITY,
    .blocks[1].patrol_scrub_support = PLATFORM_OVERRIDE_RAS2_BLOCK1_PATROL_SCRUB_SUPPORT,
    */

};

PLATFORM_OVERRIDE_MPAM_INFO_TABLE platform_mpam_cfg = {
    .msc_count = PLATFORM_MPAM_MSC_COUNT,

    .msc_node[0].msc_base_addr = PLATFORM_MPAM_MSC0_BASE_ADDR,
    .msc_node[0].msc_addr_len  = PLATFORM_MPAM_MSC0_ADDR_LEN,
    .msc_node[0].max_nrdy      = PLATFORM_MPAM_MSC0_MAX_NRDY,
    .msc_node[0].rsrc_count    = PLATFORM_MPAM_MSC0_RSRC_COUNT,

    .msc_node[0].rsrc_node[0].ris_index     = PLATFORM_MPAM_MSC0_RSRC0_RIS_INDEX,
    .msc_node[0].rsrc_node[0].locator_type  = PLATFORM_MPAM_MSC0_RSRC0_LOCATOR_TYPE,
    .msc_node[0].rsrc_node[0].descriptor1   = PLATFORM_MPAM_MSC0_RSRC0_DESCRIPTOR1,
    .msc_node[0].rsrc_node[0].descriptor2   = PLATFORM_MPAM_MSC0_RSRC0_DESCRIPTOR2,

};
