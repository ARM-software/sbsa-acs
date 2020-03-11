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
* The test table covers bit-field entries for power management control/status
* register belonging to capability id 01h (PCI power management capability
* structure)
**/

pcie_cfgreg_bitfield_entry bf_info_table29[] = {

    // Bit-field entry 1: Power Management Capabilities/Status Register, bit[9:12] Data Select
    // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x01,                                    // Capability id
       0,                                       // Not applicable
       0x04,                                    // Offset from capability id base
       (RCiEP | RCEC | iEP_EP | iEP_RP),        // Applicable to onchip peripherals
       9,                                       // Start bit position
       12,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is READ_ONLY
       "WARNING Data Select value mismatch",    // Data Select invalid configured value
       "WARNING Data Select attribute mismatch" // Data Select invalid attribute
    },

    // Bit-field entry 2: Power Management Capabilities/Status Register, bit[13:14] Data Scale
    // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x01,                                    // Capability id
       0,                                       // Not applicable
       0x04,                                    // Offset from capability id base
       (RCiEP | RCEC | iEP_EP | iEP_RP),        // Applicable to onchip peripherals
       13,                                      // Start bit position
       14,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is READ_ONLY
       "WARNING Data Scale value mismatch",     // Data Scale invalid configured value
       "WARNING Data Scale attribute mismatch"  // Data Scale invalid attribute
    },
};
