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

#ifndef __SBSA_ACS_PLATFORM_H__
#define __SBSA_ACS_PLATFORM_H__

#include <arm_def.h>
/**
  @brief SECURE DEVICE Definitions  - Modify them to match the Target Platform
**/

  #define SBSA_CNTControlBase               ARM_SYS_CNTCTL_BASE
  #define SBSA_CNTReadBase                  ARM_SYS_CNTREAD_BASE
  #define SBSA_CNTCTLBase                   ARM_SYS_TIMCTL_BASE

  #define SBSA_SECURE_SYSTEM_TIMER          0
  #define SBSA_SECURE_CNTBaseN              0
  #define SBSA_SECURE_SYS_TIMER_INTID       0

  #define SBSA_GENERIC_TWDOG_BASE           0
  #define SBSA_SP805_TWDOG_BASE             ARM_SP805_TWDG_BASE
  #define WDOG_UNLOCK_KEY                   0x1ACCE551

  #define SBSA_SEC_UART_BASE                PLAT_ARM_BL31_RUN_UART_BASE
  #define SBSA_SEC_UART_GSIV                0     //148

  #define SBSA_TRUSTED_SRAM_BASE1           ARM_TRUSTED_SRAM_BASE + 0x00
  #define SBSA_TRUSTED_SRAM_BASE2           ARM_TRUSTED_SRAM_BASE + 0x20
  #define SBSA_TRUSTED_SRAM_BASE3           ARM_TRUSTED_SRAM_BASE + 0x40
  #define SBSA_TRUSTED_SRAM_BASE4           ARM_TRUSTED_SRAM_BASE + 0x60

/**
  @brief PLATFORM FUNCTIONS - Modify this based on the Target Platform
**/
  #define acs_printf                        tf_printf
  #define sbsa_acs_acknowledge_interrupt    plat_ic_acknowledge_interrupt
  #define sbsa_acs_get_pending_interrupt_id plat_ic_get_pending_interrupt_id
  #define sbsa_acs_end_of_interrupt(x)      plat_ic_end_of_interrupt(x)
/*********************************************************************/


#endif
