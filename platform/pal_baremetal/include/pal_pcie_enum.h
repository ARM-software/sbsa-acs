/** @file
 * Copyright (c) 2020-2021, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __PAL_PCIE_ENUM_H__
#define __PAL_PCIE_ENUM_H__

#include <stdio.h>
#include <stdint.h>


/* Header Offset and Type*/
#define HEADER_OFFSET      0xC
#define TYPE0_HEADER       0
#define TYPE1_HEADER       1

#define TYPE01_RIDR        0x8

#define PCIE_HEADER_TYPE(header_value) ((header_value >> 16) & 0x3)
#define BUS_NUM_REG_CFG(sub_bus, sec_bus, pri_bus) (sub_bus << 16 | sec_bus << 8 | bus)

#define DEVICE_ID_OFFSET   16

/*Initial BUS definitions*/
#define PRI_BUS            0
#define SEC_BUS            1
#define BUS_NUM_REG_OFFSET 0x18

/*BAR offset */
#define BAR0_OFFSET        0x10
#define BAR_MAX_OFFSET     0x24
#define BAR_64_BIT         1
#define BAR_32_BIT         0

#define BAR_REG(bar_reg_value) ((bar_reg_value >> 2) & 0x1)


uint8_t  pal_mmio_read8(uint64_t addr);
uint16_t pal_mmio_read16(uint64_t addr);
uint32_t pal_mmio_read(uint64_t addr);
uint64_t pal_mmio_read64(uint64_t addr);
void     pal_mmio_write8(uint64_t addr, uint8_t data);
void     pal_mmio_write16(uint64_t addr, uint16_t data);
void     pal_mmio_write(uint64_t addr, uint32_t data);
void     pal_mmio_write64(uint64_t addr, uint64_t data);
void     *pal_mem_alloc(uint32_t size);

uint32_t pal_increment_bus_dev(uint32_t StartBdf);

uint32_t pal_pcie_get_bdf(uint32_t class_code, uint32_t start_busdev);

uint64_t pal_pcie_get_base(uint32_t bdf, uint32_t bar_index);

uint32_t pal_pci_cfg_read(uint32_t bus, uint32_t dev, uint32_t func, uint32_t offset, uint32_t *value);

uint32_t pal_pcie_read_cfg(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t func, uint32_t offset, uint32_t *value);

#endif
