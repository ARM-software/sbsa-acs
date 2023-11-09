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

#include <stdint.h>
#include "platform_override_fvp.h"

#ifndef _PAL_UART_PL011_H_
#define _PAL_UART_PL011_H_

/* typedef's */
typedef struct {
    volatile uint32_t uartdr;          /* Offset: 0x000 (R/W) Data Register */
    union {
        volatile uint32_t uartrsr;     /* Offset: 0x004 (R/ ) Receive status register */
        volatile uint32_t uartecr;     /* Offset: 0x004 ( /W) Error clear register */
    };
    volatile uint32_t reserved_0[4];   /* Offset: 0x008-0x014 Reserved */
    volatile uint32_t uartfr;          /* Offset: 0x018 (R/ ) Flag register */
    volatile uint32_t reserved_1;      /* Offset: 0x01C Reserved */
    volatile uint32_t uartilpr;        /* Offset: 0x020 (R/W) IrDA low-power counter register */
    volatile uint32_t uartibrd;        /* Offset: 0x024 (R/W) Integer baud rate register */
    volatile uint32_t uartfbrd;        /* Offset: 0x028 (R/W) Fractional baud rate register */
    volatile uint32_t uartlcr_h;       /* Offset: 0x02C (R/W) Line control register */
    volatile uint32_t uartcr;          /* Offset: 0x030 (R/W) Control register */
    volatile uint32_t uartifls;        /* Offset: 0x034 (R/W) Interrupt FIFO level select reg */
    volatile uint32_t uartimsc;        /* Offset: 0x038 (R/W) Interrupt mask set/clear register */
    volatile uint32_t uartris;         /* Offset: 0x03C (R/ ) Raw interrupt status register */
    volatile uint32_t uartmis;         /* Offset: 0x040 (R/ ) Masked interrupt status register */
    volatile uint32_t uarticr;         /* Offset: 0x044 ( /W) Interrupt clear register */
    volatile uint32_t uartdmacr;       /* Offset: 0x048 (R/W) DMA control register */
} pal_uart_t;

/* UART Enable */
#define UART_PL011_UARTCR_UARTEN_OFF       0x0u
#define UART_PL011_UARTCR_EN_MASK          (0x1u << UART_PL011_UARTCR_UARTEN_OFF)
/* Transmit enable */
#define UART_PL011_UARTCR_TXE_OFF          0x8u
#define UART_PL011_UARTCR_TX_EN_MASK       (0x1u << UART_PL011_UARTCR_TXE_OFF)
#define UART_PL011_UARTFR_TX_FIFO_FULL_OFF 0x5u
#define UART_PL011_UARTFR_TX_FIFO_FULL     (0x1u << UART_PL011_UARTFR_TX_FIFO_FULL_OFF)

#define UART_PL011_INTR_TX_OFF             0x5u
#define UART_PL011_TX_INTR_MASK            (0x1u << UART_PL011_INTR_TX_OFF)
#define UART_PL011_UARTLCR_H_FEN_OFF       0x4u
#define UART_PL011_UARTLCR_H_FEN_MASK      (0x1u << UART_PL011_UARTLCR_H_FEN_OFF)
#define UART_PL011_UARTLCR_H_WLEN_8        5
#define UART_PL011_UARTLCR_H_WLEN_8_MASK   (3 << UART_PL011_UARTLCR_H_WLEN_8)
#define UART_PL011_CLK_IN_HZ               UART_CLK_RATE_HZ
#define UART_PL011_BAUDRATE                UART_BAUD_RATE
#define UART_PL011_LINE_CONTROL            (UART_PL011_UARTLCR_H_FEN_MASK | UART_PL011_UARTLCR_H_WLEN_8_MASK)

#define PLATFORM_UART_BASE                 BASE_ADDRESS_ADDRESS

/* function prototypes */
extern void pal_driver_uart_pl011_putc(int c);

#define pal_uart_putc(x) pal_driver_uart_pl011_putc(x)

#endif /* _PAL_UART_PL011_H_ */
