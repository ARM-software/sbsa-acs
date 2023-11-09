## @file
 # Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 # SPDX-License-Identifier : Apache-2.0
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
 ##

set(PAL_SRC
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_bm_dma.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_bm_exerciser.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_bm_gic.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_bm_iovirt.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_bm_misc.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_bm_pcie.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_bm_pe.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_bm_peripherals.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_bm_pmu.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_bm_ras.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/pal_bm_smmu.c
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/src/platform_cfg_fvp.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_dma.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_exerciser.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_gic.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_hmat.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_iovirt.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_misc.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_mpam.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_pcie_enumeration.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_pcie.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_pe.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_peripherals.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_pl011_uart.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_pmu.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_ras.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_smmu.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/pal_timer_wd.c
    ${ROOT_DIR}/platform/pal_baremetal/common/src/AArch64/AvsTestInfra.S
    ${ROOT_DIR}/platform/pal_baremetal/common/src/AArch64/ModuleEntryPoint.S
    ${ROOT_DIR}/platform/pal_baremetal/common/src/AArch64/ArmSmc.S
)

#Create compile list files
list(APPEND COMPILE_LIST ${PAL_SRC})
set(COMPILE_LIST ${COMPILE_LIST} PARENT_SCOPE)

# Create PAL library
add_library(${PAL_LIB} STATIC ${PAL_SRC})

target_include_directories(${PAL_LIB} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ROOT_DIR}/
    ${ROOT_DIR}/baremetal_app/
    ${ROOT_DIR}/platform/pal_baremetal/
    ${ROOT_DIR}/platform/pal_baremetal/common/include/
    ${ROOT_DIR}/platform/pal_baremetal/common/src/AArch64/
    ${ROOT_DIR}/platform/pal_baremetal/${TARGET}/include/
)

unset(PAL_SRC)
