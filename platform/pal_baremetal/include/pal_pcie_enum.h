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

#ifndef __PAL_PCIE_ENUM_H__
#define __PAL_PCIE_ENUM_H__

#include <stdio.h>
#include <stdint.h>


/* Header Offset and Type*/
#define HEADER_OFFSET      0xC
#define TYPE0_HEADER       0
#define TYPE1_HEADER       1


#define PCIE_HEADER_TYPE(header_value) ((header_value >> 16) & 0x1)
#define BUS_NUM_REG_CFG(sub_bus, sec_bus, pri_bus) (sub_bus << 16 | sec_bus << 8 | bus)


/*Initial BUS definitions*/
#define PRI_BUS            0
#define SEC_BUS            1
#define BUS_NUM_REG_OFFSET 0x18

/*BAR offset */
#define PCIE_BAR_VAL       0x100000
#define BAR0_OFFSET        0x10
#define BAR_MAX_OFFSET     0x24
#define BAR_64_BIT         1
#define BAR_32_BIT         0

#define BAR_REG(bar_reg_value) ((bar_reg_value >> 2) & 0x1)


uint32_t pal_mmio_read(uint64_t addr);
void     pal_mmio_write(uint64_t addr, uint32_t data);

#endif
