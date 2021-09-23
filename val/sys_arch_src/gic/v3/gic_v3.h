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
#ifndef __GIC_V3_H__
#define __GIC_V3_H__

#define GICD_TYPER_ESPI_SHIFT       8
#define GICD_TYPER_ESPI_MASK        0x01

#define GICD_TYPER_ESPI_RANGE_SHIFT 27
#define GICD_TYPER_ESPI_RANGE_MASK  0x1F

#define GICD_TYPER_EPPI_NUM_SHIFT   27
#define GICD_TYPER_EPPI_NUM_MASK    0x1F

#define EXTENDED_SPI_START_INTID   4096
#define EXTENDED_PPI_START_INTID   1056

#define EXTENDED_PPI_REG_OFFSET    1024

void v3_Init(void);
void v3_EnableInterruptSource(uint32_t);
void v3_DisableInterruptSource(uint32_t);
uint32_t v3_AcknowledgeInterrupt(void);
void v3_EndofInterrupt(uint32_t int_id);
uint32_t v3_read_gicdTyper(void);
uint64_t v3_get_pe_gicr_base(void);
uint64_t v3_read_gicr_typer(void);

uint32_t v3_is_extended_spi(uint32_t int_id);
uint32_t v3_is_extended_ppi(uint32_t int_id);
void v3_clear_extended_spi_interrupt(uint32_t int_id);
void v3_disable_extended_interrupt_source(uint32_t int_id);
void v3_enable_extended_interrupt_source(uint32_t int_id);
void v3_set_extended_interrupt_priority(uint32_t int_id, uint32_t priority);
void v3_extended_init(void);

#endif /*__GIC_V3_H__ */
