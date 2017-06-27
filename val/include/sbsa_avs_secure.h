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

#ifndef __SBSA_AVS_SECURE_H__
#define __SBSA_AVS_SECURE_H__

/*******************************************************************************
  @brief Defines for runtime services function ids
 ******************************************************************************/
#define SBSA_AVS_SMC                    0x84001000

typedef enum {
  SBSA_SECURE_TEST_NSWD_WS1=0x1000,
  SBSA_SECURE_TEST_SYS_COUNTER,
  SBSA_SECURE_TEST_SYS_TIMER_INT,
  SBSA_SECURE_TEST_WD_WS0,
  SBSA_SECURE_TEST_UART,
  SBSA_SECURE_TEST_CNTBASE,
  SBSA_SECURE_TEST_EL3_PHY,
  SBSA_SECURE_TEST_WAKEUP,
  SBSA_SECURE_TEST_FINISH,
  SBSA_SECURE_INFRA_INIT,
  SBSA_SECURE_PLATFORM_ADDRESS,
  SBSA_SECURE_PMBIRQ
}SBSA_SECURE_TEST_INDEX_e;



#define SBSA_SMC_INIT_SIGN          0x9abcdef9
#define SBSA_SECURE_GET_RESULT      0x9000

uint32_t s001_entry(uint32_t num_pe);
uint32_t s002_entry(uint32_t num_pe);
uint32_t s003_entry(uint32_t num_pe);

#endif
