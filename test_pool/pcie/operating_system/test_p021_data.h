/** @file
 * Copyright (c) 2019,2023 Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_pcie.h"

/**
* The test table covers bit-field entries for registers
* which are applicable for only type0 header
**/

pcie_cfgreg_bitfield_entry bf_info_table21[] = {

    // Bit-field entry 1: CardBus CIS Pointer, bit[0:31] = cardbus cis pointer
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x28,                                    // Offset from ECAM base
       (RCiEP | RCEC  | iEP_EP),                // Applicable to Endpoints and RCEC Functions
       0,                                       // Start bit position
       31,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "CCP value mismatch",                    // CardBus CIS pointer invalid configured value
       "CCP attribute mismatch"                 // CardBus CIS pointer invalid attribute
    },

    // Bit-field entry 2: Min Grant, bit[0:7] = Min Gnt
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x3E,                                    // Offset from ECAM base
       (RCiEP | RCEC | iEP_EP),                 // Applicable to Endpoints and RCEC Functions
       0,                                       // Start bit position
       7,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "MinGnt value mismatch",                 // MinGnt invalid configured value
       "MinGnt attribute mismatch"              // MinGnt invalid attribute
    },

    // Bit-field entry 3: Max Latency, bit[0:7] = Max latency
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x3F,                                    // Offset from ECAM base
       (RCiEP | RCEC | iEP_EP),                 // Applicable to Endpoints and RCEC Functions
       0,                                       // Start bit position
       7,                                       // End bit position
       0,                                       // Hardwired to 0b
       RSVDP_RO,                                // Attribute is Read-only
       "Max latency value mismatch",            // Max latency invalid configured value
       "Max latency attribute mismatch"         // Max latency invalid attribute
    },
};
