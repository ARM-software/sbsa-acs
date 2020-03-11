/** @file
 * Copyright (c) 2019-2020, Arm Limited or its affiliates. All rights reserved.
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
* which are applicable only for type1 header
**/

pcie_cfgreg_bitfield_entry bf_info_table22[] = {

    // Bit-field entry 1: Secondary Latency Timer, bit[0:7]
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x1B,                                    // Offset from ECAM base
       (RP | iEP_RP),                           // Applicable to Rootports
       0,                                       // Start bit position
       7,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "SLT value mismatch",                    // SLT invalid configured value
       "SLT attribute mismatch"                 // SLT invalid attribute
    },

    // Bit-field entry 2: Secondary Status Register, bit[5] 66 Mhz capable
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x1E,                                    // Offset from ECAM base
       (RP | iEP_RP),                           // Applicable to Rootports
       5,                                       // Start bit position
       5,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "SSR 66Mhz value mismatch",              // SSR 66Mhz invalid configured value
       "SSR 66Mhz attribute mismatch"           // SSR 66Mhz invalid attribute
    },

    // Bit-field entry 3: Secondary Status Register, bit[7] Fast Back-to-back transactions capable
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x1E,                                    // Offset from ECAM base
       (RP | iEP_RP),                           // Applicable to Rootports
       7,                                       // Start bit position
       7,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "SSR FBBTC value mismatch",              // SSR FBBTC invalid configured value
       "SSR FBBTC attribute mismatch"           // SSR FBBTC invalid attribute
    },

    // Bit-field entry 4: Secondary Status Register, bit[9:10] DEVSEL timing
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x1E,                                    // Offset from ECAM base
       (RP | iEP_RP),                           // Applicable to Rootports
       9,                                       // Start bit position
       10,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "SSR DEVSEL value mismatch",             // DEVSEL Timing invalid configured value
       "SSR DEVSEL attribute mismatch"          // DEVSEL Timing invalid attribute
    },

    // Bit-field entry 5: Bridge Control Register, bit[5] Master Abort Mode
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x3E,                                    // Offset from ECAM base
       (RP | iEP_RP),                           // Applicable to Rootports
       5,                                       // Start bit position
       5,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "BCR MAM value mismatch",                // MAM invalid configured value
       "BCR MAM attribute mismatch"             // MAM invalid attribute
    },

    // Bit-field entry 6: Bridge Control Register, bit[7] Fast Back-to-Back Transactions Enable
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x3E,                                    // Offset from ECAM base
       (RP | iEP_RP),                           // Applicable to Rootports
       7,                                       // Start bit position
       7,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "BCR FBBTE value mismatch",              // FBBTE invalid configured value
       "BCR FBBTE attribute mismatch"           // FBBTE invalid attribute
    },

    // Bit-field entry 7: Bridge Control Register, bit[8] Primary Discard Timer
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x3E,                                    // Offset from ECAM base
       (RP | iEP_RP),                           // Applicable to Rootports
       8,                                       // Start bit position
       8,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "BCR PDT value mismatch",                // PDT invalid configured value
       "BCR PDT attribute mismatch"             // PDT invalid attribute
    },

    // Bit-field entry 8: Bridge Control Register, bit[9] Secondary Discard Timer
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x3E,                                    // Offset from ECAM base
       (RP | iEP_RP),                           // Applicable to Rootports
       9,                                       // Start bit position
       9,                                       // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "BCR SDT value mismatch",                // SDT invalid configured value
       "BCR SDT attribute mismatch"             // SDT invalid attribute
    },

    // Bit-field entry 9: Bridge Control Register, bit[10] Discard Timer Status
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x3E,                                    // Offset from ECAM base
       (RP | iEP_RP),                           // Applicable to Rootports
       10,                                      // Start bit position
       10,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "BCR DTS value mismatch",                // DTS invalid configured value
       "BCR DTS attribute mismatch"             // DTS invalid attribute
    },

    // Bit-field entry 10: Bridge Control Register, bit[11] Discard Timer SERR Enable
    {
       HEADER,                                  // Part of Header type register
       0,                                       // Not applicable
       0,                                       // Not applicable
       0x3E,                                    // Offset from ECAM base
       (RP | iEP_RP),                           // Applicable to Rootports
       11,                                      // Start bit position
       11,                                      // End bit position
       0,                                       // Hardwired to 0b
       READ_ONLY,                               // Attribute is Read-only
       "BCR DTSE value mismatch",               // DTSE invalid configured value
       "BCR DTSE attribute mismatch"            // DTSE invalid attribute
    },
};
