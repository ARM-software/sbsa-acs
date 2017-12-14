/** @file
 * Copyright (c) 2017, ARM Limited or its affiliates. All rights reserved.

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

#ifndef __SBSA_AVS_LEVEL_H__
#define __SBSA_AVS_LEVEL_H__



  #define SBSA_ACS_MAJOR_VER  1
  #define SBSA_ACS_MINOR_VER  2

  #define G_SBSA_LEVEL  3
  #define G_PRINT_LEVEL AVS_PRINT_TEST

  #define PE_INFO_TBL_SZ         8192 /*Supports maximum 400 PEs*/
  #define GIC_INFO_TBL_SZ        8192 /*Supports maximum 256 redistributors, 256 ITS blocks & 4 distributors*/
  #define TIMER_INFO_TBL_SZ      1024 /*Supports maximum 2 system timers*/
  #define WD_INFO_TBL_SZ         512  /*Supports maximum 20 Watchdogs*/
  #define MEM_INFO_TBL_SZ        4096 /*Supports maximum 110 memory regions*/
  #define IOVIRT_INFO_TBL_SZ     4096 /*Supports maximum 60 nodes of a typical iort table*/
  #define PERIPHERAL_INFO_TBL_SZ 1024 /*Supports maximum 20 PCIe EPs (USB and SATA controllers only) */
  #define PCIE_INFO_TBL_SZ       512  /*Supports maximum 20 RC's*/


#endif
