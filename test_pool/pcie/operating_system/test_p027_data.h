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
* The test table covers bit-field entries for device control 2 register
* belonging to capability id 10h (PCIe capability structure)
**/

pcie_cfgreg_bitfield_entry bf_info_table27[] = {

    // Bit-field entry 1: Device Control Register 2, bit[5] ARI Forwarding Enable
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x28,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP),                 // Applicable to RCEC, RCiEP and iEP_EP
       5,                                       // Start bit position
       5,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is READ_ONLY
       "AFI value mismatch",                    // AFE invalid configured value
       "AFI attribute mismatch"                 // AFE invalid attribute
    },

    // Bit-field entry 2: Device Control Register 2, bit[7] Atomicop Egress Blocking
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x28,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP),                 // Applicable to RCEC, RCiEP and iEP_EP
       7,                                       // Start bit position
       7,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is READ_ONLY
       "AEB value mismatch",                    // AEB invalid configured value
       "AEB attribute mismatch"                 // AEB invalid attribute
    },

    // Bit-field entry 3: Device Control Register 2, bit[8] IDO request Enable
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x28,                                    // Offset from capability id base
       RCEC,                                    // Applicable to RCEC
       8,                                       // Start bit position
       8,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is READ_ONLY
       "IRE value mismatch",                    // IRE invalid configured value
       "IRE attribute mismatch"                 // IRE invalid attribute
    },

    // Bit-field entry 4: Device Control Register 2, bit[9] IDO Completer Enable
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x28,                                    // Offset from capability id base
       RCEC,                                    // Applicable to RCEC
       9,                                       // Start bit position
       9,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is READ_ONLY
       "ICE value mismatch",                    // ICE invalid configured value
       "ICE attribute mismatch"                 // ICE invalid attribute
    },

    // Bit-field entry 5: Device Control Register 2, bit[11] Emergency Power Reduction request
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x28,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_RP | iEP_EP),        // Applicable to RCEC, RCiEP, iEP_RP, iEP_EP
       11,                                      // Start bit position
       11,                                      // End bit position
       0,                                       // Hardwired to 0b
       RSVDP_RO,                                // Attribute is RSDVP
       "EPPR value mismatch",                   // EPPR invalid configured value
       "EPPR attribute mismatch"                // EPPR invalid attribute
    },
};
