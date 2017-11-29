/** @file
 * Copyright (c) 2017, ARM Limited or its affiliates. All rights reserved.

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
#include "include/sbsa_std_smc.h"

#include "include/sbsa_avs_wakeup.h"

/**
  @brief   This API executes all the wakeup tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  None
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_wakeup_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status, i;

  for (i=0 ; i<MAX_TEST_SKIP_NUM ; i++){
      if (g_skip_test_num[i] == AVS_WAKEUP_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "      USER Override - Skipping all Wakeup tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  status = u001_entry(num_pe);
  //status |= u002_entry(num_pe);

  return status;

}


/**
  @brief  This API initiates a Power state Suspend sequence by calling SUSPEND PSCI call

  @param power_state  - See PSCI specification
  @entry              - See PSCI specification
  @context_id         - See PSCI specification
**/
void
val_suspend_pe(uint32_t power_state, uint64_t entry, uint32_t context_id)
{
  ARM_SMC_ARGS smc_args;

  smc_args.Arg0 = ARM_SMC_ID_PSCI_CPU_SUSPEND_AARCH64;
  smc_args.Arg1 = power_state;
  smc_args.Arg2 = entry;
  smc_args.Arg3 = context_id;
  pal_pe_call_smc(&smc_args);
}


/**
  @brief   Common API to initiate any Low power state entry
  @param   semantic  - See SBSA specification

  @return  always 0 - not used for now
**/
uint32_t
val_power_enter_semantic(SBSA_POWER_SEM_e semantic)
{

  switch(semantic) {
      case SBSA_POWER_SEM_B:
          ArmCallWFI();
          break;
      default:
          break;
  }

  return 0;
}
