/** @file
 * Copyright (c) 2020-2023,Arm Limited or its affiliates. All rights reserved.
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

#include "include/pal_common_support.h"
#include "include/pal_pcie_enum.h"

extern PCIE_INFO_TABLE *g_pcie_info_table;

uint64_t
pal_exerciser_get_pcie_config_offset(uint32_t Bdf)
{
  uint32_t bus     = PCIE_EXTRACT_BDF_BUS(Bdf);
  uint32_t dev     = PCIE_EXTRACT_BDF_DEV(Bdf);
  uint32_t func    = PCIE_EXTRACT_BDF_FUNC(Bdf);
  uint64_t cfg_addr;

   /* There are 8 functions / device, 32 devices / Bus and each has a 4KB config space */
  cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
               (dev * PCIE_MAX_FUNC * 4096) + (func * 4096);

  return cfg_addr;
}


uint64_t
pal_exerciser_get_ecam(uint32_t bdf)
{

  uint32_t i = 0;
  uint32_t bus = PCIE_EXTRACT_BDF_BUS(bdf);

  while (i < g_pcie_info_table->num_entries)
  {
    if ((bus >= g_pcie_info_table->block[i].start_bus_num) &&
         (bus <= g_pcie_info_table->block[i].end_bus_num))
    {
      return g_pcie_info_table->block[i].ecam_base;
    }
    i++;
  }
  print(AVS_PRINT_ERR, "\n No ECAM base ",0);
  return 0;
}

/**
  @brief This API will return the ECSR base address of particular BAR Index
**/
uint64_t
pal_exerciser_get_ecsr_base(uint32_t Bdf, uint32_t BarIndex)
{
    return pal_pcie_get_base(Bdf, BarIndex);
}

/**
  @brief This function finds the PCI capability and return 0 if it finds.
**/
uint32_t pal_exerciser_find_pcie_capability (uint32_t ID, uint32_t Bdf, uint32_t Value, uint32_t *Offset)
{

  uint64_t NxtPtr;
  uint32_t Data;
  uint32_t TempId;
  uint64_t Ecam;
  uint32_t IdMask;
  uint32_t PtrMask;
  uint32_t PtrOffset;

  Ecam = pal_exerciser_get_ecam(Bdf);
  NxtPtr = PCIE_CAP_OFFSET;

  if (Value == 1) {
      IdMask = PCIE_CAP_ID_MASK;
      PtrMask = PCIE_NXT_CAP_PTR_MASK;
      PtrOffset = PCIE_CAP_PTR_OFFSET;
      NxtPtr = PCIE_CAP_OFFSET;
  }
  else {
      IdMask = PCI_CAP_ID_MASK;
      PtrMask = PCI_NXT_CAP_PTR_MASK;
      PtrOffset = PCI_CAP_PTR_OFFSET;
      NxtPtr = ((pal_mmio_read(Ecam + CAP_PTR_OFFSET + pal_exerciser_get_pcie_config_offset(Bdf)))
                               & CAP_PTR_MASK);
  }

  while (NxtPtr != 0) {
    Data = pal_mmio_read(Ecam + pal_exerciser_get_pcie_config_offset(Bdf) + NxtPtr);
    TempId = Data & IdMask;
    if (TempId == ID){
        *Offset = NxtPtr;
        return 0;
    }

    NxtPtr = (Data >> PtrOffset) & PtrMask;
  }

  print(AVS_PRINT_ERR, "\n No capabilities found",0);
  return 1;
}

