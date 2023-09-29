/** @file
 * Copyright (c) 2016-2023, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __SBSA_AVS_LEVEL_H__
#define __SBSA_AVS_LEVEL_H__

  #define SIZE_4K 0x100

  #define SBSA_ACS_MAJOR_VER     7
  #define SBSA_ACS_MINOR_VER     1
  #define SBSA_ACS_SUBMINOR_VER  3

  #define SBSA_MIN_LEVEL_SUPPORTED 3
  #define SBSA_MAX_LEVEL_SUPPORTED 7

  #ifdef _AARCH64_BUILD_
  unsigned long __stack_chk_guard = 0xBAAAAAAD;
  unsigned long __stack_chk_fail =  0xBAAFAAAD;
  #endif

#endif
