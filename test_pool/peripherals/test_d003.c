/** @file
 * Copyright (c) 2016-2018, 2021-2022 Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_val.h"
#include "val/include/val_interface.h"
#include "val/include/sbsa_avs_pe.h"

#include "val/include/sbsa_avs_peripherals.h"
#include "val/include/sbsa_avs_gic.h"

#define TEST_NUM   (AVS_PER_TEST_NUM_BASE + 3)
/*one space character is removed from TEST_DESC, to nullify a space written as part of the test */
#define TEST_DESC  "Check SBSA UART register offsets "
#define TEST_NUM1  (AVS_PER_TEST_NUM_BASE + 4)
#define TEST_DESC1 "Check Generic UART Interrupt      "

static uint64_t l_uart_base;
static uint32_t int_id;
static void *branch_to_test;
static uint32_t test_fail;

static
void
esr(uint64_t interrupt_type, void *context)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Update the ELR to point to next instrcution */
  val_pe_update_elr(context, (uint64_t)branch_to_test);

  val_print(AVS_PRINT_ERR, "\n       Error : Received Sync Exception type %d", interrupt_type);
  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
}

static
uint32_t
uart_reg_read(uint32_t offset, uint32_t width_mask)
{
  if (width_mask & WIDTH_BIT8)
      return *((volatile uint8_t *)(l_uart_base + offset));

  if (width_mask & WIDTH_BIT16)
      return *((volatile uint16_t *)(l_uart_base + offset));

  if (width_mask & WIDTH_BIT32)
      return *((volatile uint32_t *)(l_uart_base + offset));

  return 0;
}

static
void
uart_reg_write(uint32_t offset, uint32_t width_mask, uint32_t data)
{
  if (width_mask & WIDTH_BIT8)
      *((volatile uint8_t *)(l_uart_base + offset)) = (uint8_t)data;

  if (width_mask & WIDTH_BIT16)
      *((volatile uint16_t *)(l_uart_base + offset)) = (uint16_t)data;

  if (width_mask & WIDTH_BIT32)
      *((volatile uint32_t *)(l_uart_base + offset)) = (uint32_t)data;

}

static
void
uart_setup(void)
{



}

static
void
uart_enable_txintr(void)
{
  uint32_t data;

  /* Enable TX interrupt by setting mask bit[5] in UARTIMSC */
  data = uart_reg_read(SBSA_UARTIMSC, WIDTH_BIT32);
  data = data | (1<<5);
  uart_reg_write(SBSA_UARTIMSC, WIDTH_BIT32, data);
}

static
void
uart_disable_txintr(void)
{
  uint32_t data;

  /* Disable TX interrupt by clearing mask bit[5] in UARTIMSC */
  data = uart_reg_read(SBSA_UARTIMSC, WIDTH_BIT32);
  data = data & (~(1<<5));
  uart_reg_write(SBSA_UARTIMSC, WIDTH_BIT32, data);

}

static
void
isr(void)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uart_disable_txintr();
  val_print(AVS_PRINT_DEBUG, "\n       Received interrupt      ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM1, 01));
  val_gic_end_of_interrupt(int_id);
}

static
uint32_t
validate_register_access(uint32_t offset, uint32_t width)
{
  if (width & WIDTH_BIT8) {
      uart_reg_write(offset, WIDTH_BIT8, 0xF);
  }
  if (width & WIDTH_BIT16) {
      uart_reg_write(offset, WIDTH_BIT16, 0xF);
  }
  if (width & WIDTH_BIT32) {
      uart_reg_write(offset, WIDTH_BIT32, 0xF);
  }
  return AVS_STATUS_PASS;

}

static
void
payload(void)
{

  uint32_t count = val_peripheral_get_info(NUM_UART, 0);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
  val_pe_install_esr(EXCEPT_AARCH64_SERROR, esr);

  branch_to_test = &&exception_taken;

  if (count == 0) {
      val_print(AVS_PRINT_WARN, "\n       No UART defined by Platform      ", 0);
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  while (count != 0) {

      l_uart_base = val_peripheral_get_info(UART_BASE0, count - 1);
      if (l_uart_base == 0) {
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
          return;
      }

      uart_setup();

      /*Make sure  write to a read only register doesn't cause any exceptions*/
      validate_register_access(SBSA_UARTFR, WIDTH_BIT8 | WIDTH_BIT16 | WIDTH_BIT32);
      validate_register_access(SBSA_UARTRIS, WIDTH_BIT16 | WIDTH_BIT32);
      validate_register_access(SBSA_UARTMIS, WIDTH_BIT16 | WIDTH_BIT32);
      /* Writing bits 11:8 as 0xF and writing space character (0x20) to UART data register */
      uart_reg_write(SBSA_UARTDR, WIDTH_BIT32, 0xF20);

      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

      count--;
  }
exception_taken:
  return;
}

static
void
payload1(void)
{
  uint32_t count = val_peripheral_get_info(NUM_UART, 0);
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t timeout;

  if (count == 0) {
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM1, 01));
      return;
  }

  while (count != 0) {
      timeout = TIMEOUT_MEDIUM;
      int_id    = val_peripheral_get_info(UART_GSIV, count - 1);
      l_uart_base = val_peripheral_get_info(UART_BASE0, count - 1);

      /* If Interrupt ID is available, check for interrupt generation */
      if (int_id != 0x0) {
          /* PASS will be set from ISR */
          val_set_status(index, RESULT_PENDING(g_sbsa_level, TEST_NUM1));
          if (val_gic_install_isr(int_id, isr)) {
              val_print(AVS_PRINT_ERR, "\n       GIC Install Handler Fail", 0);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM1, 01));
              return;
          }
          uart_enable_txintr();
          val_print_raw(l_uart_base, g_print_level,
                        "\n       Test Message                      ", 0);

          while ((--timeout > 0) && (IS_RESULT_PENDING(val_get_status(index)))){
          };

          if (timeout == 0) {
              val_print(AVS_PRINT_ERR,
                        "\n       Did not receive UART interrupt %d  ", int_id);
              test_fail++;
          }
      } else {
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM1, 02));
      }

      count--;
  }
  if (test_fail)
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM1, 02));
  else
    val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM1, 02));
}


/**
   @brief    Verify UART registers for Read-only bits and also
             enable interrupt generation
**/
uint32_t
d003_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);
  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  if (!status) {
      status = val_initialize_test(TEST_NUM1, TEST_DESC1, val_pe_get_num(), g_sbsa_level);
      if (status != AVS_STATUS_SKIP)
          val_run_test_payload(TEST_NUM1, num_pe, payload1, 0);

      /* get the result from all PE and check for failure */
      status = val_check_for_error(TEST_NUM1, num_pe);
      val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM1));
  }


  return status;
}
