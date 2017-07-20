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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_common.h"


/**
  @brief  Parse the input status and print the appropriate information to console
          1. Caller       - Application layer
          2. Prerequisite - None
  @param  index  - index of the PE who is reporting this status.
  @param status  - 32-bit value concatenated from state, level, error value

  @return  none
 **/
void
val_report_status(uint32_t index, uint32_t status)
{

  if (IS_TEST_FAIL(status)) {
      val_print(AVS_PRINT_ERR, "\n       Failed on PE - %4d ", index);
      val_print(AVS_PRINT_ERR, "for Level= %2d ", (status >> LEVEL_BIT) & LEVEL_MASK);
  }

  if (IS_TEST_PASS(status))
    val_print(AVS_PRINT_TEST, ": Result:  PASS \n", status);
  else
    if (IS_TEST_FAIL(status))
      val_print(AVS_PRINT_ERR, ": Result:  --FAIL-- %x \n", status & 0xFFFF);
    else
      if (IS_TEST_SKIP(status))
        val_print(AVS_PRINT_WARN, ": Result:  -SKIPPED- %x \n", status & 0xFFFF);
      else
        if (IS_TEST_START(status))
          val_print(AVS_PRINT_INFO, "         START  ", status);
        else
          if (IS_TEST_END(status))
            val_print(AVS_PRINT_INFO, "         END  \n\n", status);
          else
            val_print(AVS_PRINT_ERR, ": Result:  %8x  \n", status);

}

/**
  @brief  Record the state and status of the test execution
          1. Caller       - Test Suite
          2. Prerequisite - val_allocate_shared_mem
  @param  index  - index of the PE who is reporting this status.
  @param  status - 32-bit value concatenated from state, level, error value

  @return  none
**/
void
val_set_status(uint32_t index, uint32_t status)
{
  volatile VAL_SHARED_MEM_t *mem;

  mem = (VAL_SHARED_MEM_t *) pal_mem_get_shared_addr();
  mem = mem + index;
  mem->status = status;

  val_data_cache_ops_by_va((addr_t)&mem->status, CLEAN_AND_INVALIDATE);
}

/**
  @brief  Return the state and status for the  input PE index
          1. Caller       - Test Suite
          2. Prerequisite - val_allocate_shared_mem
  @param  index  - index of the PE who is reporting this status.
  @return 32-bit value concatenated from state, level, error value
**/
uint32_t
val_get_status(uint32_t index)
{
  volatile VAL_SHARED_MEM_t *mem;

  mem = (VAL_SHARED_MEM_t *) pal_mem_get_shared_addr();
  mem = mem + index;

  val_data_cache_ops_by_va((addr_t)&mem->status, INVALIDATE);

  return (uint32_t)(mem->status);

}

