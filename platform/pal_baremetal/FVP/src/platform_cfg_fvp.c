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
    .block[0].end_bus_num    = PLATFORM_OVERRIDE_PCIE_END_BUS_NUM_0

/** Configure more PCIe info details as per specification for more than 1 ECAM
    Refer to platform_override_fvp.h file for an example
**/
};

PLATFORM_PCIE_PERIPHERAL_INFO_TABLE platform_pcie_peripheral_cfg = {
    .info[0].dma_support   = PERIPHERAL0_DMA_SUPPORT,
    .info[0].dma_coherent  = PERIPHERAL0_DMA_COHERENT,
    .info[0].p2p_support   = PERIPHERAL0_P2P_SUPPORT,
    .info[0].dma_64bit     = PERIPHERAL0_DMA_64BIT,
    .info[0].behind_smmu   = PERIPHERAL0_BEHIND_SMMU,
    .info[1].dma_support   = PERIPHERAL1_DMA_SUPPORT,
    .info[1].dma_coherent  = PERIPHERAL1_DMA_COHERENT,
    .info[1].p2p_support   = PERIPHERAL1_P2P_SUPPORT,
    .info[1].dma_64bit     = PERIPHERAL1_DMA_64BIT,
    .info[1].behind_smmu   = PERIPHERAL1_BEHIND_SMMU

};

PLATFORM_OVERRIDE_IOVIRT_INFO_TABLE platform_iovirt_cfg = {
    .Address              = IOVIRT_ADDRESS,
    .node_count           = NODE_COUNT,
    .type[0]              = IOVIRT_NODE_ITS_GROUP,
    .type[1]              = IOVIRT_NODE_PCI_ROOT_COMPLEX,
    .num_map[1]           = IOVIRT_RC_NUM_MAP,
    .map[1].input_base[0] = RC_MAP0_INPUT_BASE,
    .map[1].id_count[0]   = RC_MAP0_ID_COUNT,
    .map[1].output_base[0]= RC_MAP0_OUTPUT_BASE,
    .map[1].output_ref[0] = RC_MAP0_OUTPUT_REF

};

PLATFORM_OVERRIDE_NODE_DATA platform_node_type = {
    .its_count                     = IOVIRT_ITS_COUNT,
    .smmu.base                     = IOVIRT_SMMUV3_BASE_ADDRESS,
    .smmu.context_interrupt_offset = IOVIRT_SMMU_CTX_INT_OFFSET,
    .smmu.context_interrupt_count  = IOVIRT_SMMU_CTX_INT_CNT,
    .rc.segment                    = IOVIRT_RC_PCI_SEG_NUM,
    .rc.cca                        = IOVIRT_RC_MEMORY_PROPERTIES,
    .rc.ats_attr                   = IOVIRT_RC_ATS_ATTRIBUTE

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

PCIE_READ_TABLE platform_pcie_device_hierarchy = {
    .num_entries             = PLATFORM_PCIE_NUM_ENTRIES,

    .device[0].class_code    = PLATFORM_PCIE_DEV0_CLASSCODE,
    .device[0].vendor_id     = PLATFORM_PCIE_DEV0_VENDOR_ID,
    .device[0].device_id     = PLATFORM_PCIE_DEV0_DEV_ID,
    .device[0].bus           = PLATFORM_PCIE_DEV0_BUS_NUM,
    .device[0].dev           = PLATFORM_PCIE_DEV0_DEV_NUM,
    .device[0].function      = PLATFORM_PCIE_DEV0_FUNC_NUM,
    .device[0].seg           = PLATFORM_PCIE_DEV0_SEG_NUM,

    .device[1].class_code    = PLATFORM_PCIE_DEV1_CLASSCODE,
    .device[1].vendor_id     = PLATFORM_PCIE_DEV1_VENDOR_ID,
    .device[1].device_id     = PLATFORM_PCIE_DEV1_DEV_ID,
    .device[1].bus           = PLATFORM_PCIE_DEV1_BUS_NUM,
    .device[1].dev           = PLATFORM_PCIE_DEV1_DEV_NUM,
    .device[1].function      = PLATFORM_PCIE_DEV1_FUNC_NUM,
    .device[1].seg           = PLATFORM_PCIE_DEV1_SEG_NUM,

    .device[2].class_code    = PLATFORM_PCIE_DEV2_CLASSCODE,
    .device[2].vendor_id     = PLATFORM_PCIE_DEV2_VENDOR_ID,
    .device[2].device_id     = PLATFORM_PCIE_DEV2_DEV_ID,
    .device[2].bus           = PLATFORM_PCIE_DEV2_BUS_NUM,
    .device[2].dev           = PLATFORM_PCIE_DEV2_DEV_NUM,
    .device[2].function      = PLATFORM_PCIE_DEV2_FUNC_NUM,
    .device[2].seg           = PLATFORM_PCIE_DEV2_SEG_NUM,

    .device[3].class_code    = PLATFORM_PCIE_DEV3_CLASSCODE,
    .device[3].vendor_id     = PLATFORM_PCIE_DEV3_VENDOR_ID,
    .device[3].device_id     = PLATFORM_PCIE_DEV3_DEV_ID,
    .device[3].bus           = PLATFORM_PCIE_DEV3_BUS_NUM,
    .device[3].dev           = PLATFORM_PCIE_DEV3_DEV_NUM,
    .device[3].function      = PLATFORM_PCIE_DEV3_FUNC_NUM,
    .device[3].seg           = PLATFORM_PCIE_DEV3_SEG_NUM,

    .device[4].class_code    = PLATFORM_PCIE_DEV4_CLASSCODE,
    .device[4].vendor_id     = PLATFORM_PCIE_DEV4_VENDOR_ID,
    .device[4].device_id     = PLATFORM_PCIE_DEV4_DEV_ID,
    .device[4].bus           = PLATFORM_PCIE_DEV4_BUS_NUM,
    .device[4].dev           = PLATFORM_PCIE_DEV4_DEV_NUM,
    .device[4].function      = PLATFORM_PCIE_DEV4_FUNC_NUM,
    .device[4].seg           = PLATFORM_PCIE_DEV4_SEG_NUM,

    .device[5].class_code    = PLATFORM_PCIE_DEV5_CLASSCODE,
    .device[5].vendor_id     = PLATFORM_PCIE_DEV5_VENDOR_ID,
    .device[5].device_id     = PLATFORM_PCIE_DEV5_DEV_ID,
    .device[5].bus           = PLATFORM_PCIE_DEV5_BUS_NUM,
    .device[5].dev           = PLATFORM_PCIE_DEV5_DEV_NUM,
    .device[5].function      = PLATFORM_PCIE_DEV5_FUNC_NUM,
    .device[5].seg           = PLATFORM_PCIE_DEV5_SEG_NUM,

    .device[6].class_code    = PLATFORM_PCIE_DEV6_CLASSCODE,
    .device[6].vendor_id     = PLATFORM_PCIE_DEV6_VENDOR_ID,
    .device[6].device_id     = PLATFORM_PCIE_DEV6_DEV_ID,
    .device[6].bus           = PLATFORM_PCIE_DEV6_BUS_NUM,
    .device[6].dev           = PLATFORM_PCIE_DEV6_DEV_NUM,
    .device[6].function      = PLATFORM_PCIE_DEV6_FUNC_NUM,
    .device[6].seg           = PLATFORM_PCIE_DEV6_SEG_NUM,

    .device[7].class_code    = PLATFORM_PCIE_DEV7_CLASSCODE,
    .device[7].vendor_id     = PLATFORM_PCIE_DEV7_VENDOR_ID,
    .device[7].device_id     = PLATFORM_PCIE_DEV7_DEV_ID,
    .device[7].bus           = PLATFORM_PCIE_DEV7_BUS_NUM,
    .device[7].dev           = PLATFORM_PCIE_DEV7_DEV_NUM,
    .device[7].function      = PLATFORM_PCIE_DEV7_FUNC_NUM,
    .device[7].seg           = PLATFORM_PCIE_DEV7_SEG_NUM,

    .device[8].class_code    = PLATFORM_PCIE_DEV8_CLASSCODE,
    .device[8].vendor_id     = PLATFORM_PCIE_DEV8_VENDOR_ID,
    .device[8].device_id     = PLATFORM_PCIE_DEV8_DEV_ID,
    .device[8].bus           = PLATFORM_PCIE_DEV8_BUS_NUM,
    .device[8].dev           = PLATFORM_PCIE_DEV8_DEV_NUM,
    .device[8].function      = PLATFORM_PCIE_DEV8_FUNC_NUM,
    .device[8].seg           = PLATFORM_PCIE_DEV8_SEG_NUM,

    .device[9].class_code    = PLATFORM_PCIE_DEV9_CLASSCODE,
    .device[9].vendor_id     = PLATFORM_PCIE_DEV9_VENDOR_ID,
    .device[9].device_id     = PLATFORM_PCIE_DEV9_DEV_ID,
    .device[9].bus           = PLATFORM_PCIE_DEV9_BUS_NUM,
    .device[9].dev           = PLATFORM_PCIE_DEV9_DEV_NUM,
    .device[9].function      = PLATFORM_PCIE_DEV9_FUNC_NUM,
    .device[9].seg           = PLATFORM_PCIE_DEV9_SEG_NUM,

    .device[10].class_code    = PLATFORM_PCIE_DEV10_CLASSCODE,
    .device[10].vendor_id     = PLATFORM_PCIE_DEV10_VENDOR_ID,
    .device[10].device_id     = PLATFORM_PCIE_DEV10_DEV_ID,
    .device[10].bus           = PLATFORM_PCIE_DEV10_BUS_NUM,
    .device[10].dev           = PLATFORM_PCIE_DEV10_DEV_NUM,
    .device[10].function      = PLATFORM_PCIE_DEV10_FUNC_NUM,
    .device[10].seg           = PLATFORM_PCIE_DEV10_SEG_NUM,

    .device[11].class_code    = PLATFORM_PCIE_DEV11_CLASSCODE,
    .device[11].vendor_id     = PLATFORM_PCIE_DEV11_VENDOR_ID,
    .device[11].device_id     = PLATFORM_PCIE_DEV11_DEV_ID,
    .device[11].bus           = PLATFORM_PCIE_DEV11_BUS_NUM,
    .device[11].dev           = PLATFORM_PCIE_DEV11_DEV_NUM,
    .device[11].function      = PLATFORM_PCIE_DEV11_FUNC_NUM,
    .device[11].seg           = PLATFORM_PCIE_DEV11_SEG_NUM,

    .device[12].class_code    = PLATFORM_PCIE_DEV12_CLASSCODE,
    .device[12].vendor_id     = PLATFORM_PCIE_DEV12_VENDOR_ID,
    .device[12].device_id     = PLATFORM_PCIE_DEV12_DEV_ID,
    .device[12].bus           = PLATFORM_PCIE_DEV12_BUS_NUM,
    .device[12].dev           = PLATFORM_PCIE_DEV12_DEV_NUM,
    .device[12].function      = PLATFORM_PCIE_DEV12_FUNC_NUM,
    .device[12].seg           = PLATFORM_PCIE_DEV12_SEG_NUM,

    .device[13].class_code    = PLATFORM_PCIE_DEV13_CLASSCODE,
    .device[13].vendor_id     = PLATFORM_PCIE_DEV13_VENDOR_ID,
    .device[13].device_id     = PLATFORM_PCIE_DEV13_DEV_ID,
    .device[13].bus           = PLATFORM_PCIE_DEV13_BUS_NUM,
    .device[13].dev           = PLATFORM_PCIE_DEV13_DEV_NUM,
    .device[13].function      = PLATFORM_PCIE_DEV13_FUNC_NUM,
    .device[13].seg           = PLATFORM_PCIE_DEV13_SEG_NUM,

    .device[14].class_code    = PLATFORM_PCIE_DEV14_CLASSCODE,
    .device[14].vendor_id     = PLATFORM_PCIE_DEV14_VENDOR_ID,
    .device[14].device_id     = PLATFORM_PCIE_DEV14_DEV_ID,
    .device[14].bus           = PLATFORM_PCIE_DEV14_BUS_NUM,
    .device[14].dev           = PLATFORM_PCIE_DEV14_DEV_NUM,
    .device[14].function      = PLATFORM_PCIE_DEV14_FUNC_NUM,
    .device[14].seg           = PLATFORM_PCIE_DEV14_SEG_NUM,

    .device[15].class_code    = PLATFORM_PCIE_DEV15_CLASSCODE,
    .device[15].vendor_id     = PLATFORM_PCIE_DEV15_VENDOR_ID,
    .device[15].device_id     = PLATFORM_PCIE_DEV15_DEV_ID,
    .device[15].bus           = PLATFORM_PCIE_DEV15_BUS_NUM,
    .device[15].dev           = PLATFORM_PCIE_DEV15_DEV_NUM,
    .device[15].function      = PLATFORM_PCIE_DEV15_FUNC_NUM,
    .device[15].seg           = PLATFORM_PCIE_DEV15_SEG_NUM,

    .device[16].class_code    = PLATFORM_PCIE_DEV16_CLASSCODE,
    .device[16].vendor_id     = PLATFORM_PCIE_DEV16_VENDOR_ID,
    .device[16].device_id     = PLATFORM_PCIE_DEV16_DEV_ID,
    .device[16].bus           = PLATFORM_PCIE_DEV16_BUS_NUM,
    .device[16].dev           = PLATFORM_PCIE_DEV16_DEV_NUM,
    .device[16].function      = PLATFORM_PCIE_DEV16_FUNC_NUM,
    .device[16].seg           = PLATFORM_PCIE_DEV16_SEG_NUM,

    .device[17].class_code    = PLATFORM_PCIE_DEV17_CLASSCODE,
    .device[17].vendor_id     = PLATFORM_PCIE_DEV17_VENDOR_ID,
    .device[17].device_id     = PLATFORM_PCIE_DEV17_DEV_ID,
    .device[17].bus           = PLATFORM_PCIE_DEV17_BUS_NUM,
    .device[17].dev           = PLATFORM_PCIE_DEV17_DEV_NUM,
    .device[17].function      = PLATFORM_PCIE_DEV17_FUNC_NUM,
    .device[17].seg           = PLATFORM_PCIE_DEV17_SEG_NUM,

    .device[18].class_code    = PLATFORM_PCIE_DEV18_CLASSCODE,
    .device[18].vendor_id     = PLATFORM_PCIE_DEV18_VENDOR_ID,
    .device[18].device_id     = PLATFORM_PCIE_DEV18_DEV_ID,
    .device[18].bus           = PLATFORM_PCIE_DEV18_BUS_NUM,
    .device[18].dev           = PLATFORM_PCIE_DEV18_DEV_NUM,
    .device[18].function      = PLATFORM_PCIE_DEV18_FUNC_NUM,
    .device[18].seg           = PLATFORM_PCIE_DEV18_SEG_NUM,

    .device[19].class_code    = PLATFORM_PCIE_DEV19_CLASSCODE,
    .device[19].vendor_id     = PLATFORM_PCIE_DEV19_VENDOR_ID,
    .device[19].device_id     = PLATFORM_PCIE_DEV19_DEV_ID,
    .device[19].bus           = PLATFORM_PCIE_DEV19_BUS_NUM,
    .device[19].dev           = PLATFORM_PCIE_DEV19_DEV_NUM,
    .device[19].function      = PLATFORM_PCIE_DEV19_FUNC_NUM,
    .device[19].seg           = PLATFORM_PCIE_DEV19_SEG_NUM,

    .device[20].class_code    = PLATFORM_PCIE_DEV20_CLASSCODE,
    .device[20].vendor_id     = PLATFORM_PCIE_DEV20_VENDOR_ID,
    .device[20].device_id     = PLATFORM_PCIE_DEV20_DEV_ID,
    .device[20].bus           = PLATFORM_PCIE_DEV20_BUS_NUM,
    .device[20].dev           = PLATFORM_PCIE_DEV20_DEV_NUM,
    .device[20].function      = PLATFORM_PCIE_DEV20_FUNC_NUM,
    .device[20].seg           = PLATFORM_PCIE_DEV20_SEG_NUM
};
