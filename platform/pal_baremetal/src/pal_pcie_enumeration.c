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
#include "include/platform_override_fvp.h"

extern PCIE_INFO_TABLE *g_pcie_info_table;

uint64_t g_bar64_start = PLATFORM_OVERRIDE_PCIE_BAR64_VAL;
uint32_t g_bar32_start = PLATFORM_OVERRIDE_PCIE_BAR32_VAL;

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
        pal_pci_cfg_write(bus, dev, func, offset, g_bar64_start);
        print(AVS_PRINT_INFO, "Value written to BAR register is %x\n", g_bar64_start);
        g_bar64_start = g_bar64_start + bar_size;
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
        pal_pci_cfg_write(bus, dev, func, offset, g_bar32_start);
        print(AVS_PRINT_INFO, "Value written to BAR register is %x\n", g_bar32_start);
        g_bar32_start = g_bar32_start + bar_size;
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

/**
    @brief   Returns the Bus, Dev, Function (in the form seg<<24 | bus<<16 | Dev <<8 | func)
             for a matching class code.

    @param   ClassCode  - is a 32bit value of format ClassCode << 16 | sub_class_code
    @param   StartBdf   - is 0     : start enumeration from Host bridge
                          is not 0 : start enumeration from the input segment, bus, dev
                          this is needed as multiple controllers with same class code are
                          potentially present in a system.
    @return  the BDF of the device matching the class code
**/
uint32_t
pal_pcie_get_bdf(uint32_t ClassCode, uint32_t StartBdf)
{

  uint32_t  Bus, InputBus, Seg = 0;
  uint32_t  Dev, InputDev;
  uint32_t  Func, InputFunc;
  uint32_t class_code;
  InputBus  = PCIE_EXTRACT_BDF_BUS(StartBdf);
  InputDev  = PCIE_EXTRACT_BDF_DEV(StartBdf);
  InputFunc = PCIE_EXTRACT_BDF_FUNC(StartBdf);

  for (Bus = InputBus; Bus < PCIE_MAX_BUS; Bus++)
  {
    for (Dev = InputDev; Dev < PCIE_MAX_DEV; Dev++)
    {
      for (Func = InputFunc; Func < PCIE_MAX_FUNC; Func++)
      {
        pal_pci_cfg_read(Bus, Dev, Func, TYPE01_RIDR, &class_code);
        if ((class_code >> CC_BASE_SHIFT) == (ClassCode >> 16))
        {
          if((class_code >> CC_SUB_SHIFT) == (ClassCode >> 8))
          {
           // Found our device
           // Return the BDF*/
            return (uint32_t)(PCIE_CREATE_BDF(Seg, Bus, Dev, Func));
          }
        }
      }
    }
  }
  return 0;
}

/**
  @brief  Increment the Device number (and Bus number if Dev num reaches 32) to the next valid value.

  @param  StartBdf  Segment/Bus/Dev/Func in the format created by PCIE_CREATE_BDF

  @return the new incremented BDF
**/
uint32_t
pal_increment_bus_dev(uint32_t StartBdf)
{

  uint32_t Seg = PCIE_EXTRACT_BDF_SEG(StartBdf);
  uint32_t Bus = PCIE_EXTRACT_BDF_BUS(StartBdf);
  uint32_t Dev = PCIE_EXTRACT_BDF_DEV(StartBdf);


  if (Dev != (PCIE_MAX_DEV-1)) {
      Dev++;
  } else {
      Bus++;
      Dev = 0;
  }

  return PCIE_CREATE_BDF(Seg, Bus, Dev, 0);
}

/**
  @brief  This API returns the Base Address Register value for a given BDF and index
  @param  bdf       - the device whose PCI Config space BAR needs to be returned.
  @param  bar_index - the '0' based BAR index identifying the correct 64-bit BAR

  @return the 64-bit BAR value
*/
uint64_t
pal_pcie_get_base(uint32_t bdf, uint32_t bar_index)
{
  uint32_t   Bus, Dev, Func;
  uint32_t   offset;
  uint32_t   bar_reg_value;
  uint64_t   bar_value, bar_upper_bits;

  Bus  = PCIE_EXTRACT_BDF_BUS(bdf);
  Dev  = PCIE_EXTRACT_BDF_DEV(bdf);
  Func = PCIE_EXTRACT_BDF_FUNC(bdf);

  offset = BAR0_OFFSET + (4*bar_index);
  pal_pci_cfg_read(Bus, Dev, Func, offset, &bar_reg_value);
  if (BAR_REG(bar_reg_value) == BAR_64_BIT)
  {
     bar_value = bar_reg_value & 0xFFFFFFF0;
     pal_pci_cfg_read(Bus, Dev, Func, offset+4, &bar_reg_value);
     bar_upper_bits = bar_reg_value;
     bar_value = bar_value | (bar_upper_bits << 32 );
  }
  if (BAR_REG(bar_reg_value) == BAR_32_BIT)
  {
     bar_value = bar_reg_value;
  }
  print(AVS_PRINT_INFO, "value read from BAR %x\n", bar_value);

  return bar_value;

}

/**
    @brief   Returns the Bus, Dev, Function (in the form seg<<24 | bus<<16 | Dev <<8 | func)
             for a matching class code.

    @param   ClassCode  - is a 32bit value of format ClassCode << 16 | sub_class_code
    @param   StartBdf   - is 0     : start enumeration from Host bridge
                          is not 0 : start enumeration from the input segment, bus, dev
                          this is needed as multiple controllers with same class code are
                          potentially present in a system.
    @return  the BDF of the device matching the class code
**/
uint32_t
pal_pcie_get_bdf_wrapper(uint32_t class_code, uint32_t start_bdf)
{

  return pal_pcie_get_bdf(class_code, start_bdf);
}

/**

    @brief   Returns the Device ID of the bdf

    @param   bdf - Bus, Device and Function of the device

    @return  device_id on success
**/
void *
pal_pci_bdf_to_dev(uint32_t bdf)
{

  uint32_t bus;
  uint32_t dev;
  uint32_t func;
  uint32_t vendor_id, *device_id;

  bus  = PCIE_EXTRACT_BDF_BUS(bdf);
  dev  = PCIE_EXTRACT_BDF_DEV(bdf);
  func = PCIE_EXTRACT_BDF_FUNC(bdf);

  pal_pci_cfg_read(bus, dev, func, 0, &vendor_id);
  vendor_id = vendor_id >> DEVICE_ID_OFFSET;
  device_id = &vendor_id;

  return (void *)device_id;

}
