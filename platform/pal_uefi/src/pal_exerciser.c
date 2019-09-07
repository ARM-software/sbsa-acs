/** @file
 * Copyright (c) 2018-2019, Arm Limited or its affiliates. All rights reserved.
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


/* This is a place-holder file. Porting to match implementation is required */

#include "include/pal_exerciser.h"

/**
  @brief   This API popultaes information from all the PCIe stimulus generation IP available
           in the system into exerciser_info_table structure
  @param   ExerciserInfoTable   - Table pointer to be filled by this API
  @return  ExerciserInfoTable   - Contains info to communicate with stimulus generation hardware
**/
VOID
pal_exerciser_create_info_table (
  EXERCISER_INFO_TABLE *ExerciserInfoTable
  )
{
    return;
}

/**
  @brief   This API returns the requested information about the PCIe stimulus hardware
  @param   Type         - Information type required from the stimulus hadrware
  @param   Instance     - Stimulus hadrware instance number
  @return  Value        - Information value for input type
**/
UINT32
pal_exerciser_get_info (
  EXERCISER_INFO_TYPE Type,
  UINT32 Instance
  )
{
	return 0;
}

/**
  @brief   This API writes the configuration parameters of the PCIe stimulus generation hardware
  @param   Type         - Parameter type that needs to be set in the stimulus hadrware
  @param   Value1       - Parameter 1 that needs to be set
  @param   Value2       - Parameter 2 that needs to be set
  @param   Instance     - Stimulus hardware instance number
  @return  Status       - SUCCESS if the input paramter type is successfully written
**/
UINT32 pal_exerciser_set_param (
  EXERCISER_PARAM_TYPE Type,
  UINT64 Value1,
  UINT64 Value2,
  UINT32 Instance
  )
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
UINT32
pal_exerciser_get_param (
  EXERCISER_PARAM_TYPE Type,
  UINT64 *Value1,
  UINT64 *Value2,
  UINT32 Instance
  )
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
UINT32
pal_exerciser_set_state (
  EXERCISER_STATE State,
  UINT64 *Value,
  UINT32 Instance
  )
{
	return 0;
}

/**
  @brief   This API obtains the state of the PCIe stimulus generation hardware
  @param   State        - State that is read from the stimulus hadrware
  @param   Value        - Additional information associated with the state
  @param   Instance     - Stimulus hardware instance number
  @return  Status       - SUCCESS if the state is successfully read from hardware
**/
UINT32
pal_exerciser_get_state (
  EXERCISER_STATE State,
  UINT64 *Value,
  UINT32 Instance
  )
{
	return 0;
}

/**
  @brief   This API performs the input operation using the PCIe stimulus generation hardware
  @param   Ops          - Operation thta needs to be performed with the stimulus hadrware
  @param   Value        - Additional information to perform the operation
  @param   Instance     - Stimulus hardware instance number
  @return  Status       - SUCCESS if the operation is successfully performed using the hardware
**/
UINT32
pal_exerciser_ops (
  EXERCISER_OPS Ops,
  UINT64 Param,
  UINT32 Instance
  )
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
UINT32
pal_exerciser_get_data (
  EXERCISER_DATA_TYPE Type,
  exerciser_data_t *Data,
  UINT32 Instance
  )
{
    return 0;
}
