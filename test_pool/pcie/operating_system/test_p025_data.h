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
* The test table covers bit-field entries for device control register
* belonging to capability id 10h (PCIe capability structure)
**/

pcie_cfgreg_bitfield_entry bf_info_table25[] = {

    // Bit-field entry 1: Device Control Register, bit[9] Phantom Functions Enable
    // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x08,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP | iEP_RP),        // Applicable to onchip peripherals
       9,                                       // Start bit position
       9,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_WRITE,                              // Attribute is Read-Write
       "WARNING PFE value mismatch",            // PFE invalid configured value
       "WARNING PFE attribute mismatch"         // PFE invalid attribute
    },

    // Bit-field entry 2: Device Control Register, bit[10] Aux power PM Enable
    // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x08,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP | iEP_RP),        // Applicable to onchip peripherals
       10,                                      // Start bit position
       10,                                      // End bit position
       0,                                       // Hardwired to 0b
       STICKY_RW,                               // Attribute is Read-Write
       "WARNING APPE value mismatch",           // APPE invalid configured value
       "WARNING APPE attribute mismatch"        // APPE invalid attribute
    },

    // Bit-field entry 3: Device Control Register, bit[11] Enable No Snoop
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x08,                                    // Offset from capability id base
       RCEC,                                    // Applicable to RCEC
       11,                                      // Start bit position
       11,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-Only
       "ENS value mismatch",                    // ENS invalid configured value
       "ENS attribute mismatch"                 // ENS invalid attribute
    },

    // Bit-field entry 4: Device Control Register, bit[15] Initiate FLR
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x08,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to Rootports
       15,                                      // Start bit position
       15,                                      // End bit position
       0,                                       // Hardwired to 0b
       RSVDP_RO,                                // Attribute is rsvdp
       "IFLR value mismatch",                   // IFLR invalid configured value
       "IFLR attribute mismatch"                // IFLR invalid attribute
    },
};
