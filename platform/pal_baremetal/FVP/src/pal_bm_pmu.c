/** @file
 * Copyright (c) 2023 Arm Limited or its affiliates. All rights reserved.
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


/**
  @brief   This API checks if pmu monitor count value is valid
  @param   interface_acpiid - acpiid of interface
  @param   count_value - monitor count value
  @param   eventid - eventid
  @return  0 - monitor count value is valid
           non-zero - error or invallid count value
**/
uint32_t
pal_pmu_check_monitor_count_value(uint64_t interface_acpiid, uint32_t count_value, uint32_t eventid)
{
    return NOT_IMPLEMENTED;
}

/**
  @brief   This API generates required workload for given pmu node and event id
  @param   interface_acpiid - acpiid of interface
  @param   pmu_node_index - pmu node index
  @param   mon_index - monitor index
  @param   eventid - eventid
  @return  0 - success status
           non-zero - error status
**/
uint32_t
pal_generate_traffic(uint64_t interface_acpiid, uint32_t pmu_node_index,
                                     uint32_t mon_index, uint32_t eventid)
{
    return NOT_IMPLEMENTED;
}

/**
  @brief   This API checks if PMU node is secure only.
  @param   interface_acpiid - acpiid of interface
  @param   num_traffic_type_support - num of traffic type supported.
  @return  0 - success status
           non-zero - error status
**/
uint32_t
pal_pmu_get_multi_traffic_support_interface(uint64_t *interface_acpiid,
                                                       uint32_t *num_traffic_type_support)
{
    return NOT_IMPLEMENTED;
}
