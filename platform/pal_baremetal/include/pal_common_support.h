/** @file
 * Copyright (c) 2020, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __PAL_COMMON_SUPPORT_H_
#define __PAL_COMMON_SUPPORT_H_

#include <stdio.h>
#include <stdint.h>

extern uint32_t g_print_level;

#define AVS_PRINT_ERR   5      /* Only Errors. use this to de-clutter the terminal and focus only on specifics */
#define AVS_PRINT_WARN  4      /* Only warnings & errors. use this to de-clutter the terminal and focus only on specifics */
#define AVS_PRINT_TEST  3      /* Test description and result descriptions. THIS is DEFAULT */
#define AVS_PRINT_DEBUG 2      /* For Debug statements. contains register dumps etc */
#define AVS_PRINT_INFO  1      /* Print all statements. Do not use unless really needed */

#define PCIE_EXTRACT_BDF_SEG(bdf)  ((bdf >> 24) & 0xFF)
#define PCIE_EXTRACT_BDF_BUS(bdf)  ((bdf >> 16) & 0xFF)
#define PCIE_EXTRACT_BDF_DEV(bdf)  ((bdf >> 8) & 0xFF)
#define PCIE_EXTRACT_BDF_FUNC(bdf) (bdf & 0xFF)

#define PCIE_CFG_SIZE  4096

#define PCIE_MAX_BUS   256
#define PCIE_MAX_DEV    32
#define PCIE_MAX_FUNC    8

#define print(verbose, string, ...)  if(verbose >= g_print_level) \
                                                   printf(string, ##__VA_ARGS__)

#define PCIE_CREATE_BDF(Seg, Bus, Dev, Func) ((Seg << 24) | (Bus << 16) | (Dev << 8) | Func)
#define CLEAN_AND_INVALIDATE  0x1
#define CLEAN                 0x2
#define INVALIDATE            0x3


typedef struct {
  uint64_t   Arg0;
  uint64_t   Arg1;
  uint64_t   Arg2;
  uint64_t   Arg3;
  uint64_t   Arg4;
  uint64_t   Arg5;
  uint64_t   Arg6;
  uint64_t   Arg7;
} ARM_SMC_ARGS;

typedef struct {
  uint32_t num_of_pe;
} PE_INFO_HDR;

/**
  @brief  structure instance for PE entry
**/
typedef struct {
  uint32_t   pe_num;    ///< PE Index
  uint32_t   attr;      ///< PE attributes
  uint64_t   mpidr;     ///< PE MPIDR
  uint32_t   pmu_gsiv;  ///< PMU Interrupt ID
} PE_INFO_ENTRY;

typedef struct {
  PE_INFO_HDR    header;
  PE_INFO_ENTRY  pe_info[];
} PE_INFO_TABLE;

void pal_pe_data_cache_ops_by_va(uint64_t addr, uint32_t type);

typedef struct {
  uint64_t   ecam_base;     ///< ECAM Base address
  uint64_t   bar_start_addr; ///< ECAM BAR start address
  uint32_t   segment_num;   ///< Segment number of this ECAM
  uint32_t   start_bus_num; ///< Start Bus number for this ecam space
  uint32_t   end_bus_num;   ///< Last Bus number
} PCIE_INFO_BLOCK;

typedef struct {
  uint32_t  num_entries;
  PCIE_INFO_BLOCK block[];
} PCIE_INFO_TABLE;


#endif
