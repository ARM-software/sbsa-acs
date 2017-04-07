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
#include "include/sbsa_avs_secure.h"
#include "include/sbsa_avs_common.h"
#include "include/sbsa_std_smc.h"
#include "include/sbsa_avs_peripherals.h"


/**
  @brief   This API executes all the Secure mode tests sequentially
           1. Caller       -  Application layer
           2. Prerequisite -  Secure FW code ported at EL3
  @param   level  - level of compliance being tested for
  @param   num_pe - the number of PE to run these tests on
  @return  Consolidated status of all the tests run
**/
uint32_t
val_secure_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status;

  status = val_secure_trusted_firmware_init();
  if(status != SBSA_SMC_INIT_SIGN){
      val_print(AVS_PRINT_WARN, "\n   ARM-TF firmware not ported, skipping all secure tests", 0);
      return AVS_STATUS_SKIP;
  }

  status = s001_entry(num_pe);
  status |= s002_entry(num_pe);
  status |= s003_entry(num_pe);
  status |= m002_entry(num_pe);

  return status;
}

uint32_t
val_secure_trusted_firmware_init(void)
{
  SBSA_SMC_t  smc;

  smc.test_index = SBSA_SECURE_INFRA_INIT;
  val_secure_call_smc(&smc);

  val_secure_get_result(&smc, 2);
  return smc.test_arg02;
}
/**
  @brief   Call PAL layer to initiate an SMC call to jump to Exception Level 3
           1. Caller       - VAL
           2. Prerequisite - SBSA_SMC service available in secure FW
  @param   smc_param  - Parameters which are used by the Secure Firmware

  @return  None
**/
void
val_secure_call_smc(SBSA_SMC_t *smc_param)
{

  ARM_SMC_ARGS l_smc_args;

  l_smc_args.Arg0 = SBSA_AVS_SMC;
  l_smc_args.Arg1 = smc_param->test_index;
  l_smc_args.Arg2 = smc_param->test_arg01;
  l_smc_args.Arg3 = smc_param->test_arg02;

  pal_pe_call_smc(&l_smc_args);

}

/**
  @brief  This API returns the result of the last executed SBSA_SMC call

**/
uint32_t
val_secure_get_result(SBSA_SMC_t *smc, uint32_t timeout)
{
  ARM_SMC_ARGS l_smc_args;

  while (timeout--) {
      l_smc_args.Arg0 = SBSA_AVS_SMC;
      l_smc_args.Arg1 = SBSA_SECURE_GET_RESULT;
      pal_pe_call_smc(&l_smc_args);

      if (!(IS_RESULT_PENDING(l_smc_args.Arg1)))
          break;
  }

  /* the test index corresponding to this result is returned back in Arg1 */
  smc->test_index = l_smc_args.Arg0;
  /* Arg2 and Arg3 are the actual return data */
  smc->test_arg01 = l_smc_args.Arg1;
  smc->test_arg02 = l_smc_args.Arg2;
  smc->test_arg03 = l_smc_args.Arg3;

  val_print(AVS_PRINT_INFO, "\n       return data 1 is %x ", smc->test_index);
  val_print(AVS_PRINT_INFO, "    return data 2 is %x", smc->test_arg01);

  return (uint32_t)l_smc_args.Arg1;
}


