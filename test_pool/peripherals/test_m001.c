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

#include "val/include/sbsa_avs_val.h"
#include "val/include/val_interface.h"

#include "val/include/sbsa_avs_peripherals.h"
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM   (AVS_PER_TEST_NUM_BASE + 5)
#define TEST_DESC  "Memory Access to Un-Populated addr"

#define START_ADDR 0x4200000       // 66 MB - Randomly chosen
#define LOOP_VAR   3               // Number of Addresses to check
#define STEP_SIZE  0x1000000       // Step size to increment address

static void *branch_to_test;

static
void
payload();

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to point to next instrcution */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(AVS_PRINT_INFO, "\n       Received DAbort Exception ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
}

static
void
payload()
{
  addr_t addr = START_ADDR;
  uint64_t attr;
  uint32_t loop_var = LOOP_VAR;      //Increase to 2048 times to increase coverage
                              //of all unpopulated regions

  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);

  /* If we don't find a single un-populated address, mark this test as skipped */
  val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
  while (loop_var > 0) {
      if (val_memory_get_info(addr, &attr) == MEM_TYPE_NOT_POPULATED) {
         /* default value of FAIL, Pass is set in the exception handler */
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));

          branch_to_test = &&exception_taken;

          *((volatile uint64_t*)addr) = 0x100;
exception_taken:
          /* if the access did not go to our exception handler, fail and exit */
          if (IS_TEST_FAIL(val_get_status(index))) {
              val_print(AVS_PRINT_ERR, "\n      Memory access check fails at address = 0x%llx ", addr);
              return;
          }

      }
      addr += STEP_SIZE;   //check at 16MB hops
      loop_var --;
  }

}

uint32_t
m001_entry(uint32_t num_pe)
{

  uint32_t error_flag = 0;
  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, val_pe_get_num(), g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  error_flag = val_check_for_error(TEST_NUM, num_pe);

  if (!error_flag)
      status = AVS_STATUS_PASS;
  else
      status = AVS_STATUS_FAIL;

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}

