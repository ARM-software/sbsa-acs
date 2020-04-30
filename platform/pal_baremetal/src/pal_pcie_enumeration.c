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

#include "include/pal_pcie_enum.h"
#include "include/pal_common_support.h"

extern PCIE_INFO_TABLE *g_pcie_info_table;

uint64_t g_bar_start = PCIE_BAR_VAL;

/**
  @brief   This API reads 32-bit data from PCIe config space pointed by Bus,
           Device, Function and register offset.
  @param   bus,dev,func -  Bus(8-bits), device(8-bits) & function(8-bits)
  @param   offset - Register offset within a device PCIe config space
  @param   *data  - 32-bit data read from the config space

  @return  success/failure
**/
uint32_t pal_pci_cfg_read(uint32_t bus, uint32_t dev, uint32_t func, uint32_t offset, uint32_t *value)
{

  uint32_t cfg_addr;
  uint64_t ecam_base = g_pcie_info_table->block[0].ecam_base;

  cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * PCIE_CFG_SIZE) + (dev * PCIE_MAX_FUNC * PCIE_CFG_SIZE) + (func * PCIE_CFG_SIZE);
  *value = pal_mmio_read(ecam_base + cfg_addr + offset);
  return 0;

}

/**
  @brief   This API writes 32-bit data to PCIe config space pointed by Bus,
           Device, Function and register offset.
  @param   bus,dev,func - Bus(8-bits), device(8-bits) & function(8-bits)
  @param   offset - Register offset within a device PCIe config space
  @param   data   - data to be written to the config space

  @return  None
**/
void pal_pci_cfg_write(uint32_t bus, uint32_t dev, uint32_t func, uint32_t offset, uint32_t data)
{

  uint64_t ecam_base = g_pcie_info_table->block[0].ecam_base;
  uint32_t cfg_addr;

  cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * PCIE_CFG_SIZE) + (dev * PCIE_MAX_FUNC * PCIE_CFG_SIZE) + (func * PCIE_CFG_SIZE);

  pal_mmio_write(ecam_base + cfg_addr + offset, data);

}

/**
  @brief   This API programs all the BAR register in PCIe config space pointed by Bus,
           Device and Function for an End Point PCIe device
  @param   bus,dev,func - Bus(8-bits), device(8-bits) & function(8-bits)

  @return  None
**/
void pal_pcie_program_bar_reg(uint32_t bus, uint32_t dev, uint32_t func)
{

  uint64_t bar_size, bar_upper_bits;
  uint32_t bar_reg_value, offset = BAR0_OFFSET;

  while(offset <= BAR_MAX_OFFSET)
  {
    pal_pci_cfg_read(bus, dev, func, offset, &bar_reg_value);
    if (bar_reg_value == 0)
    {
	    /** This BAR is not implemented **/
            offset=offset + 4;
            continue;
    }

    if (BAR_REG(bar_reg_value) == BAR_64_BIT)
    {
        print(AVS_PRINT_INFO, "The BAR supports 64-bit address decoding capability \n", 0);

        /** BAR supports 64-bit address therefore, write all 1's
          *  to BARn and BARn+1 and identify the size requested
        **/
        pal_pci_cfg_write(bus, dev, func, offset, 0xFFFFFFF0);
        pal_pci_cfg_write(bus, dev, func, offset+4, 0xFFFFFFFF);

        pal_pci_cfg_read(bus, dev, func, offset, &bar_reg_value);
        bar_size = bar_reg_value & 0xFFFFFFF0;

        pal_pci_cfg_read(bus, dev, func, offset+4, &bar_reg_value);
        bar_upper_bits = bar_reg_value;
        bar_size = bar_size | (bar_upper_bits << 32 );

        bar_size = ~bar_size + 1;
        pal_pci_cfg_write(bus, dev, func, offset, g_bar_start);
        print(AVS_PRINT_INFO, "Value written to BAR register is %x\n", g_bar_start);
        g_bar_start = g_bar_start + bar_size;
        offset=offset+8;
    }

    if (BAR_REG(bar_reg_value) == BAR_32_BIT)
    {
        print(AVS_PRINT_INFO, "The BAR supports 32-bit address decoding capability\n", 0);

        /**BAR supports 32-bit address. Write all 1's
         * to BARn and identify the size requested
        **/
        pal_pci_cfg_write(bus, dev, func, offset, 0xFFFFFFF0);
        pal_pci_cfg_read(bus, dev, func, offset, &bar_reg_value);
        bar_reg_value = bar_reg_value & 0xFFFFFFF0;
        bar_size = ~bar_reg_value + 1;
        pal_pci_cfg_write(bus, dev, func, offset, g_bar_start);
        print(AVS_PRINT_INFO, "Value written to BAR register is %x\n", g_bar_start);
        g_bar_start = g_bar_start + bar_size;
        offset=offset+4;
     }
  }
}

/**
  @brief   This API performs the PCIe bus enumeration
  @param   bus,sec_bus - Bus(8-bits), secondary bus (8-bits)

  @return  sub_bus - Subordinate bus
**/
uint32_t pal_pcie_enumerate_device(uint32_t bus, uint32_t sec_bus)
{

  uint32_t vendor_id;
  uint32_t header_value;
  uint32_t sub_bus = bus;
  uint32_t dev;
  uint32_t func;

  if (bus == (PCIE_MAX_BUS-1))
      return sub_bus;

  for (dev = 0; dev < PCIE_MAX_DEV; dev++)
  {
    for (func = 0; func < PCIE_MAX_FUNC; func++)
    {
        pal_pci_cfg_read(bus, dev, func, 0, &vendor_id);
        if ((vendor_id == 0x0) || (vendor_id == 0xFFFFFFFF))
                continue;

        print(AVS_PRINT_INFO, "The Vendor id read is %x\n", vendor_id);
        print(AVS_PRINT_INFO, "Valid PCIe device found at %x %x %x\n ", bus, dev, func);
        pal_pci_cfg_read(bus, dev, func, HEADER_OFFSET, &header_value);
        if (PCIE_HEADER_TYPE(header_value) == TYPE1_HEADER)
        {
            print(AVS_PRINT_INFO, "TYPE1 HEADER found\n", 0);
            pal_pci_cfg_write(bus, dev, func, BUS_NUM_REG_OFFSET, BUS_NUM_REG_CFG(0xFF, sec_bus, bus));
            sub_bus = pal_pcie_enumerate_device(sec_bus, (sec_bus+1));
            pal_pci_cfg_write(bus, dev, func, BUS_NUM_REG_OFFSET, BUS_NUM_REG_CFG(sub_bus, sec_bus, bus));
            sec_bus = sub_bus + 1;
        }

        if (PCIE_HEADER_TYPE(header_value) == TYPE0_HEADER)
        {
            print(AVS_PRINT_INFO, "END POINT found\n", 0);
            pal_pcie_program_bar_reg(bus, dev, func);
            sub_bus = sec_bus - 1;
        }

      }
    }
    return sub_bus;
}

void pal_pcie_enumerate(void)
{
    print(AVS_PRINT_TEST, "\nStarting Enumeration \n", 0);
    pal_pcie_enumerate_device(PRI_BUS, SEC_BUS);
}
