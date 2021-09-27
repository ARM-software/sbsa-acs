/** @file
 * Copyright (c) 2021, Arm Limited or its affiliates. All rights reserved.
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
#ifndef __GIC_H__
#define __GIC_H__


#define GIC_SUCCESS 0

/* GIC specific registers */

#define GICD_CTRL           0x0000
#define GICD_TYPER          0x0004
#define GICD_IPRIORITYR     0x0400
#define GICD_IROUTERn       0x6100

#define GICR_CTLR           0x0000
#define GICR_TYPER          0x0008
#define GICR_IPRIORITYR     0x0400
#define GICR_ICENABLER      0x0180
#define GICR_WAKER          0x0014
#define GICR_PWRR           0x0024
#define NEXT_DW_OFFSET      0x4

#define GIC_ARE_ENABLE           (1 << 4)
#define GIC_DEFAULT_PRIORITY     0x80
#define GICR_CTLR_FRAME_SIZE     0x00010000
#define GICR_SGI_PPI_FRAME_SIZE  0x00010000
#define GICR_TYPER_AFF           (0xFFFFFFFFULL << 32)

#define GIC_ICDIPTR         0x800
#define GIC_ICCICR          0x00
#define GIC_ICCBPR          0x08
#define GIC_ICCPMR          0x04
#define GIC_ICCIAR          0x0C
#define GIC_ICCEIOR         0x10

#define PE_AFF0   0xFF
#define PE_AFF1   (0xFF << 8)
#define PE_AFF2   (0xFF << 16)
#define PE_AFF3   (0xFFULL << 32)


void val_sbsa_gic_init(void);
void val_sbsa_gic_disableInterruptSource(uint32_t int_id);
void val_sbsa_gic_enableInterruptSource(uint32_t int_id);
uint32_t val_sbsa_gic_acknowledgeInterrupt(void);
void val_sbsa_gic_endofInterrupt(uint32_t int_id);
uint32_t val_sbsa_gic_espi_support(void);
uint32_t val_sbsa_gic_max_espi_val(void);
uint32_t val_sbsa_gic_check_espi_interrupt(uint32_t int_id);
void val_sbsa_gic_clear_espi_interrupt(uint32_t int_id);
uint32_t val_sbsa_gic_eppi_support(void);
uint32_t val_sbsa_gic_max_eppi_val(void);
uint32_t val_sbsa_gic_check_eppi_interrupt(uint32_t int_id);

#endif /*__GIC_H__ */
