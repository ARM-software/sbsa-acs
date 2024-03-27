/** @file
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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
  The test table covers bit-field entries for Slot Capabilites, Slot Control
  and Slot Status registers in PCI Capability Structure (cap10) and are
  specific to Root Ports.
**/

pcie_cfgreg_bitfield_entry bf_info_table63[] = {

    // Bit-field entry 1: Slot Capabilities, bit 0 - Attention Button Present
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       0,                                       // Start bit position
       0,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap ABP value mismatch",           // ABP invalid configured value
       "Slot Cap ABP attribute mismatch"        // ABP invalid attribute
    },

    // Bit-field entry 2: Slot Capabilities, bit 1 - Power Controllor Present
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       1,                                       // Start bit position
       1,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap PCP value mismatch",           // PCP invalid configured value
       "Slot Cap PCP attribute mismatch"        // PCP invalid attribute
    },

    // Bit-field entry 3: Slot Capabilities, bit 2 - MRL Sensor Present
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       2,                                       // Start bit position
       2,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap MRL SP value mismatch",        // MRL SP invalid configured value
       "Slot Cap MRL SP attribute mismatch"     // MRL SP invalid attribute
    },

    // Bit-field entry 4: Slot Capabilities, bit 3 - Attention Indicator Present
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       3,                                       // Start bit position
       3,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap AIP value mismatch",           // AIP invalid configured value
       "Slot Cap AIP attribute mismatch"        // AIP invalid attribute
    },

    // Bit-field entry 5: Slot Capabilities, bit 4 - Power Indicator Present
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       4,                                       // Start bit position
       4,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap PIP value mismatch",           // PIP invalid configured value
       "Slot Cap PIP attribute mismatch"        // PIP invalid attribute
    },

    // Bit-field entry 6: Slot Capabilities, bit 5 - Hot-Plug Suprise
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       5,                                       // Start bit position
       5,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap HPS value mismatch",           // HPS invalid configured value
       "Slot Cap HPS attribute mismatch"        // HPS invalid attribute
    },

    // Bit-field entry 7: Slot Capabilities, bit 6 - Hot-Plug Capable
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       6,                                       // Start bit position
       6,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap HPC value mismatch",           // HPC invalid configured value
       "Slot Cap HPC attribute mismatch"        // HPC invalid attribute
    },

    // Bit-field entry 8: Slot Capabilities, bits [7:14] - Slot Power Limit Value
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       7,                                       // Start bit position
       14,                                      // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap SPLV value mismatch",          // SPLV invalid configured value
       "Slot Cap SPLV attribute mismatch"       // SPLV invalid attribute
    },

    // Bit-field entry 9: Slot Capabilities, bits [15:16] - Slot Power Limit Scale
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       15,                                      // Start bit position
       16,                                      // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap SPLS value mismatch",          // SPLS invalid configured value
       "Slot Cap SPLS attribute mismatch"       // SPLS invalid attribute
    },

    // Bit-field entry 10: Slot Capabilities, bit 17 - Electromechanical Interlock Present
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       17,                                      // Start bit position
       17,                                      // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap EIP value mismatch",           // EIP invalid configured value
       "Slot Cap EIP attribute mismatch"        // EIP invalid attribute
    },

    // Bit-field entry 11: Slot Capabilities, bit 18 - No Command Completed Support
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       18,                                      // Start bit position
       18,                                      // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap NCCS value mismatch",          // NCCS invalid configured value
       "Slot Cap NCCS attribute mismatch"       // NCCS invalid attribute
    },

    // Bit-field entry 12: Slot Capabilities, bits [19:31] - Physical Slot Number
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x14,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       19,                                      // Start bit position
       31,                                      // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Cap PSN value mismatch",           // PSN invalid configured value
       "Slot Cap PSN attribute mismatch"        // PSN invalid attribute
    },

    /* Slot control register bits have Read-Write attributes by default. But according to the spec,
       as Slot is not implemented off of any iEP_RP, slot control bits are hardwired to 0b.
       So write access to these bits are forbidden.
    */

    // Bit-field entry 13: Slot Control, bit 0 - Attention Button Pressed Enable
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x18,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       0,                                       // Start bit position
       0,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Control ABPE value mismatch",      // ABPE invalid configured value
       "Slot Control ABPE attribute mismatch"   // ABPE invalid attribute
    },

    // Bit-field entry 14: Slot Control, bit 1 - Power Fault Detected Enable
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x18,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       1,                                       // Start bit position
       1,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Control PFDE value mismatch",      // PFDE invalid configured value
       "Slot Control PFDE attribute mismatch"   // PFDE invalid attribute
    },

    // Bit-field entry 15: Slot Control, bit 2 - MRL Sensor Changed Enable
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x18,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       2,                                       // Start bit position
       2,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Control MSCE value mismatch",      // MSCE invalid configured value
       "Slot Control MSCE attribute mismatch"   // MSCE invalid attribute
    },

    // Bit-field entry 16: Slot Control, bit 3 - Presence Detect Changed Enable
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x18,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       3,                                       // Start bit position
       3,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Control PDCE value mismatch",      // PDCE invalid configured value
       "Slot Control PDCE attribute mismatch"   // PDCE invalid attribute
    },

    // Bit-field entry 17: Slot Control, bit 4 - Command Completed Interrupt Enable
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x18,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       4,                                       // Start bit position
       4,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Control CCIE value mismatch",      // CCIE invalid configured value
       "Slot Control CCIE attribute mismatch"   // CCIE invalid attribute
    },

    // Bit-field entry 18: Slot Control, bit 5 - Hot-Plug Interrupt Enable
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x18,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       5,                                       // Start bit position
       5,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Control HPIE value mismatch",      // HPIE invalid configured value
       "Slot Control HPIE attribute mismatch"   // HPIE invalid attribute
    },

    // Bit-field entry 19: Slot Control, bits [6:7] - Attention Indicator Control
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x18,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       6,                                       // Start bit position
       7,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Control AIC value mismatch",       // AIC invalid configured value
       "Slot Control AIC attribute mismatch"    // AIC invalid attribute
    },

    // Bit-field entry 20: Slot Control, bits [8:9] - Power Indicator Control
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x18,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       8,                                       // Start bit position
       9,                                       // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Control PIC value mismatch",       // PIC invalid configured value
       "Slot Control PIC attribute mismatch"    // PIC invalid attribute
    },

    // Bit-field entry 21: Slot Control, bit 10 - Power Controller Control
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x18,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       10,                                      // Start bit position
       10,                                      // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Control PCC value mismatch",       // PCC invalid configured value
       "Slot Control PCC attribute mismatch"    // PCC invalid attribute
    },

    // Bit-field entry 22: Slot Control, bit 11 -  Electromechanical Interlock Control
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x18,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       11,                                      // Start bit position
       11,                                      // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Control EIC value mismatch",       // EIC invalid configured value
       "Slot Control EIC attribute mismatch"    // EIC invalid attribute
    },

    // Bit-field entry 23: Slot Control, bit 13 - Auto Slot Power Limit Disable
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x18,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       13,                                      // Start bit position
       13,                                      // End bit position
       0,                                       // Hardwired to 0b
       HW_INIT,                                 // Attribute is HW_INIT
       "Slot Control ASPL value mismatch",      // ASPL invalid configured value
       "Slot Control ASPL attribute mismatch"   // ASPL invalid attribute
    },

    // Bit-field entry 24: Slot Status, bit 5 - MRL Sensor State
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x1A,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       5,                                       // Start bit position
       5,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is READ_ONLY
       "Slot Status MRL SS value mismatch",     // MRL invalid configured value
       "Slot Status MRL SS attribute mismatch"  // MRL invalid attribute
    },

    // Bit-field entry 25: Slot Status, bit 6 - Presence Detect State
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x1A,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       6,                                       // Start bit position
       6,                                       // End bit position
       1,                                       // Hardwired to 1b
       READ_ONLY,                               // Attribute is READ_ONLY
       "Slot Status PDS value mismatch",        // PDS invalid configured value
       "Slot Status PDS attribute mismatch"     // PDS invalid attribute
    },

    // Bit-field entry 26: Slot Status, bit 7 - Electromechanical Interlock Status
    {
       PCIE_CAP,                                // Part of PCIe capability register
       0x10,                                    // Capability id
       0,                                       // Not applicable
       0x1A,                                    // Offset from capability id base
       iEP_RP,                                  // Applicable to integrated root ports
       7,                                       // Start bit position
       7,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is READ_ONLY
       "Slot Status EIS value mismatch",        // EIS invalid configured value
       "Slot Status EIS attribute mismatch"     // EIS invalid attribute
    },

};
