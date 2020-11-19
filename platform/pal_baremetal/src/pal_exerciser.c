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

#include "include/pal_common_support.h"
#include "include/pal_pcie_enum.h"
#include "include/platform_override_fvp.h"

/**
  @brief This API will return the ECSR base address of particular BAR Index
**/
uint64_t
pal_exerciser_get_ecsr_base(uint32_t Bdf, uint32_t BarIndex)
{
    return pal_pcie_get_base(Bdf, BarIndex);
}

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

/**
  @brief   This API writes the configuration parameters of the PCIe stimulus generation hardware
  @param   Type         - Parameter type that needs to be set in the stimulus hadrware
  @param   Value1       - Parameter 1 that needs to be set
  @param   Value2       - Parameter 2 that needs to be set
  @param   Instance     - Stimulus hardware instance number
  @return  Status       - SUCCESS if the input paramter type is successfully written
**/
uint32_t pal_exerciser_set_param(EXERCISER_PARAM_TYPE Type, uint64_t Value1, uint64_t Value2, uint32_t Bdf)
{
  return 0;
}

/**
  @brief   This API reads the configuration parameters of the PCIe stimulus generation hardware
  @param   Type         - Parameter type that needs to be read from the stimulus hadrware
  @param   Value1       - Parameter 1 that is read from hardware
  @param   Value2       - Parameter 2 that is read from hardware
  @param   Instance     - Stimulus hardware instance number
  @return  Status       - SUCCESS if the requested paramter type is successfully read
**/
uint32_t pal_exerciser_get_param(EXERCISER_PARAM_TYPE Type, uint64_t *Value1, uint64_t *Value2, uint32_t Bdf)
{
  return 0;
}

/**
  @brief   This API obtains the state of the PCIe stimulus generation hardware
  @param   State        - State that is read from the stimulus hadrware
  @param   Bdf          - Stimulus hardware bdf number
  @return  Status       - SUCCESS if the state is successfully read from hardware
**/
uint32_t pal_exerciser_get_state(EXERCISER_STATE *State, uint32_t Bdf)
{
    *State = EXERCISER_ON;
    return 0;
}

/**
  @brief   This API performs the input operation using the PCIe stimulus generation hardware
  @param   Ops          - Operation thta needs to be performed with the stimulus hadrware
  @param   Value        - Additional information to perform the operation
  @param   Instance     - Stimulus hardware instance number
  @return  Status       - SUCCESS if the operation is successfully performed using the hardware
**/
uint32_t pal_exerciser_ops(EXERCISER_OPS Ops, uint64_t Param, uint32_t Bdf)
{
  return 0;
}

/**
  @brief This function triggers the DMA operation
**/
uint32_t pal_exerciser_start_dma_direction (uint64_t Base, EXERCISER_DMA_ATTR Direction)
{
   return 0;
}

/**
  @brief This function finds the PCI capability and return 0 if it finds.
**/
uint32_t pal_exerciser_find_pcie_capability (uint32_t ID, uint32_t Bdf, uint32_t Value, uint32_t *Offset)
{
  return 0;
}

/**
  @brief   This API sets the state of the PCIe stimulus generation hardware
  @param   State        - State that needs to be set for the stimulus hadrware
  @param   Value        - Additional information associated with the state
  @param   Instance     - Stimulus hardware instance number
  @return  Status       - SUCCESS if the input state is successfully written to hardware
**/
uint32_t pal_exerciser_set_state (EXERCISER_STATE State, uint64_t *Value, uint32_t Instance)
{
    return 0;
}

/**
  @brief   This API returns test specific data from the PCIe stimulus generation hardware
  @param   type         - data type for which the data needs to be returned
  @param   data         - test specific data to be be filled by pal layer
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the requested data is successfully filled
**/
uint32_t pal_exerciser_get_data(EXERCISER_DATA_TYPE Type, exerciser_data_t *Data, uint32_t Bdf, uint32_t Ecam)
{
   return 1;
}
