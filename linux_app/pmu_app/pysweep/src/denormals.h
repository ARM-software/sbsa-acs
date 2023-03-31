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
#ifndef __included_denormals_h
#define __included_denormals_h

#include <stdint.h>

/* Sample denormal values */
static double DOUBLE_DENORMAL = 7.9e-323;   /* 0x1p-1070 */
static float FLOAT_DENORMAL = 2.2e-44f;     /* 0x1p-145f */

/*
 * Check if denormals are enabled, without affecting
 * current state.
 */
extern int denormals_are_enabled(void);

/*
 * Attempt to set denormals-enabled state. Return the
 * resulting state - which can be testing to determine
 * success or failure.
 */
extern int denormals_set_enabled(int enable);

/*
 * Run some self-tests.
 */
extern void test_denormals(void);

#endif /* included */

