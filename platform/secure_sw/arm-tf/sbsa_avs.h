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

#include <arm_def.h>
#include "sbsa_acs_platform.h"

/*********************************************************************/

#define ACS_STATUS_PENDING   0xA0000000

#define ACS_STATUS_FAIL 0x90000000
#define ACS_STATUS_SKIP 0x10000000
#define ACS_STATUS_PASS 0x0


void     sbsa_acs_mmio_write(uint64_t addr, uint32_t data);
uint32_t sbsa_acs_mmio_read(uint64_t addr);
void     sbsa_acs_set_status(unsigned int status, unsigned int data);


/**
  @brief  Defines for CNTControlBase Registers
**/
#define CNTCR                           0x000
#define CNTSR                           0x004
#define CNTCV_LO                        0x008
#define CNTCV_HI                        0x00C
#define CNTFID0                         0x020
#define CounterID0                      0xFD0

/**
  @brief  Defines for runtime services function ids
**/
#define SBSA_AVS_SMC                    0x84001000

typedef enum {
  SBSA_SECURE_TEST_NSWD_WS1=0x1000,
  SBSA_SECURE_TEST_SYS_COUNTER,
  SBSA_SECURE_TEST_SYS_TIMER_INT,
  SBSA_SECURE_TEST_WD_WS0,
  SBSA_SECURE_TEST_SEC_UART,
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

#define SBSA_GENERIC             0xA
#define SP805                    0xB

uint64_t sbsa_smc_handler(uint32_t smc_fid,
                          uint64_t x1,
                          uint64_t x2,
                          uint64_t x3,
                          uint64_t x4,
                          void *cookie,
                          void *handle,
                          uint64_t flags);


#endif
