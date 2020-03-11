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
* The test table covers bit-field entries for device capabilities 2 register
* belonging to capability id 10h (PCIe capability structure)
**/

pcie_cfgreg_bitfield_entry bf_info_table26[] = {

    // Bit-field entry 1: Device Capabilities Register 2, bit[5] ARI Forwarding Support
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP | EP),            // Applicable to Endpoints, RCEC and RCiEP
       5,                                       // Start bit position
       5,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "AFS value mismatch",                    // AFS invalid configured value
       "AFS attribute mismatch"                 // AFS invalid attribute
    },

    // Bit-field entry 2: Device Capabilities Register 2, bit[5] ARI Forwarding Support
    // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to iEP_RP
       5,                                       // Start bit position
       5,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "WARNING AFS value mismatch",            // AFS invalid configured value
       "WARNING AFS attribute mismatch"         // AFS invalid attribute
    },

    // Bit-field entry 3: Device Capabilities Register 2, bit[6] AtomicOp Routing Supported
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP),                 // Applicable to integrated Endpoints, RCEC
       6,                                       // Start bit position
       6,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "ARS value mismatch",                    // ARS invalid configured value
       "ARS attribute mismatch"                 // ARS invalid attribute
    },

    // Bit-field entry 4: Device Capabilities Register 2, bit[6] AtomicOp Routing Supported
    // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to iEP_RP
       6,                                       // Start bit position
       6,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "WARNING ARS value mismatch",            // ARS invalid configured value
       "WARNING ARS attribute mismatch"         // ARS invalid attribute
    },

    // Bit-field entry 5: Device Capabilities Register 2, bit[10] No RO-enabled PR-PR passing
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       (RCEC | RCiEP | iEP_EP | EP),            // Applicable to integrated endpoint pair
       10,                                      // Start bit position
       10,                                      // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW INIT
       "NREPP value mismatch",                  // NREPP invalid configured value
       "NREPP attribute mismatch"               // NREPP invalid attribute
    },


    // Bit-field entry 6: Device Capabilities Register 2, bit[14:15] LN System CLS
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       (RCiEP | iEP_EP | EP),                   // Applicable to RCiEP, iEP_EP and EP
       14,                                      // Start bit position
       15,                                      // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW INIT
       "LSC value mismatch",                    // LSC invalid configured value
       "LSC attribute mismatch"                 // LSC invalid attribute
    },

    // Bit-field entry 7: Device Capabilities Register 2, bit[16] 10-bit tag completer supported
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to iEP_RP
       16,                                      // Start bit position
       16,                                      // End bit position
       1,                                       // Hardwired to 1b
       HW_INIT,                                 // Attribute is HW INIT
       "TCS value mismatch",                    // TCS invalid configured value
       "TCS attribute mismatch"                 // TCS invalid attribute
    },

    // Bit-field entry 8: Device Capabilities Register 2, bit[17] 10-bit tag requester supported
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       (iEP_RP | iEP_EP),                       // Applicable to integrated pair
       17,                                      // Start bit position
       17,                                      // End bit position
       1,                                       // Hardwired to 1b
       HW_INIT,                                 // Attribute is HW INIT
       "TRS value mismatch",                    // TRS invalid configured value
       "TRS attribute mismatch"                 // TRS invalid attribute
    },

    // Bit-field entry 9: Device Capabilities Register 2, bit[20] Extended Fmt Field Supported
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       (iEP_RP | iEP_EP),                       // Applicable to integrated pair
       20,                                      // Start bit position
       20,                                      // End bit position
       1,                                       // Hardwired to 1b
       READ_ONLY,                               // Attribute is READ_ONLY
       "EFFS value mismatch",                   // EFFS invalid configured value
       "EFFS attribute mismatch"                // EFFS invalid attribute
    },

    // Bit-field entry 10: Device Capabilities Register 2, bit[20] Extended Fmt Field Supported
    // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       (RCiEP | RCEC),                          // Applicable to RCEC and RCiEP
       20,                                      // Start bit position
       20,                                      // End bit position
       1,                                       // Hardwired to 1b
       READ_ONLY,                               // Attribute is READ_ONLY
       "WARNING EFFS value mismatch",           // EFFS invalid configured value
       "WARNING EFFS attribute mismatch"        // EFFS invalid attribute
    },

    // Bit-field entry 11: Device Capabilities Register 2, bit[21] End-End TLP Prefix Supported
    // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       RCEC,                                    // Applicable to RCEC
       21,                                      // Start bit position
       21,                                      // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "WARNING ETPS value mismatch",           // ETPS invalid configured value
       "WARNING ETPS attribute mismatch"        // ETPS invalid attribute
    },

    // Bit-field entry 12: Device Capabilities Register 2, bit[22:23] Max End-End TLP Prefixes
    // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       RCEC,                                    // Applicable to RCEC
       22,                                      // Start bit position
       23,                                      // End bit position
       0,                                       // Hardwired to 0b
       RSVDP_RO,                                // Attribute is RSVDP
       "WARNING METP value mismatch",           // METP invalid configured value
       "WARNING METP attribute mismatch"        // METP invalid attribute
    },

    // Bit-field entry 13: Device Cap Register 2, bit[24:25] Emergency power reduction supported
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to iEP_RP
       24,                                      // Start bit position
       25,                                      // End bit position
       0,                                       // Hardwired to 0b
       RSVDP_RO,                                // Attribute is RSVDP
       "EPRS value mismatch",                   // EPRS invalid configured value
       "EPRS attribute mismatch"                // EPRS invalid attribute
    },

   // Bit-field entry 14: Device Cap Register 2, bit[24:25] Emergency power reduction supported
   // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       iEP_EP,                                  // Applicable to iEP_EP
       24,                                      // Start bit position
       25,                                      // End bit position
       0,                                       // Hardwired to 0b
       RSVDP_RO,                                // Attribute is RSVDP
       "WARNING EPRS value mismatch",           // EPRS invalid configured value
       "WARNING EPRS attribute mismatch"        // EPRS invalid attribute
    },

    // Bit-field entry 15: Device Cap Register 2, bit[26] Emergency power reduction init required
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to iEP_RP
       26,                                      // Start bit position
       26,                                      // End bit position
       0,                                       // Hardwired to 0b
       RSVDP_RO,                                // Attribute is RSVDP
       "EPRIR value mismatch",                  // EPRIR invalid configured value
       "EPRIR attribute mismatch"               // EPRIR invalid attribute
    },

    // Bit-field entry 16: Device Cap Register 2, bit[26] Emergency power reduction init required
    // WARNING
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x24,                                    // Offset from capability id base
       iEP_EP,                                  // Applicable to iEP_EP
       26,                                      // Start bit position
       26,                                      // End bit position
       0,                                       // Hardwired to 0b
       RSVDP_RO,                                // Attribute is RSVDP
       "WARNING EPRIR value mismatch",          // EPRIR invalid configured value
       "WARNING EPRIR attribute mismatch"       // EPRIR invalid attribute
    },
};
