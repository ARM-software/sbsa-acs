/** @file
 * Copyright (c) 2019, Arm Limited or its affiliates. All rights reserved.
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
* The test table covers bit-field entries for power management capabilities
* register belonging to capability id 01h (PCI power management capability
* structure)
**/

pcie_cfgreg_bitfield_entry bf_info_table28[] = {

    // Bit-field entry 1: Power Management Capabilities Register, bit[19] PME Clock
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x01,                                    // Capability id
       0,                                       // Not applicable
       0,                                       // Offset from capability id base
       PCIe_ALL,                                // Applicable to all PCIe devices
       19,                                      // Start bit position
       19,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is READ_ONLY
       "PME Clock value mismatch",              // PME Clock invalid configured value
       "PME Clock attribute mismatch"           // PME Clock invalid attribute
    },

    // Bit-field entry 2: Power Management Capabilities Register, bit[22:24] Aux Current
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x01,                                    // Capability id
       0,                                       // Not applicable
       0,                                       // Offset from capability id base
       (RCiEP | RCEC | iEP_EP | iEP_RP),        // Applicable to onchip peripherals
       22,                                      // Start bit position
       24,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is READ_ONLY
       "Aux Current value mismatch",            // Aux Current invalid configured value
       "Aux Current attribute mismatch"         // Aux Current invalid attribute
    },
};
