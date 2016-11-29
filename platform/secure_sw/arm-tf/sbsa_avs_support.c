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

#include <debug.h>
#include <runtime_svc.h>
#include <std_svc.h>
#include <stdint.h>
#include <arch_helpers.h>

#include "sbsa_avs.h"

extern uint64_t g_sbsa_test_index;
extern uint64_t g_sbsa_acs_result;
extern uint64_t g_sbsa_acs_return_data;


/**
  @brief   This API will perform MMIO register write
**/
void
sbsa_acs_mmio_write(uint64_t addr, uint32_t data)
{
	tf_printf("mmio write: addr = %lx   data = %x \n", addr, data);

	*(volatile unsigned int *)addr = data;
}

/**
  @brief   This API will perform MMIO register read
**/
uint32_t
sbsa_acs_mmio_read(uint64_t addr)
{

	tf_printf("mmio read: addr = %lx   data = %x \n", addr, *(volatile unsigned int *)addr);
	return *(volatile unsigned int *)addr;
}

/**
  @brief   This API will set the global status, which will be used
           to decide the test pass/fail/skip
**/
void
sbsa_acs_set_status(unsigned int status, unsigned int data)
{

	g_sbsa_acs_result       = status;
	g_sbsa_acs_return_data  = data;

}



