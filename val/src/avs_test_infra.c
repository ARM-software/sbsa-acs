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
#include "include/sbsa_avs_pe.h"
#include "include/sbsa_avs_common.h"

/**
  @brief  This API calls PAL layer to print a formatted string
          to the output console.
          1. Caller       - Application layer
          2. Prerequisite - None.

  @param level   the print verbosity (1 to 5)
  @param string  formatted ASCII string
  @param data    64-bit data. set to 0 if no data is to sent to console.

  @return        None
 **/
void
val_print(uint32_t level, char8_t *string, uint64_t data)
{

  if (level >= g_print_level)
      pal_print(string, data);

}

/**
  @brief  This API calls PAL layer to print a string to the output console.
          1. Caller       - Application layer
          2. Prerequisite - None.

  @param level   the print verbosity (1 to 5)
  @param string  formatted ASCII string
  @param data    64-bit data. set to 0 if no data is to sent to console.

  @return        None
 **/
void
val_print_raw(uint32_t level, char8_t *string, uint64_t data)
{

  if (level >= g_print_level){
      uint64_t uart_address = val_peripheral_get_info(UART_BASE0, 0);
      pal_print_raw(uart_address, string, data);
  }

}

/**
  @brief  This API calls PAL layer to read from a Memory address
          and return 32-bit data.
          1. Caller       - Test Suite
          2. Prerequisite - None.

  @param addr   64-bit address

  @return       32-bits of data
 **/
uint32_t
val_mmio_read(addr_t addr)
{
  return pal_mmio_read(addr);

}

/**
  @brief  This function will call PAL layer to write 32-bit data to
          a Memory address.
        1. Caller       - Test Suite
        2. Prerequisite - None.

  @param addr   64-bit address
  @param data   32-bit data

  @return       None
 **/
void
val_mmio_write(addr_t addr, uint32_t data)
{

  pal_mmio_write(addr, data);
}

/**
  @brief  This API prinst the test number, description and
          sets the test status to pending for the input number of PEs.
          1. Caller       - Application layer
          2. Prerequisite - val_allocate_shared_mem

  @param test_num unique number identifying this test
  @param desc     brief description of the test
  @param num_pe   the number of PE to execute this test on.
  @param level    compliance level being tested against

  @return         Skip - if the user has overriden to skip the test.
 **/
uint32_t
val_initialize_test(uint32_t test_num, char8_t *desc, uint32_t num_pe, uint32_t level)
{

  uint32_t i;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_print(AVS_PRINT_ERR, "%4d : ", test_num); //Always print this
  val_print(AVS_PRINT_TEST, desc, 0);
  val_report_status(0, SBSA_AVS_START(level, test_num));
  val_pe_initialize_default_exception_handler(val_pe_default_esr);

  g_sbsa_tests_total++;

  for (i = 0; i < num_pe; i++)
      val_set_status(i, RESULT_PENDING(level, test_num));

  for (i=0 ; i<MAX_TEST_SKIP_NUM ; i++){
      if (g_skip_test_num[i] == test_num) {
          val_print(AVS_PRINT_TEST, "\n       USER OVERRIDE  - Skip Test        ", 0);
          val_set_status(index, RESULT_SKIP(g_sbsa_level, test_num, 0));
          return AVS_STATUS_SKIP;
      }
  }

  return AVS_STATUS_PASS;
}

/**
  @brief  Allocate memory which is to be shared across PEs

  @param  None

  @result None
**/
void
val_allocate_shared_mem()
{

  pal_mem_allocate_shared(val_pe_get_num(), sizeof(VAL_SHARED_MEM_t));

}

/**
  @brief  Free the memory which was allocated by allocate_shared_mem
        1. Caller       - Application Layer
        2. Prerequisite - val_allocate_shared_mem

  @param  None

  @result None
**/
void
val_free_shared_mem()
{

  pal_mem_free_shared();
}

/**
  @brief  This function sets the address of the test entry and the test
          argument to the shared address space which is picked up by the
          secondary PE identified by index.
          1. Caller       - VAL
          2. Prerequisite - val_allocate_shared_mem

  @param index     the PE Index
  @param addr      Address of the test payload which needs to be executed by PE
  @param test_data 64-bit data to be passed as a parameter to test payload

  @return        None
 **/
void
val_set_test_data(uint32_t index, uint64_t addr, uint64_t test_data)
{
  volatile VAL_SHARED_MEM_t *mem;

  if(index > val_pe_get_num())
  {
      val_print(AVS_PRINT_ERR, "\n Incorrect PE index = %d", index);
      return;
  }

  mem = (VAL_SHARED_MEM_t *)pal_mem_get_shared_addr();
  mem = mem + index;

  mem->data0 = addr;
  mem->data1 = test_data;

  val_data_cache_ops_by_va((addr_t)&mem->data0, CLEAN_AND_INVALIDATE);
  val_data_cache_ops_by_va((addr_t)&mem->data1, CLEAN_AND_INVALIDATE);
}

/**
  @brief  This API returns the optional data parameter between PEs
          to the output console.
          1. Caller       - Test Suite
          2. Prerequisite - val_set_test_data

  @param index   PE index whose data parameter has to be returned.

  @return    64-bit data
 **/

void
val_get_test_data(uint32_t index, uint64_t *data0, uint64_t *data1)
{

  volatile VAL_SHARED_MEM_t *mem;

  if(index > val_pe_get_num())
  {
      val_print(AVS_PRINT_ERR, "\n Incorrect PE index = %d", index);
      return;
  }

  mem = (VAL_SHARED_MEM_t *) pal_mem_get_shared_addr();
  mem = mem + index;

  val_data_cache_ops_by_va((addr_t)&mem->data0, INVALIDATE);
  val_data_cache_ops_by_va((addr_t)&mem->data1, INVALIDATE);

  *data0 = mem->data0;
  *data1 = mem->data1;

}

/**
  @brief  This function will wait for all PEs to report their status
          or we timeout and set a failure for the PE which timed-out
          1. Caller       - Application layer
          2. Prerequisite - val_set_status

  @param test_num  Unique test number
  @param num_pe    Number of PE who are executing this test
  @param timeout   integer value ob expiry the API will timeout and return

  @return        None
 **/

void
val_wait_for_test_completion(uint32_t test_num, uint32_t num_pe, uint32_t timeout)
{

  uint32_t i = 0, j = 0;

  //For single PE tests, there is no need to wait for the results
  if (num_pe == 1)
      return;

  while(--timeout)
  {
      j = 0;
      for (i = 0; i < num_pe; i++)
      {
          if (IS_RESULT_PENDING(val_get_status(i))) {
              j = i+1;
          }
      }
      //If None of the PE have the status as Pending, return
      if (!j)
          return;
  }
  //We are here if we timed-out, set the last index PE as failed
  val_set_status(j-1, RESULT_FAIL(g_sbsa_level, test_num, 0xF));
}

/**
  @brief  This API Executes the payload function on secondary PEs
          1. Caller       - Application layer
          2. Prerequisite - val_pe_create_info_table

  @param test_num   unique test number
  @param num_pe     The number of PEs to run this test on
  @param payload    Function pointer of the test entry function
  @param test_input optional parameter for the test payload

  @return        None
 **/
void
val_run_test_payload(uint32_t test_num, uint32_t num_pe, void (*payload)(void), uint64_t test_input)
{

  uint32_t my_index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t i;

  payload();  //this is test run separately on present PE
  if (num_pe == 1)
      return;

  //Now run the test on all other PE
  for (i = 0; i < num_pe; i++) {
      if (i != my_index)
          val_execute_on_pe(i, payload, test_input);
  }

  val_wait_for_test_completion(test_num, num_pe, TIMEOUT_LARGE);
}

/**
  @brief  Prints the status of the completed test
          1. Caller       - Test Suite
          2. Prerequisite - val_set_status

  @param test_num   unique test number
  @param num_pe     The number of PEs to query for status

  @return     Success or on failure - status of the last failed PE
 **/
uint32_t
val_check_for_error(uint32_t test_num, uint32_t num_pe)
{
  uint32_t i;
  uint32_t status = 0;
  uint32_t error_flag = 0;
  uint32_t my_index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* this special case is needed when the Main PE is not the first entry
     of pe_info_table but num_pe is 1 for SOC tests */
  if (num_pe == 1) {
      status = val_get_status(my_index);
      val_report_status(my_index, status);
      if (IS_TEST_PASS(status)) {
          g_sbsa_tests_pass++;
          return AVS_STATUS_PASS;
      }
      if (IS_TEST_SKIP(status))
          return AVS_STATUS_SKIP;

      g_sbsa_tests_fail++;
      return AVS_STATUS_FAIL;
  }

  for (i = 0; i < num_pe; i++) {
      status = val_get_status(i);
      //val_print(AVS_PRINT_ERR, "Status %4x \n", status);
      if (IS_TEST_FAIL_SKIP(status)) {
          val_report_status(i, status);
          error_flag += 1;
          break;
      }
  }

  if (!error_flag)
      val_report_status(my_index, status);

  if (IS_TEST_PASS(status)) {
      g_sbsa_tests_pass++;
      return AVS_STATUS_PASS;
  }
  if (IS_TEST_SKIP(status))
      return AVS_STATUS_SKIP;

  g_sbsa_tests_fail++;
  return AVS_STATUS_FAIL;
}

/**
  @brief  Clean and Invalidate the Data cache line containing
          the input address tag
**/
void
val_data_cache_ops_by_va(addr_t addr, uint32_t type)
{
  pal_pe_data_cache_ops_by_va(addr, type);

}

/**
  @brief  Update ELR based on the offset provided
**/
void
val_pe_update_elr(void *context, uint64_t offset)
{
    pal_pe_update_elr(context, offset);
}

/**
  @brief  Get ESR from exception context
**/
uint64_t
val_pe_get_esr(void *context)
{
    return pal_pe_get_esr(context);
}

/**
  @brief  Get FAR from exception context
**/
uint64_t
val_pe_get_far(void *context)
{
    return pal_pe_get_far(context);
}

/**
  @brief  Write to an address, meant for debugging purpose
**/
void
val_debug_brk(uint32_t data)
{
   addr_t address = 0x9000F000; // address = pal_get_debug_address();
   *(addr_t *)address = data;
}
