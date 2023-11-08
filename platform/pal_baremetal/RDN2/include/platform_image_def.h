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

#ifndef _PAL_CONFIG_H_
#define _PAL_CONFIG_H_

/*
 * Device Info in physical addresses.
 */
#define PLATFORM_NORMAL_WORLD_IMAGE_SIZE  0x4000000
#define PLATFORM_HOST_IMAGE_SIZE          (PLATFORM_NORMAL_WORLD_IMAGE_SIZE / 2)
#define PLATFORM_MEMORY_POOL_SIZE         (250 * 0x100000)
#define PLATFORM_SHARED_REGION_SIZE       0x100000
#define PLATFORM_HEAP_REGION_SIZE         (PLATFORM_MEMORY_POOL_SIZE \
                                             - PLATFORM_SHARED_REGION_SIZE)

/*
 * Run-time address of the ACS Non-secure image. It has to match
 * the location where the DUT software loads the ACS NS Image.
 */
#define PLATFORM_NORMAL_WORLD_IMAGE_BASE     0xE0000000
#define PLATFORM_HOST_IMAGE_BASE             PLATFORM_NORMAL_WORLD_IMAGE_BASE
#define PLATFORM_MEMORY_POOL_BASE           (PLATFORM_NORMAL_WORLD_IMAGE_BASE \
                                                 + PLATFORM_NORMAL_WORLD_IMAGE_SIZE)

#define PLATFORM_SHARED_REGION_BASE         PLATFORM_MEMORY_POOL_BASE
#define PLATFORM_HEAP_REGION_BASE           (PLATFORM_SHARED_REGION_BASE + \
                                            PLATFORM_SHARED_REGION_SIZE)

#endif /* _PAL_CONFIG_H_ */
