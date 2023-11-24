/** @file
 * Copyright (c) 2020-2023, Arm Limited or its affiliates. All rights reserved.
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

#include "pal_pcie_enum.h"
#include "pal_common_support.h"
#include "platform_override_struct.h"

extern pcie_device_bdf_table *g_pcie_bdf_table;
extern PCIE_INFO_TABLE platform_pcie_cfg;
extern PCIE_READ_TABLE platform_pcie_device_hierarchy;
extern PERIPHERAL_INFO_TABLE  *g_peripheral_info_table;

uint64_t
pal_pcie_get_mcfg_ecam()
{

  /* Not Applicable for Baremetal as no MCFG table */
  return (platform_pcie_cfg.block[0].ecam_base);
}


void
pal_pcie_create_info_table(PCIE_INFO_TABLE *PcieTable)
{

  uint32_t i = 0;

  if (PcieTable == NULL) {
    print(AVS_PRINT_ERR, "Input PCIe Table Pointer is NULL. Cannot create PCIe INFO\n");
    return;
  }

  PcieTable->num_entries = 0;

  if(platform_pcie_cfg.num_entries == 0) {
    print(AVS_PRINT_ERR, "Number of ECAM is 0. Cannot create PCIe INFO\n");
    return;
  }


  for(i = 0; i < platform_pcie_cfg.num_entries; i++)
  {
      PcieTable->block[i].ecam_base      = platform_pcie_cfg.block[i].ecam_base;
      PcieTable->block[i].segment_num    = platform_pcie_cfg.block[i].segment_num;
      PcieTable->block[i].start_bus_num  = platform_pcie_cfg.block[i].start_bus_num;
      PcieTable->block[i].end_bus_num    = platform_pcie_cfg.block[i].end_bus_num;
      PcieTable->num_entries++;
   }
  return;
}

/**
  @brief  Returns the ECAM address of the input PCIe bridge function

  @param   bus        PCI bus address
  @param   dev        PCI device address
  @param   fn         PCI function number
  @param   seg        PCI segment number

  @return ECAM address if success, else NULL address
**/

uint64_t
pal_pcie_ecam_base(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t func)
{

  uint8_t ecam_index;
  uint64_t ecam_base;

  (void) dev;
  (void) func;

  ecam_index = 0;
  ecam_base = 0;


  while (ecam_index < platform_pcie_cfg.num_entries)
  {
      if ((bus >= platform_pcie_cfg.block[ecam_index].start_bus_num) &&
          (bus <= platform_pcie_cfg.block[ecam_index].end_bus_num) &&
          (seg == platform_pcie_cfg.block[ecam_index].segment_num ))
      {
          ecam_base = platform_pcie_cfg.block[ecam_index].ecam_base;
          break;
      }
      ecam_index++;
  }

  return ecam_base;
}

/**
    @brief   Reads 32-bit data from PCIe config space pointed by Bus,
           Device, Function and register offset

    @param   Bdf      - BDF value for the device
    @param   offset - Register offset within a device PCIe config space
    @param   *data - 32 bit value at offset from ECAM base of the device specified by BDF value
    @return  success/failure
**/
uint32_t
pal_pcie_read_cfg(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t func, uint32_t offset, uint32_t *value)
{

  uint32_t cfg_addr;
  uint64_t ecam_base = pal_pcie_ecam_base(seg, bus, dev, func);

  ecam_base = pal_pcie_ecam_base(seg, bus, dev, func);
  cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
               (dev * PCIE_MAX_FUNC * 4096) + (func * 4096);

  *value = pal_mmio_read(ecam_base + cfg_addr + offset);
  return 0;

}

/**
    @brief   Reads 32-bit data from PCIe platform config file pointed by Bus,
           Device, Function and register offset

    @param   Bdf      - BDF value for the device
    @param   offset - Register offset within a device PCIe config space
    @param   *data - 32 bit value at offset from ECAM base of the device specified by BDF value
    @return  success/failure
**/
uint32_t
pal_pcie_io_read_cfg(uint32_t Bdf, uint32_t offset, uint32_t *data)
{

  uint32_t InputSeg, InputBus, InputDev, InputFunc;
  uint32_t i;

  InputSeg  = PCIE_EXTRACT_BDF_SEG(Bdf);
  InputBus  = PCIE_EXTRACT_BDF_BUS(Bdf);
  InputDev  = PCIE_EXTRACT_BDF_DEV(Bdf);
  InputFunc = PCIE_EXTRACT_BDF_FUNC(Bdf);

  for(i = 0; i < platform_pcie_device_hierarchy.num_entries; i++)
  {
     if(InputSeg  == platform_pcie_device_hierarchy.device[i].seg &&
        InputBus  == platform_pcie_device_hierarchy.device[i].bus &&
        InputDev  == platform_pcie_device_hierarchy.device[i].dev &&
        InputFunc == platform_pcie_device_hierarchy.device[i].func)
        {
           if (offset == TYPE01_RIDR)
           {
              *data = platform_pcie_device_hierarchy.device[i].class_code;
              return 0;
           }
           /* conditions to read other details
            * place holder
            */
         }
  }

  print(AVS_PRINT_ERR, "No PCI devices found in the system\n");
  return PCIE_NO_MAPPING;
}

/**
    @brief Write 32-bit data to PCIe config space pointed by Bus,
           Device, Function and register offset

    @param   bdf      - BDF value for the device
    @param   offset - Register offset within a device PCIe config space
    @param   data - 32 bit value at offset from ECAM base of the device specified by BDF value
    @return  success/failure
**/
void
pal_pcie_io_write_cfg(uint32_t bdf, uint32_t offset, uint32_t data)
{

  uint32_t bus  = PCIE_EXTRACT_BDF_BUS(bdf);
  uint32_t dev  = PCIE_EXTRACT_BDF_DEV(bdf);
  uint32_t func = PCIE_EXTRACT_BDF_FUNC(bdf);
  uint32_t seg  = PCIE_EXTRACT_BDF_SEG(bdf);
  uint32_t cfg_addr;
  uint64_t ecam_base = 0;

  ecam_base = pal_pcie_ecam_base(seg, bus, dev, func);
  cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
               (dev * PCIE_MAX_FUNC * 4096) + (func * 4096);

  pal_mmio_write(ecam_base + cfg_addr + offset, data);

}

/**
    @brief   Get the PCIe device/port type

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number

    @return  Returns PCIe device/port type
**/
uint32_t
pal_pcie_get_pcie_type(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{

  uint32_t reg_value;
  uint32_t next_cap_offset;

  pal_pcie_read_cfg(seg, bus, dev, fn, TYPE01_CPR, &reg_value);
  next_cap_offset = (reg_value & TYPE01_CPR_MASK);
  while (next_cap_offset)
  {
     pal_pcie_read_cfg(seg, bus, dev, fn, next_cap_offset, &reg_value);
     if ((reg_value & PCIE_CIDR_MASK) == CID_PCIECS)
     {
          return ((reg_value >> PCIE_DEVICE_TYPE_SHIFT) & PCIE_DEVICE_TYPE_MASK);
     }
     next_cap_offset = ((reg_value >> PCIE_NCPR_SHIFT) & PCIE_NCPR_MASK);
  }

  return 0;
}

/**
    @brief   Get the PCIe device snoop bit transaction attribute

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number

    @return  0 snoop
             1 no snoop
             2 device error
**/
uint32_t
pal_pcie_get_snoop_bit(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{

  uint32_t reg_value;
  uint32_t next_cap_offset;

  pal_pcie_read_cfg(seg, bus, dev, fn,TYPE01_CPR, &reg_value);
  next_cap_offset = (reg_value & TYPE01_CPR_MASK);
  while (next_cap_offset)
  {
     pal_pcie_read_cfg(seg, bus, dev, fn, next_cap_offset, &reg_value);
     if ((reg_value & PCIE_CIDR_MASK) == CID_PCIECS)
     {
          pal_pcie_read_cfg(seg, bus, dev, fn, next_cap_offset + PCI_EXP_DEVCTL, &reg_value);
          /* Extract bit 11 (no snoop) */
          return ((reg_value >> DEVCTL_SNOOP_BIT) & 0x1);
     }
     next_cap_offset = ((reg_value >> PCIE_NCPR_SHIFT) & PCIE_NCPR_MASK);
  }

  return 2;
}

void
pal_pcie_read_ext_cap_word(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn,
                           uint32_t ext_cap_id, uint8_t offset, uint16_t *val)
{

  uint32_t next_cap_offset;
  uint32_t reg_value;

  next_cap_offset = PCIE_ECAP_START;
  while (next_cap_offset)
  {
      pal_pcie_read_cfg(seg, bus, dev, fn, next_cap_offset, &reg_value);
      if ((reg_value & PCIE_ECAP_CIDR_MASK) == ext_cap_id)
      {
          pal_pcie_read_cfg(seg, bus, dev, fn, next_cap_offset + offset, &reg_value);
          *val = reg_value & 0xffff;
      }
      next_cap_offset = ((reg_value >> PCIE_ECAP_NCPR_SHIFT) & PCIE_ECAP_NCPR_MASK);
  }

  return;
}


/**
    @brief   Checks if device is behind SMMU

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number

    @return  status code:0 -> not present, nonzero -> present
**/
uint32_t
pal_pcie_is_device_behind_smmu(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{

  uint32_t i;

  for(i = 0; i < platform_pcie_device_hierarchy.num_entries; i++)
  {
     if(seg == platform_pcie_device_hierarchy.device[i].seg &&
        bus == platform_pcie_device_hierarchy.device[i].bus &&
        dev == platform_pcie_device_hierarchy.device[i].dev &&
        fn  == platform_pcie_device_hierarchy.device[i].func)
        {
              return platform_pcie_device_hierarchy.device[i].behind_smmu;
        }
  }

  return 0;
}

/**
    @brief   Get the PCIe device DMA support

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number

    @return  0 no support
             1 support
             2 device error
**/
uint32_t
pal_pcie_get_dma_support(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{

  uint32_t i;

  for(i = 0; i < platform_pcie_device_hierarchy.num_entries; i++)
  {
     if(seg == platform_pcie_device_hierarchy.device[i].seg &&
        bus == platform_pcie_device_hierarchy.device[i].bus &&
        dev == platform_pcie_device_hierarchy.device[i].dev &&
        fn  == platform_pcie_device_hierarchy.device[i].func)
        {
              return platform_pcie_device_hierarchy.device[i].dma_support;
        }
  }

  return 2;

}

/**
    @brief   Get the PCIe device DMA coherency support

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number

    @return  0 DMA is not coherent
             1 DMA is coherent
             2 device error
**/
uint32_t
pal_pcie_get_dma_coherent(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{

  uint32_t i;

  for(i = 0; i < platform_pcie_device_hierarchy.num_entries; i++)
  {
     if(seg == platform_pcie_device_hierarchy.device[i].seg &&
        bus == platform_pcie_device_hierarchy.device[i].bus &&
        dev == platform_pcie_device_hierarchy.device[i].dev &&
        fn  == platform_pcie_device_hierarchy.device[i].func)
        {
              return platform_pcie_device_hierarchy.device[i].dma_coherent;
        }
  }

  return 2;

}

/**
  @brief   This API checks the PCIe device P2P support
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  1 - P2P feature not supported 0 - P2P feature supported
**/
uint32_t
pal_pcie_dev_p2p_support(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{
  uint32_t i;

  for(i = 0; i < platform_pcie_device_hierarchy.num_entries; i++)
  {
     if(seg == platform_pcie_device_hierarchy.device[i].seg &&
        bus == platform_pcie_device_hierarchy.device[i].bus &&
        dev == platform_pcie_device_hierarchy.device[i].dev &&
        fn  == platform_pcie_device_hierarchy.device[i].func)
        {
              return platform_pcie_device_hierarchy.device[i].p2p_support;
        }
  }

  return 1;
}

/**
    @brief   Return the DMA addressability of the device

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number

    @return  DMA Mask : 0 or 1
**/
uint32_t
pal_pcie_is_devicedma_64bit(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{

  uint32_t i;

  for(i = 0; i < platform_pcie_device_hierarchy.num_entries; i++)
  {
     if(seg == platform_pcie_device_hierarchy.device[i].seg &&
        bus == platform_pcie_device_hierarchy.device[i].bus &&
        dev == platform_pcie_device_hierarchy.device[i].dev &&
        fn  == platform_pcie_device_hierarchy.device[i].func)
        {
              return platform_pcie_device_hierarchy.device[i].dma_64bit;
        }
  }

  return 0;

}

/**
  @brief  This API checks if a PCIe device has an Address
          Translation Cache or not.
  @param   bdf      - PCIe BUS/Device/Function
  @return  1 - Address translations cached 0 - Address translations not cached
 **/
uint32_t
pal_pcie_is_cache_present(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{
  uint32_t i;

  // Search for the device in the manufacturer data table
  for(i = 0; i < platform_pcie_device_hierarchy.num_entries; i++)
  {
     if(seg == platform_pcie_device_hierarchy.device[i].seg &&
        bus == platform_pcie_device_hierarchy.device[i].bus &&
        dev == platform_pcie_device_hierarchy.device[i].dev &&
        fn  == platform_pcie_device_hierarchy.device[i].func)
        {
              return platform_pcie_device_hierarchy.device[i].atc_present;
        }
  }

  return 0;
}

/**
    @brief   Get legacy IRQ routing for a PCI device

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @param   irq_map    pointer to IRQ map structure

    @return  irq_map    IRQ routing map
    @return  status code
**/
uint32_t
pal_pcie_get_legacy_irq_map(uint32_t Seg, uint32_t Bus, uint32_t Dev, uint32_t Fn, PERIPHERAL_IRQ_MAP *IrqMap)
{
  uint32_t i;

  for(i = 0; i < platform_pcie_device_hierarchy.num_entries; i++)
  {
     if(Seg  == platform_pcie_device_hierarchy.device[i].seg &&
        Bus  == platform_pcie_device_hierarchy.device[i].bus &&
        Dev  == platform_pcie_device_hierarchy.device[i].dev &&
        Fn == platform_pcie_device_hierarchy.device[i].func)
        {
            pal_memcpy(IrqMap, &platform_pcie_device_hierarchy.device[i].irq_map,
                       sizeof(platform_pcie_device_hierarchy.device[i].irq_map));
            return 0;
        }
  }

  print(AVS_PRINT_ERR, "No PCI devices found in the system\n");
  return PCIE_NO_MAPPING;
}

/**
    @brief   Get bdf of root port

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @param   seg        PCI segment number

    @return  BDF of root port
    @return  status code
             0: Success
             1: Input BDF cannot be found
             2: RP of input device not found
**/
uint32_t
pal_pcie_get_root_port_bdf(uint32_t *Seg, uint32_t *Bus, uint32_t *Dev, uint32_t *Func)
{

  uint32_t bdf;
  uint32_t dp_type;
  uint32_t tbl_index;
  uint32_t reg_value;
  uint32_t next_cap_offset;
  uint32_t curr_seg, curr_bus, curr_dev, curr_func;
  pcie_device_bdf_table *bdf_tbl_ptr;

  tbl_index = 0;
  bdf_tbl_ptr = g_pcie_bdf_table;

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      curr_seg  = PCIE_EXTRACT_BDF_SEG(bdf);
      curr_bus  = PCIE_EXTRACT_BDF_BUS(bdf);
      curr_dev  = PCIE_EXTRACT_BDF_DEV(bdf);
      curr_func = PCIE_EXTRACT_BDF_FUNC(bdf);
      pal_pcie_read_cfg(curr_seg, curr_bus, curr_dev, curr_func, TYPE01_CPR, &reg_value);
      next_cap_offset = (reg_value & TYPE01_CPR_MASK);
      while (next_cap_offset)
      {
         pal_pcie_read_cfg(curr_seg, curr_bus, curr_dev, curr_func, next_cap_offset, &reg_value);
         if ((reg_value & PCIE_CIDR_MASK) == CID_PCIECS)
         {
            pal_pcie_read_cfg(curr_seg, curr_bus, curr_dev, curr_func, next_cap_offset + CIDR_OFFSET, &reg_value);
            /* Read Device/Port bits [7:4] in Function's PCIe Capabilities register */
            dp_type = (reg_value >> ((PCIECR_OFFSET - CIDR_OFFSET)*8 +
                            PCIECR_DPT_SHIFT)) & PCIECR_DPT_MASK;
            dp_type = (1 << dp_type);

            if (dp_type == RP || dp_type == iEP_RP)
            {
                 /* Check if the entry's bus range covers down stream function */
               pal_pcie_read_cfg(curr_seg, curr_bus, curr_dev, curr_func, BUS_NUM_REG_OFFSET, &reg_value);
               if ((*Bus >= ((reg_value >> SECBN_SHIFT) & SECBN_MASK)) &&
                   (*Bus <= ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK)))
               {
                   *Seg  = PCIE_EXTRACT_BDF_SEG(bdf);
                   *Bus  = PCIE_EXTRACT_BDF_BUS(bdf);
                   *Dev  = PCIE_EXTRACT_BDF_DEV(bdf);
                   *Func = PCIE_EXTRACT_BDF_FUNC(bdf);
                   return 0;
               }
            }
            return 2;
         }
         next_cap_offset = ((reg_value >> PCIE_NCPR_SHIFT) & PCIE_NCPR_MASK);
      }
  }

  return 1;
}

/**
  @brief  Checks the discovered PCIe hierarchy is matching with the
          topology described in info table.
  @return Returns 0 if device entries matches , 1 if there is mismatch.
**/
uint32_t pal_pcie_check_device_list(void)
{
  uint32_t tbl_index = 0;
  uint32_t pltf_pcie_device_bdf;
  uint32_t bdf;
  pcie_device_bdf_table *bdf_tbl_ptr;
  uint32_t vendor_id, device_id, class_code;
  uint32_t pltf_vendor_id, pltf_device_id, pltf_class_code;
  uint32_t i = 0;
  uint32_t Seg, Bus, Dev, Func;
  uint32_t data = 0;

    bdf_tbl_ptr = g_pcie_bdf_table;

    if (platform_pcie_device_hierarchy.num_entries != bdf_tbl_ptr->num_entries) {
      print(AVS_PRINT_ERR, "  Number of PCIe devices entries in \
              info table not equal to platform hierarchy\n", 0);
      return 1;
    }

    while (tbl_index < bdf_tbl_ptr->num_entries)
    {
      Seg  = platform_pcie_device_hierarchy.device[tbl_index].seg;
      Bus  = platform_pcie_device_hierarchy.device[tbl_index].bus;
      Dev  = platform_pcie_device_hierarchy.device[tbl_index].dev;
      Func = platform_pcie_device_hierarchy.device[tbl_index].func;
      pltf_vendor_id = platform_pcie_device_hierarchy.device[tbl_index].vendor_id;
      pltf_device_id = platform_pcie_device_hierarchy.device[tbl_index].device_id;
      pltf_class_code = platform_pcie_device_hierarchy.device[tbl_index].class_code >> CC_SHIFT;

      pltf_pcie_device_bdf = PCIE_CREATE_BDF(Seg, Bus, Dev, Func);
      tbl_index++;
      while (i < bdf_tbl_ptr->num_entries) {
          bdf = bdf_tbl_ptr->device[i++].bdf;

          if (pltf_pcie_device_bdf == bdf)
          {

             Seg  = PCIE_EXTRACT_BDF_SEG(bdf);
             Bus  = PCIE_EXTRACT_BDF_BUS(bdf);
             Dev  = PCIE_EXTRACT_BDF_DEV(bdf);
             Func = PCIE_EXTRACT_BDF_FUNC(bdf);
             pal_pcie_read_cfg(Seg, Bus, Dev, Func, TYPE0_HEADER, &data);
             vendor_id = data & 0xFFFF;
             if (vendor_id != pltf_vendor_id) {
                print(AVS_PRINT_ERR, " VendorID mismatch for PCIe device with bdf = 0x%x\n", bdf);
                return 1;
             }
             device_id = data >> DEVICE_ID_OFFSET;
             if (device_id != pltf_device_id) {
                print(AVS_PRINT_ERR, " DeviceID mismatch for PCIe device with bdf = 0x%x\n", bdf);
                return 1;
             }
             pal_pcie_read_cfg(Seg, Bus, Dev, Func, TYPE01_RIDR, &class_code);
             class_code = class_code >> CC_SHIFT;
             if (class_code != pltf_class_code) {
                print(AVS_PRINT_ERR, "ClassCode mismatch for PCIe device with bdf = 0x%x\n", bdf);
                return 1;
             }

             i = 0;
             break;
          }

      }

      /* If any bdf match not found in platform device hierarchy and info table, return false */
      if (i == bdf_tbl_ptr->num_entries) {
          print(AVS_PRINT_ERR, " Bdf not found in info table = 0x%x\n", pltf_pcie_device_bdf);
          return 1;
      }
    }
    return 0;
}

/**
  @brief  Returns the memory offset that can be accesed.
          This offset is platform-specific. It needs to
          be modified according to the requirement.

  @param  memory offset
  @return memory offset
**/
uint32_t
pal_pcie_mem_get_offset(uint32_t type)
{

  switch (type) {
      case MEM_OFFSET_SMALL:
         return MEM_OFFSET_SMALL;
      case MEM_OFFSET_MEDIUM:
         return MEM_OFFSET_MEDIUM;
      default:
         return MEM_OFFSET_SMALL;
  }

}

/**
    @brief   Reads 32-bit data from BAR space pointed by Bus,
             Device, Function and register offset.

    @param   Bdf     - BDF value for the device
    @param   address - BAR memory address
    @param   *data   - 32 bit value at BAR address
    @return  success/failure
**/
uint32_t
pal_pcie_bar_mem_read(uint32_t Bdf, uint64_t address, uint32_t *data)
{
  (void) Bdf;

  *data = pal_mmio_read(address);
   return 0;
}

/**
    @brief   Write 32-bit data to BAR space pointed by Bus,
             Device, Function and register offset.

    @param   Bdf     - BDF value for the device
    @param   address - BAR memory address
    @param   data    - 32 bit value to writw BAR address
    @return  success/failure
**/

uint32_t
pal_pcie_bar_mem_write(uint32_t Bdf, uint64_t address, uint32_t data)
{
  (void) Bdf;

   pal_mmio_write(address, data);
   return 0;
}

