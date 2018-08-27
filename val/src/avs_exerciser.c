/** @file
 * Copyright (c) 2016-2018, Arm Limited or its affiliates. All rights reserved.
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

#include "include/val_exerciser.h"


/**
  @brief   This API popultaes information from all the PCIe stimulus generation IP available
           in the system into exerciser_info_table structure
  @param   exerciser_info_table - Table pointer to be filled by this API
  @return  exerciser_info_table - Contains info to communicate with stimulus generation hardware
**/
void val_exerciser_create_info_table(EXERCISER_INFO_TABLE *exerciser_info_table)
{
    pal_exerciser_create_info_table(exerciser_info_table);
}

/**
  @brief   This API returns the requested information about the PCIe stimulus hardware
  @param   type         - Information type required from the stimulus hadrware
  @param   instance     - Stimulus hadrware instance number
  @return  value        - Information value for input type
**/
uint32_t val_exerciser_get_info(EXERCISER_INFO_TYPE type, uint32_t instance)
{
    return pal_exerciser_get_info(type, instance);
}

/**
  @brief   This API writes the configuration parameters of the PCIe stimulus generation hardware
  @param   type         - Parameter type that needs to be set in the stimulus hadrware
  @param   value1       - Parameter 1 that needs to be set
  @param   value2       - Parameter 2 that needs to be set
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the input paramter type is successfully written
**/
uint32_t val_exerciser_set_param(EXERCISER_PARAM_TYPE type, uint64_t value1, uint64_t value2, uint32_t instance)
{
    return pal_exerciser_set_param(type, value1, value2, instance);
}

/**
  @brief   This API reads the configuration parameters of the PCIe stimulus generation hardware
  @param   type         - Parameter type that needs to be read from the stimulus hadrware
  @param   value1       - Parameter 1 that is read from hardware
  @param   value2       - Parameter 2 that is read from hardware
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the requested paramter type is successfully read
**/
uint32_t val_exerciser_get_param(EXERCISER_PARAM_TYPE type, uint64_t *value1, uint64_t *value2, uint32_t instance)
{
    return pal_exerciser_get_param(type, value1, value2, instance);
}

/**
  @brief   This API sets the state of the PCIe stimulus generation hardware
  @param   state        - State that needs to be set for the stimulus hadrware
  @param   value        - Additional information associated with the state
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the input state is successfully written to hardware
**/
uint32_t val_exerciser_set_state(EXERCISER_STATE state, uint64_t *value, uint32_t instance)
{
    return pal_exerciser_set_state(state, value, instance);
}

/**
  @brief   This API obtains the state of the PCIe stimulus generation hardware
  @param   state        - State that is read from the stimulus hadrware
  @param   value        - Additional information associated with the state
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the state is successfully read from hardware
**/
uint32_t val_exerciser_get_state(EXERCISER_STATE state, uint64_t *value, uint32_t instance)
{
    return pal_exerciser_get_state(state, value, instance);
}

/**
  @brief   This API performs the input operation using the PCIe stimulus generation hardware
  @param   ops          - Operation thta needs to be performed with the stimulus hadrware
  @param   value        - Additional information to perform the operation
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the operation is successfully performed using the hardware
**/
uint32_t val_exerciser_do_ops(EXERCISER_OPS ops, uint64_t param, uint32_t instance)
{
    return pal_exerciser_do_ops(ops, param, instance);
}
