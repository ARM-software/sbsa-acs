/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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

#include "pal_common_support.h"
#include "platform_override_struct.h"

/**
  @brief   This API checks the PCIe hierarchy fo P2P support
           1. Caller       -  Test Suite
  @return  1 - P2P feature not supported 0 - P2P feature supported
**/
uint32_t
pal_pcie_p2p_support(void)
{
  // This API checks the PCIe hierarchy for P2P support as defined
  // in the PCIe platform configuration

  return PLATFORM_PCIE_P2P_NOT_SUPPORTED;

}

/**
    @brief   Return if driver present for pcie device
    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @return  Driver present : 0 or 1
**/
uint32_t
pal_pcie_device_driver_present(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{
  (void) seg;
  (void) bus;
  (void) dev;
  (void) fn;

  return 1;

}

/**
    @brief   Create a list of MSI(X) vectors for a device

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @param   mvector    pointer to a MSI(X) list address

    @return  mvector    list of MSI(X) vectors
    @return  number of MSI(X) vectors
**/
uint32_t
pal_get_msi_vectors(uint32_t Seg, uint32_t Bus, uint32_t Dev, uint32_t Fn, PERIPHERAL_VECTOR_LIST **MVector)
{
  (void) Seg;
  (void) Bus;
  (void) Dev;
  (void) Fn;
  (void) MVector;

  return 0;
}

/**
    @brief   Gets RP support of transaction forwarding.

    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number
    @param   seg        PCI segment number

    @return  0 if rp not involved in transaction forwarding
             1 if rp is involved in transaction forwarding
**/
uint32_t
pal_pcie_get_rp_transaction_frwd_support(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{
  (void) seg;
  (void) bus;
  (void) dev;
  (void) fn;

  return 1;
}

/**
  @brief  Returns whether a PCIe Function is an on-chip peripheral or not

  @param  bdf        - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return Returns TRUE if the Function is on-chip peripheral, FALSE if it is
          not an on-chip peripheral
**/
uint32_t
pal_pcie_is_onchip_peripheral(uint32_t bdf)
{
  (void) bdf;

  return 0;
}

/**
  @brief  This API is used as placeholder to check if the bdf
          obtained is valid or not

  @param  bdf
  @return 0 if bdf is valid else 1
**/
uint32_t
pal_pcie_check_device_valid(uint32_t bdf)
{

  /*Add BDFs to this function if PCIe tests
    need to be ignored for a BDF for any reason
  */

  (void) bdf;

  return 0;
}

/**
  @brief  Returns the memory offset that can be accesed safely.
          This offset is platform-specific. It needs to
          be modified according to the requirement.

  @param  bdf      - PCIe BUS/Device/Function
  @param  mem_type - If the memory is Pre-fetchable or Non-prefetchable memory
  @return memory offset
**/
uint32_t
pal_pcie_mem_get_offset(uint32_t bdf, PCIE_MEM_TYPE_INFO_e mem_type)
{

  (void) bdf;
  (void) mem_type;
  return MEM_OFFSET_SMALL;
}
