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

#ifndef __PAL_PMU_H__
#define __PAL_PMU_H__

/* Not defined APMT related structures and defines in EDK2,
 * Remove this header once EDK2 supports APMT */

typedef enum {
  PMU_EVENT_IB_TOTAL_BW,        /* Inbound total bandwidth     */
  PMU_EVENT_OB_TOTAL_BW,        /* Outbound total bandwidth    */
  PMU_EVENT_IB_READ_BW,         /* Inbound read bandwidth      */
  PMU_EVENT_IB_WRITE_BW,        /* Inbound write bandwidth     */
  PMU_EVENT_OB_READ_BW,         /* Outbound read bandwidth     */
  PMU_EVENT_OB_WRITE_BW,        /* Outbound write bandwidth    */
  PMU_EVENT_IB_OPEN_TXN,        /* Inbound open transactions   */
  PMU_EVENT_IB_TOTAL_TXN,       /* Inbound total transactions  */
  PMU_EVENT_OB_OPEN_TXN,        /* Outbound open transactions  */
  PMU_EVENT_OB_TOTAL_TXN,       /* Outbound total transactions */
  PMU_EVENT_LOCAL_BW,           /* Local traffic bandwidth     */
  PMU_EVENT_REMOTE_BW,          /* Remote traffic bandwidth    */
  PMU_EVENT_ALL_BW,             /* All traffic bandwidth       */
  PMU_EVENT_TRAFFIC_1,          /* traffic type 1 */
  PMU_EVENT_TRAFFIC_2           /* traffic type 2 */
} PMU_EVENT_TYPE_e;

typedef enum {
  PMU_NODE_MEM_CNTR,
  PMU_NODE_SMMU,
  PMU_NODE_PCIE_RC,
  PMU_NODE_ACPI_DEVICE,
  PMU_NODE_PE_CACHE
} PMU_NODE_INFO_TYPE;

#define PMU_EVENT_INVALID 0xFFFFFFFF

#endif
