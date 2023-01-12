/** @file
 * Copyright (c) 2022, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __PMU_REG_H__
#define __PMU_REG_H__

/* Macro can be used to declare MASK and SHIFT for bitfields using MSB and LSB */
#define BITFIELD_DECL(type, name, msb, lsb) \
    const uint32_t name##_SHIFT = lsb; \
    const type name##_MASK = ((((type)0x1) << (msb - lsb + 1)) - 1);

/* Macro can be used to read bitfields with name##_MASK and name##_SHIFT
   already defined or declared  for it*/
#define BITFIELD_READ(name, val) ((val >> name##_SHIFT) & name##_MASK)

/* Macro can be used to set bitfields with name##_MASK and name##_SHIFT
   already defined or declared  for it*/
#define BITFIELD_SET(name, val) ((val & name##_MASK) << name##_SHIFT)

/* Macro can be used write a particular bitfield with name##_MASK and name##_SHIFT
   already defined or declared  for it without affecting other fields*/
#define BITFIELD_WRITE(reg_val, name, val) \
        ((reg_val & ~(name##_MASK << name##_SHIFT)) | val << name##_SHIFT)

/*******************************************************************************
 * PMU memory mapped register offsets
 ******************************************************************************/
#define REG_PMEVCNTR               0x0000
#define REG_PMEVCNTR_L             0x0000
#define REG_PMEVCNTR_H             0x0004
#define REG_PMCCNTR                0x003C
#define REG_PMCCNTR_L              0x00F8
#define REG_PMCCNTR_H              0x00FC
#define REG_PMEVTYPER              0x0400
#define REG_PMCCFILTR              0x047C
#define REG_PMSVR                  0x0600
#define REG_PMEVFILTR              0x0A00
#define REG_PMCNTENSET             0x0C00
#define REG_PMCNTENCLR             0x0C20
#define REG_PMINTENSET             0x0C40
#define REG_PMINTENCLR             0x0C60
#define REG_PMOVSCLR               0x0C80
#define REG_PMOVSSET               0x0CC0
#define REG_PMCGR                  0x0CE0
#define REG_PMCFGR                 0x0E00
#define REG_PMCR                   0x0E04
#define REG_PMIIDR                 0x0E08
#define REG_PMCEID                 0x0E20
#define REG_PMSSCR                 0x0E30
#define REG_PMSSRR_L               0x0E38
#define REG_PMSSRR_H               0x0E3C
#define REG_PMSCR_L                0x0E40
#define REG_PMSCR_H                0x0E44
#define REG_PMIRQCR0_L             0x0E80
#define REG_PMIRQCR0_H             0x0E84
#define REG_PMIRQCR1               0x0E88
#define REG_PMIRQCR2               0x0E8C
#define REG_PMIRQSR_L              0x0EF8
#define REG_PMIRQSR_H              0x0EFC
#define REG_PMDEVAFF_L             0x0FA8
#define REG_PMDEVAFF_H             0x0FAC
#define REG_PMAUTHSTATUS           0x0FB8
#define REG_PMDEVARCH              0x0FBC
#define REG_PMDEVID                0x0FC8
#define REG_PMDEVTYPE              0x0FCC
#define REG_PMPIDR4                0x0FD0
#define REG_PMPIDR5                0x0FD4
#define REG_PMPIDR6                0x0FD8
#define REG_PMPIDR7                0x0FDC
#define REG_PMPIDR0                0x0FE0
#define REG_PMPIDR1                0x0FE4
#define REG_PMPIDR2                0x0FE8
#define REG_PMPIDR3                0x0FEC
#define REG_PMCIDR0                0x0FF0
#define REG_PMCIDR1                0x0FF4
#define REG_PMCIDR2                0x0FF8
#define REG_PMCIDR3                0x0FFC



/* PMCFGR bit definitions */
BITFIELD_DECL(uint32_t, PMCFGR_N, 7, 0)
BITFIELD_DECL(uint32_t, PMCFGR_SIZE, 13, 8)
BITFIELD_DECL(uint32_t, PMCFGR_CC, 14, 14)
BITFIELD_DECL(uint32_t, PMCFGR_CCD, 15, 15)
BITFIELD_DECL(uint32_t, PMCFGR_EX, 16, 16)
BITFIELD_DECL(uint32_t, PMCFGR_NA, 17, 17)
BITFIELD_DECL(uint32_t, PMCFGR_UEN, 19, 19)
BITFIELD_DECL(uint32_t, PMCFGR_MSI, 20, 20)
BITFIELD_DECL(uint32_t, PMCFGR_FZO, 21, 21)
BITFIELD_DECL(uint32_t, PMCFGR_SS, 22, 22)
BITFIELD_DECL(uint32_t, PMCFGR_TRO, 23, 23)
BITFIELD_DECL(uint32_t, PMCFGR_HDBG, 24, 24)
BITFIELD_DECL(uint32_t, PMCFGR_NCG, 31, 28)

/* PMCR bit configuration */
BITFIELD_DECL(uint32_t, PMCR_E, 0, 0)
BITFIELD_DECL(uint32_t, PMCR_P, 1, 1)
BITFIELD_DECL(uint32_t, PMCR_C, 2, 2)
BITFIELD_DECL(uint32_t, PMCR_D, 3, 3)
BITFIELD_DECL(uint32_t, PMCR_X, 4, 4)
BITFIELD_DECL(uint32_t, PMCR_DP, 5, 5)
BITFIELD_DECL(uint32_t, PMCR_NA, 8, 8)
BITFIELD_DECL(uint32_t, PMCR_FZO, 9, 9)
BITFIELD_DECL(uint32_t, PMCR_HDBG, 10, 10)
BITFIELD_DECL(uint32_t, PMCR_TRO, 11, 11)

/* PMSCR_L bit configuration */
BITFIELD_DECL(uint32_t, PMSCR_SO, 0, 0)
BITFIELD_DECL(uint32_t, PMSCR_NSRA, 1, 1)
BITFIELD_DECL(uint32_t, PMSCR_NSMSI, 2, 2)
BITFIELD_DECL(uint32_t, PMSCR_MSI_MPAM_NS, 3, 3)
BITFIELD_DECL(uint32_t, PMSCR_NAO, 4, 4)
BITFIELD_DECL(uint32_t, PMSCR_IMPL, 31, 31)

#endif /*__PMU_REG_H__ */
