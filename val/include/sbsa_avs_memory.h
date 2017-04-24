/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

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

#ifndef __SBSA_AVS_MEMORY_H__
#define __SBSA_AVS_MEMORY_H__

addr_t val_memory_ioremap(void *addr, uint32_t size, uint64_t attr);

void val_memory_unmap(void *ptr);


#endif // __SBSA_AVS_PERIPHERAL_H__
