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

#ifndef __SBSA_AVS_PERIPHERALS_H__
#define __SBSA_AVS_PERIPHERALS_H__


uint32_t d001_entry(uint32_t num_pe);
uint32_t d002_entry(uint32_t num_pe);
uint32_t d003_entry(uint32_t num_pe);


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


uint32_t m001_entry(uint32_t num_pe);
uint32_t m002_entry(uint32_t num_pe);

#endif // __SBSA_AVS_PERIPHERAL_H__
