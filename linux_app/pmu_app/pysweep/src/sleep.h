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
#ifndef __included_sleep_h
#define __included_sleep_h

/*
 * Wait for a high-resolution amount of time, handling EINTR.
 * The standard way is to use select() or pselect(). The underlying
 * system call adjusts the time argument to show how much time remains,
 * but this is hidden by the library. So to know how long we actually
 * slept we have to read the current time, and iterate.
 */

/* For glibc versions before 2.17, link with -lrt */

/*
 * Result is a negative return code (with errno set)
 * or a non-negative count of the number of times we had to iterate.
 */

int microsleep_ns(long long);

int microsleep(double);

#endif
