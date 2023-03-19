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

/*
 * Detect if the architecture being compiled for is any ARM-based architecture
 * and define a macro to indicate ISA if so.
 */

#ifndef __included_arch_h
#define __included_arch_h

#if defined(__arm64__) || defined(__AARCH64EL__) || defined(__ARM_ARCH_ISA_A64)
#define ARCH_A64        /* meaning the instruction set */
#define ARCH_AARCH64    /* meaning the execution mode */
#define ARCH_ARM
#elif defined(__thumb__)
#define ARCH_T32
#define ARCH_AARCH32
#define ARCH_ARM
#elif defined(__ARMEL__)
#define ARCH_A32
#define ARCH_AARCH32
#define ARCH_ARM
#endif

#endif /* included */

/* end of arch.h */
