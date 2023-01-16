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

/* Private header for load generation */

#ifndef __included_loadgenp_h
#define __included_loadgenp_h

#include "loadgen.h"

#include <stdio.h>
#include <stddef.h>

extern int workload_verbose;

extern void *load_alloc_mem(struct workload_mem *);

extern void load_free_mem(struct workload_mem *);

extern void *load_construct_code(Workload *);

extern void load_free_code(Workload *);

extern void *load_construct_data(Character const *, struct workload_mem *);

#ifdef __cplusplus
template<typename T>
inline T round_size(T size, unsigned int granule)
{
    return (size + (granule-1)) & ~(T)(granule-1);
}
#else
/* Round the size by ANDing with a suitable constant.
   The constant must be as wide as the size. */
#define round_size(size, granule) ((size + ((granule)-1)) & ~((size)*0+(granule)-1))
#endif

extern void fprint_mem(FILE *, void const *, size_t);

extern void fprint_code(FILE *, void const *, size_t);

#endif /* included */
