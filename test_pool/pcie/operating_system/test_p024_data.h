/** @file
 * Copyright (c) 2019,2023-2024, Arm Limited or its affiliates. All rights reserved.
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

#include "val/sbsa/include/sbsa_acs_pcie.h"

/**
* The test table covers bit-field entries for device capabilities register
* belonging to capability id 10h (PCIe capability structure)
**/

pcie_cfgreg_bitfield_entry bf_info_table24[] = {

    // Bit-field entry 1: Device Capabilities Register, bit[3:4] Phantom Functions Supported
    // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x04,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP | iEP_RP),        // Applicable to Endpoints and RCEC Functions
       3,                                       // Start bit position
       4,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "WARNING : PFS value mismatch",          // PFS invalid configured value
       "WARNING : PFS attribute mismatch"       // PFS invalid attribute
    },

    // Bit-field entry 2: Device Capabilities Register, bit[5] Extended tag field support
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x04,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP | iEP_RP),        // Applicable to Endpoints and RCEC Functions
       5,                                       // Start bit position
       5,                                       // End bit position
       1,                                       // Hardwired to 1b
       READ_ONLY,                               // Attribute is Read-only
       "ETFS value mismatch",                   // ETFS invalid configured value
       "ETFS attribute mismatch"                // ETFS invalid attribute
    },

    // Bit-field entry 3: Device Capabilities Register, bit[6:8] Endpoint L0S Acceptable Latency
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x04,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable only for RP's
       6,                                       // Start bit position
       8,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "ELAL value mismatch",                   // ELAL invalid configured value
       "ELAL attribute mismatch"                // ELAL invalid attribute
    },

    // Bit-field entry 4: Device Capabilities Register, bit[9:11] Endpoint L1 Acceptable Latency
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x04,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable only for RP's
       9,                                       // Start bit position
       11,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "ELAL value mismatch",                   // ELAL invalid configured value
       "ELAL attribute mismatch"                // ELAL invalid attribute
    },

    // Bit-field entry 5: Device Capabilities Register, bit[15] Role based error reporting
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x04,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP | iEP_RP),        // Applicable to all onchip peripherals
       15,                                      // Start bit position
       15,                                      // End bit position
       1,                                       // Hardwired to 1b
       READ_ONLY,                               // Attribute is Read-only
       "RBER value mismatch",                   // RBER invalid configured value
       "RBER attribute mismatch"                // RBER invalid attribute
    },

    // Bit-field entry 6: Device Capabilities Register, bit[18:25] Captured Slot Power Limit Value
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x04,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP | iEP_RP),        // Applicable to all onchip peripherals
       18,                                      // Start bit position
       25,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "CSPLV value mismatch",                  // CSPLV invalid configured value
       "CSPLV attribute mismatch"               // CSPLV invalid attribute
    },

    // Bit-field entry 7: Device Capabilities Register, bit[26:27] Captured Slot Power Limit Scale
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x04,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP | iEP_RP),        // Applicable to all onchip peripherals
       26,                                      // Start bit position
       27,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "CSPLS value mismatch",                  // CSPLS invalid configured value
       "CSPLS attribute mismatch"               // CSPLS invalid attribute
    },

    // Bit-field entry 8: Device Capabilities Register, bit[28] Functions Level Reset Capability
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x04,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to Rootports
       28,                                      // Start bit position
       28,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "FLRC value mismatch",                   // FLRC invalid configured value
       "FLRC attribute mismatch"                // FLRC invalid attribute
    },
};
