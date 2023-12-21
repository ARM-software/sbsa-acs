/** @file
 * Copyright (c) 2016-2023, Arm Limited or its affiliates. All rights reserved.
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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_common.h"

extern uint32_t g_override_skip;

/**
  @brief  Parse the input status and print the appropriate information to console
          1. Caller       - Application layer
          2. Prerequisite - None
  @param  index  - index of the PE who is reporting this status.
  @param status  - 32-bit value concatenated from state, level, error value

  @return  none
 **/
void
val_report_status(uint32_t index, uint32_t status, char8_t *ruleid)
{

  /* Test stays quiet if it is overridden by any of the user options */
  if (!g_override_skip)
    return;

  if (IS_TEST_FAIL(status)) {
      val_print(AVS_PRINT_ERR, "\n       Failed on PE - %4d ", index);
      val_print(AVS_PRINT_ERR, "for Level= %2d ", (status >> LEVEL_BIT) & LEVEL_MASK);
  }

  if (IS_TEST_PASS(status)) {
      val_print(AVS_PRINT_DEBUG, "\n       ", 0);
      val_print(AVS_PRINT_DEBUG, ruleid, 0);
      val_print(AVS_PRINT_DEBUG, "\n                                  ", 0);
      val_print(AVS_PRINT_TEST, ": Result:  PASS\n", status);
  }
  else
    if (IS_TEST_FAIL(status)) {
        if (ruleid) {
            val_print(AVS_PRINT_ERR, "\n       ", 0);
            val_print(AVS_PRINT_ERR, ruleid, 0);
            val_print(AVS_PRINT_ERR, "\n       Checkpoint -- %2d             ",
                      status & STATUS_MASK);
        }
        val_print(AVS_PRINT_ERR, "     : Result:  FAIL\n", 0);
    }
    else
      if (IS_TEST_SKIP(status)) {
          if (ruleid) {
              val_print(AVS_PRINT_WARN, "\n       ", 0);
              val_print(AVS_PRINT_WARN, ruleid, 0);
              val_print(AVS_PRINT_WARN, "\n       Checkpoint -- %2d             ",
                        status & STATUS_MASK);
          }
          val_print(AVS_PRINT_WARN, "     : Result:  SKIPPED\n", 0);
      }
      else
        if (IS_TEST_START(status))
          val_print(AVS_PRINT_INFO, "\n       START  ", status);
        else
          if (IS_TEST_END(status))
            val_print(AVS_PRINT_INFO, "       END\n\n", status);
          else
            val_print(AVS_PRINT_ERR, ": Result:  %8x\n", status);

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

