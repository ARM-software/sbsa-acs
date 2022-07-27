/** @file
 * Copyright (c) 2021-2022, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __BSA_GIC_ITS_H
#define __BSA_GIC_ITS_H


#include "include/val_interface.h"
#include "include/sbsa_avs_common.h"
#include "include/sbsa_avs_memory.h"

#define SIZE_4KB    0x00001000
#define SIZE_64KB   0x00010000

#define PAGE_MASK   0xFFF
#define PAGE_SHIFT   12

#define SIZE_TO_PAGES(Size)   (((Size) >> PAGE_SHIFT) + (((Size) & PAGE_MASK) ? 1 : 0))
#define PAGES_TO_SIZE(Pages)   ((Pages) << PAGE_SHIFT)

#define ARM_LPI_MINID       8192
#define ARM_LPI_MIN_IDBITS  14
#define ARM_LPI_MAX_IDBITS  31

#define WAIT_ITS_COMMAND_DONE   10000

/* GICv3 specific registers */

/* Distributor Interrupt Controller Type Register */
#define ARM_GICD_TYPER          0x0004

/* GICD_TYPER bits */
#define ARM_GICD_TYPER_LPIS                 (1 << 17) /* LPIS Enable */
#define ARM_GICD_TYPER_IDbits(GICD_Typer)   ((GICD_Typer >> 19) & 0x1F) /* IDBits implemented */

/* GIC Redistributor Control frame */
#define ARM_GICR_CTLR           0x0000  /* Redistributor Control Register      */
#define ARM_GICR_PROPBASER      0x0070  /* Redistributor Config base Register  */
#define ARM_GICR_PENDBASER      0x0078  /* Redistributor Pending base Register */

#define ARM_GICR_CTLR_ENABLE_LPIS   (1 << 0)

#define ARM_GICR_TYPER          0x0008  // Redistributor Type Register


// GIC Redistributor
#define ARM_GICR_CTLR_FRAME_SIZE    SIZE_64KB
#define ARM_GICR_SGI_PPI_FRAME_SIZE SIZE_64KB
#define ARM_GICR_VLPI_FRAME_SIZE    SIZE_64KB
#define ARM_GICR_RESERVED_PAGE_SIZE SIZE_64KB

/* GICR_TYPER Bits */
#define NEXT_DW_OFFSET          0x4         /* Used to read Upper 4 Bytes of GICR_TYPER */
#define ARM_GICR_TYPER_PLPIS    (1 << 0)
#define ARM_GICR_TYPER_VLPIS    (1 << 1)
#define ARM_GICR_TYPER_PN_MASK  (0xFFFF00)
#define ARM_GICR_TYPER_PN_SHIFT 8

/* GICR_PROPBASER Bits */
#define ARM_GICR_PROPBASER_IDbits(Propbaser) (Propbaser & 0x1F) /* IDBits implemented */
#define PROPBASER_PA_SHIFT                   12
#define PROPBASER_PA_LEN                     40
#define ARM_GICR_PROPBASER_PA_MASK           (((1ul << PROPBASER_PA_LEN) - 1) << PROPBASER_PA_SHIFT)

#define PENDBASER_PA_SHIFT                   16
#define PENDBASER_PA_LEN                     36
#define ARM_GICR_PENDBASER_PA_MASK           (((1ul << PENDBASER_PA_LEN) - 1) << PENDBASER_PA_SHIFT)

/* GIC ITS Register Offset from ITS_CTRL_BASE */
#define ARM_GITS_CTLR               0x0000
#define ARM_GITS_IIDR               0x0004
#define ARM_GITS_TYPER              0x0008
#define ARM_GITS_CBASER             0x0080
#define ARM_GITS_CWRITER            0x0088
#define ARM_GITS_CREADR             0x0090
#define ARM_GITS_BASER(n)           (0x0100 + 8*n)

#define ARM_GITS_TRANSLATER         0x10040

/* GITS_CTLR Register Bits */
#define ARM_GITS_CTLR_ENABLE        (1 << 0)

/* GITS_BASER Register Bits */
#define ARM_NUM_GITS_BASER                          8
#define ARM_GITS_BASER_INDIRECT                     (1ul << 62)
#define ARM_GITS_BASER_GET_TYPE(gits_baser)         ((gits_baser >> 56) & 0x7)
#define ARM_GITS_BASER_GET_ENTRY_SIZE(gits_baser)   ((gits_baser >> 48) & 0x1F)
#define BASER_PA_SHIFT                              12
#define BASER_PA_LEN                                36
#define ARM_GITS_BASER_PA_MASK                      (((1ul << BASER_PA_LEN) - 1) << BASER_PA_SHIFT)
#define ARM_GITS_BASER_VALID                        (1ul << 63)

#define ARM_GITS_TBL_TYPE_DEVICE    0x1
#define ARM_GITS_TBL_TYPE_CLCN      0x4

/* GITS_TYPER Bits */
#define ARM_GITS_TYPER_DevBits(its_typer)           ((its_typer >> 13) & 0x1F)
#define ARM_GITS_TYPER_CIDBits(its_typer)           ((its_typer >> 32) & 0xF)
#define ARM_GITS_TYPER_IDbits(its_typer)            ((its_typer >> 8) & 0x1F)
#define ARM_GITS_TYPER_PTA                          (1 << 19)

/* GITS_CREADR Bits */
#define ARM_GITS_CREADR_STALL       (1 << 0)

/* GITS_CWRITER Bits */
#define ARM_GITS_CWRITER_RETRY      (1 << 0)

/* GITS_CBASER Bits */
#define ARM_GITS_CBASER_VALID           (1ul << 63)
#define CBASER_PA_SHIFT                 12
#define CBASER_PA_LEN                   40
#define ARM_GITS_CBASER_PA_MASK         (((1ul << CBASER_PA_LEN) - 1) << CBASER_PA_SHIFT)
#define ITT_PAR_SHIFT                   8
#define ITT_PAR_LEN                     44
#define ITT_PAR_MASK                    (((1ul << ITT_PAR_LEN) - 1) << ITT_PAR_SHIFT)

//
// ARM MP Core IDs
//
#define ARM_CORE_AFF0         0xFF
#define ARM_CORE_AFF1         (0xFF << 8)
#define ARM_CORE_AFF2         (0xFF << 16)
#define ARM_CORE_AFF3         (0xFFULL << 32)

#define GET_CONFIG_TABLE_SIZE_BY_BITS(gicd_idbits, gicr_idbits)  \
      ((gicd_idbits < gicr_idbits)?((1 << (gicd_idbits+1)) - 8192):((1 << (gicr_idbits+1)) - 8192))
#define GET_MIN(a, b)    ((a < b)?a:b)
#define GET_PENDING_TABLE_SIZE_BY_BITS(gicd_idbits, gicr_idbits)  \
      ((gicd_idbits < gicr_idbits)?((1 << (gicd_idbits+1))/8):((1 << (gicr_idbits+1))/8))

#define LPI_ID1     0x203A
#define LPI_ID2     0x203B
#define LPI_ID3     0x203C
#define LPI_ID4     0x203D

#define LPI_PRIORITY1       0x50
#define LPI_PRIORITY2       0x60
#define LPI_PRIORITY3       0x70
#define LPI_PRIORITY4       0x80
#define LPI_PRIORITY_MASK   0xFC
#define LPI_ENABLE          (1 << 0)
#define LPI_DISABLE         0x0

#define ARM_ITS_CMD_MAPD    0x8
#define ARM_ITS_CMD_MAPC    0x9
#define ARM_ITS_CMD_MAPI    0xB
#define ARM_ITS_CMD_INV     0xC
#define ARM_ITS_CMD_DISCARD 0xF
#define ARM_ITS_CMD_SYNC    0x5

#define RD_BASE_SHIFT       16
#define NUM_PAGES_8         8

#define ITS_CMD_SHIFT_DEVID 32
#define ITS_CMD_SHIFT_VALID 63
#define ITS_NEXT_CMD_PTR    4
#define NUM_BYTES_IN_DW     8

uint32_t ArmGicRedistributorConfigurationForLPI(uint64_t gicd_base, uint64_t rd_base);

void ClearConfigTable(uint32_t int_id);
void SetConfigTable(uint32_t int_id, uint32_t Priority);

uint32_t val_its_gicd_lpi_support(uint64_t gicd_base);
uint32_t val_its_gicr_lpi_support(uint64_t rd_base);


void EnableLPIsRD(uint64_t rd_base);
void val_its_create_lpi_map(uint32_t its_index, uint32_t device_id,
                            uint32_t int_id, uint32_t Priority);
void val_its_clear_lpi_map(uint32_t its_index, uint32_t device_id, uint32_t int_id);

uint64_t val_its_get_translater_addr(uint32_t its_index);
uint32_t val_its_get_max_lpi(void);
uint32_t val_its_init(void);
uint64_t val_its_get_curr_rdbase(uint64_t rd_base, uint32_t length);

#endif
