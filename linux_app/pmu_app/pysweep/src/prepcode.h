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
 * Prepare code block for execution
 *
 * This might involve changing memory protection, synchronizing cache etc.
 */

#ifndef __included_prepcode_h
#define __included_prepcode_h

#include <stddef.h>

/*
 * Do all necessary prep for executing a block of data as code.
 */
#define PREPCODE_PROTECT   1 
#define PREPCODE_COHERENCE 2
#define PREPCODE_DEBUGGER  4   /* Notify debugger about new code */
#define PREPCODE_ALL    0xff
int prepare_code(void const *, size_t size, unsigned int flags);
int prepare_code_elf(void const *, size_t size, unsigned int flags, void const *elf, size_t elf_size);

int unprepare_code(void const *, size_t size, unsigned int flags);

/* Just change protection (if needed) */
int prepare_code_protection(void const *, size_t size);

/* Just ensure coherence (if needed) */
int prepare_code_coherence(void const *, size_t size);

#endif /* included */

