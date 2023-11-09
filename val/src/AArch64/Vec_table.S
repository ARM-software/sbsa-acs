/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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

  .section .bss.tt, "aw"
  .align 12
  .global tt_l0_base
tt_l0_base:
  .fill  4096

  .align 12
  .global tt_l1_base
tt_l1_base:
  .fill 4096

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_1
tt_l2_base_1:
  .fill 16384

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_2
tt_l2_base_2:
  .fill 16384

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_3
tt_l2_base_3:
  .fill 16384

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_4
tt_l2_base_4:
  .fill 16384

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_5
tt_l2_base_5:
  .fill 16384

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_6
tt_l2_base_6:
  .fill 16384

   .align 12
 // Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_1
tt_l3_base_1:
  .fill 16384

  .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_2
tt_l3_base_2:
  .fill 16384

  .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_3
tt_l3_base_3:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_4
tt_l3_base_4:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_5
tt_l3_base_5:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_6
tt_l3_base_6:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_7
tt_l3_base_7:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_8
tt_l3_base_8:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_9
tt_l3_base_9:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_10
tt_l3_base_10:
  .fill 16384
