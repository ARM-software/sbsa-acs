/** @file
 * Copyright (c) 2020, Arm Limited or its affiliates. All rights reserved.
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


#include "include/pal_pcie_enum.h"
#include "include/pal_common_support.h"

/**
  @brief  Provides a single point of abstraction to read from all
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 32-bit data read from the input address
**/
uint32_t
pal_mmio_read(uint64_t addr)
{

  uint32_t data;

  if (addr & 0x3) {
      addr = addr & ~(0x3);  //make sure addr is aligned to 4 bytes
  }

  data = (*(volatile uint32_t *)addr);
  print(AVS_PRINT_INFO, " pal_mmio_read Address = %8x  Data = %x \n", addr, data);

  return data;

}

/**
  @brief  Provides a single point of abstraction to write to all
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  32-bit data to write to address

  @return None
**/
void
pal_mmio_write(uint64_t addr, uint32_t data)
{

  if (addr & 0x3) {
      print(AVS_PRINT_WARN, "\n  Error-Input address is not aligned. Masking the last 2 bits \n");
      addr = addr & ~(0x3);  //make sure addr is aligned to 4 bytes
  }

  print(AVS_PRINT_INFO, " pal_mmio_write Address = %8x  Data = %x \n", addr, data);
  *(volatile uint32_t *)addr = data;

}
