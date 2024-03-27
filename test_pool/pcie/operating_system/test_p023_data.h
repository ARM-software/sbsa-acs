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
* The test table covers bit-field entries for PCIe capabilities register
* belonging to capability id 10h (PCIe capability structure)
**/

pcie_cfgreg_bitfield_entry bf_info_table23[] = {

    // Bit-field entry 1: PCI Express Capabilities Register, bit[8] Slot Implemented
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x02,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to Rootports
       8,                                       // Start bit position
       8,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "SI value mismatch",                     // SI invalid configured value
       "SI attribute mismatch"                  // SI invalid attribute
    },
};
