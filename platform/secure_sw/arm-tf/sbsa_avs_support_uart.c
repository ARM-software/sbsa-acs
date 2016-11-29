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
#include <platform.h>
#include "sbsa_avs.h"

extern uint64_t g_sbsa_test_index;
extern uint64_t g_sbsa_acs_result;
extern uint64_t g_sbsa_acs_return_data;

#define WIDTH_BIT8     0x1
#define WIDTH_BIT16    0x2
#define WIDTH_BIT32    0x4

#define SBSA_UARTDR    0x0
#define SBSA_UARTRSR   0x4
#define SBSA_UARTFR    0x18
#define SBSA_UARTLCR_H 0x2C
#define SBSA_UARTCR    0x30
#define SBSA_UARTIMSC  0x38
#define SBSA_UARTRIS   0x3C
#define SBSA_UARTMIS   0x40
#define SBSA_UARTICR   0x44

uint64_t l_uart_base;

/**
  @brief   This API will return UART memory mapped register value based
           on desired width (8/16/32)
**/
uint32_t
uart_reg_read(uint32_t offset, uint32_t width_mask)
{
	if (width_mask & WIDTH_BIT8)
		return *((uint8_t *)(l_uart_base + offset));

	if (width_mask & WIDTH_BIT16)
		return *((uint16_t *)(l_uart_base + offset));

	if (width_mask & WIDTH_BIT32)
		return *((uint32_t *)(l_uart_base + offset));

	return 0;
}

/**
  @brief   This API will write UART memory mapped register value based
           on desired width (8/16/32)
**/
void
uart_reg_write(uint32_t offset, uint32_t width_mask, uint32_t data)
{
	if (width_mask & WIDTH_BIT8)
		*((uint8_t *)(l_uart_base + offset)) = (uint8_t)data;

	if (width_mask & WIDTH_BIT16)
		*((uint16_t *)(l_uart_base + offset)) = (uint16_t)data;

	if (width_mask & WIDTH_BIT32)
		*((uint32_t *)(l_uart_base + offset)) = (uint32_t)data;

}

void
uart_setup()
{



}

/**
  @brief   This API will unmask TX interrupt bit
**/
void
uart_enable_txintr()
{
	uint32_t data;

	/* unmask TX interrupt bit 5 in */
	data = uart_reg_read(SBSA_UARTIMSC, WIDTH_BIT32);
	data = data | (1<<5);
	uart_reg_write(SBSA_UARTIMSC, WIDTH_BIT32, data);
}

/**
  @brief   This API will mask TX interrupt bit
**/
void
uart_disable_txintr()
{
	uint32_t data;

	/* mask TX interrupt bit 5 in */
	data = uart_reg_read(SBSA_UARTIMSC, WIDTH_BIT32);
	data = data & (~(1<<5));
	uart_reg_write(SBSA_UARTIMSC, WIDTH_BIT32, data);
}

/**
  @brief   This API will verify read-only behavior of UART registers
**/
uint32_t
validate_register_readonly(uint32_t offset, uint32_t width)
{
	uint32_t data = 0;

	if (width & WIDTH_BIT8)
	{
		data = uart_reg_read(offset, WIDTH_BIT8);
		uart_reg_write(offset, WIDTH_BIT8, 0xF);
		if (data != uart_reg_read(offset, WIDTH_BIT8))
		{
		        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x1);
			return ACS_STATUS_FAIL;
		}
	}
	if (width & WIDTH_BIT16)
	{
		data = uart_reg_read(offset, WIDTH_BIT16);
		uart_reg_write(offset, WIDTH_BIT16, 0xF);
		if (data != uart_reg_read(offset, WIDTH_BIT16))
		{
		        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x1);
			return ACS_STATUS_FAIL;
		}

	}
	if (width & WIDTH_BIT32)
	{
		data = uart_reg_read(offset, WIDTH_BIT32);
		uart_reg_write(offset, WIDTH_BIT32, 0xF);
		if (data != uart_reg_read(offset, WIDTH_BIT32))
		{
		        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x1);
			return ACS_STATUS_FAIL;
		}
	}
	return ACS_STATUS_PASS;
}

/**
  @brief   This API will verify secure UART functionality
**/
void
uart_compliance_test()
{
        volatile uint32_t timeout = 0x5;
	uint32_t int_id;
	uint32_t data;

	l_uart_base = SBSA_SEC_UART_BASE;
	acs_printf("\n Testing UART controller at %lx \n", l_uart_base);
	if (l_uart_base == 0) {
		sbsa_acs_set_status(ACS_STATUS_FAIL, 0x1);
		return;
	}
	int_id    = SBSA_SEC_UART_GSIV;

	uart_setup();

	if (validate_register_readonly(SBSA_UARTFR, WIDTH_BIT8 | WIDTH_BIT16 | WIDTH_BIT32))
		return;

	if (validate_register_readonly(SBSA_UARTRIS, WIDTH_BIT16 | WIDTH_BIT32))
		return;

	if (validate_register_readonly(SBSA_UARTMIS, WIDTH_BIT16 | WIDTH_BIT32))
		return;

	/* Check bits 11:8 in the UARTDR reg are read-only */
	data = uart_reg_read(SBSA_UARTDR, WIDTH_BIT32);
	uart_reg_write(SBSA_UARTDR, WIDTH_BIT32, data | 0x0F00);
	data = (data >> 8) & 0x0F;
	if (data != ((uart_reg_read(SBSA_UARTDR, WIDTH_BIT32)>>8) & 0x0F))
	{
		acs_printf("\n UARTDR Bits 11:8 are not Read Only");
		sbsa_acs_set_status(ACS_STATUS_FAIL, 0x1);
		return;
	}

	sbsa_acs_set_status(ACS_STATUS_PASS, 0x1);

	acs_printf("\n UART basic tests done \n");
	/* If Interrupt ID is available, check for interrupt generation */
	if (int_id != 0x0)
	{
		uart_enable_txintr();
		acs_printf("\n Test Message                   ");
		while(--timeout)
		{
			int_id = plat_ic_get_pending_interrupt_id();
			acs_printf(" %x ", int_id);
			if (int_id == 29)
			{
				plat_ic_acknowledge_interrupt();
				uart_disable_txintr();
				plat_ic_end_of_interrupt(29);
				g_sbsa_acs_result = ACS_STATUS_PASS;
				break;
			}
		}

		if (timeout == 0) {
			g_sbsa_acs_result = ACS_STATUS_FAIL;
		}
	}

	return;
}

