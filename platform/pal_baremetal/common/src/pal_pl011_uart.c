/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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

#include "pal_pl011_uart.h"

static volatile uint64_t g_uart = PLATFORM_UART_BASE;
static uint8_t is_uart_init_done;

/**
 *   @brief    - This function initializes the UART
 *   @param    - uart_base_addr: Base address of UART
 *   @return   - none
**/
static void pal_driver_uart_pl011_init(void)
{
    uint32_t bauddiv = (UART_PL011_CLK_IN_HZ * 4) / UART_PL011_BAUDRATE;

    /* Disable uart before programming */
    ((pal_uart_t *)g_uart)->uartcr &= ~UART_PL011_UARTCR_EN_MASK;

    /* Write the IBRD */
    ((pal_uart_t *)g_uart)->uartibrd = bauddiv >> 6;

    /* Write the FBRD */
    ((pal_uart_t *)g_uart)->uartfbrd = bauddiv & 0x3F;

    /* Set line of control */
    ((pal_uart_t *)g_uart)->uartlcr_h = UART_PL011_LINE_CONTROL;

    /* Clear any pending errors */
    ((pal_uart_t *)g_uart)->uartecr = 0;

    /* Enable tx, rx, and uart overall */
    ((pal_uart_t *)g_uart)->uartcr = UART_PL011_UARTCR_EN_MASK
                            | UART_PL011_UARTCR_TX_EN_MASK;
}

/**
 *   @brief    - This function checks for empty TX FIFO
 *   @param    - none
 *   @return   - status
**/
static int pal_driver_uart_pl011_is_tx_empty(void)
{
    if ((((pal_uart_t *)g_uart)->uartcr & UART_PL011_UARTCR_EN_MASK) &&
        /* UART is enabled */
        (((pal_uart_t *)g_uart)->uartcr & UART_PL011_UARTCR_TX_EN_MASK) &&
        /* Transmit is enabled */
        ((((pal_uart_t *)g_uart)->uartfr & UART_PL011_UARTFR_TX_FIFO_FULL) == 0))
    {
        return 1;
    } else
    {
        return 0;
    }
}

/**
 *   @brief    - This function checks for empty TX FIFO and writes to FIFO register
 *   @param    - char to be written
 *   @return   - none
**/
void pal_driver_uart_pl011_putc(int c)
{
    const uint8_t pdata = (uint8_t)c;

    if (is_uart_init_done == 0)
    {
        pal_driver_uart_pl011_init();
        is_uart_init_done = 1;
    }

    /* ensure TX buffer to be empty */
    while (!pal_driver_uart_pl011_is_tx_empty())
      ;

    /* write the data (upper 24 bits are reserved) */
    ((pal_uart_t *)g_uart)->uartdr = pdata;
}
