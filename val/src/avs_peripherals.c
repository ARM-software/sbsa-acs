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
#include "include/sbsa_avs_peripherals.h"
#include "include/sbsa_avs_common.h"


PERIPHERAL_INFO_TABLE  *g_peripheral_info_table;

#ifndef TARGET_LINUX
/**
  @brief  Sequentially execute all the peripheral tests
          1. Caller       - Application
          2. Prerequisite - val_peripheral_create_info_table
  @param  level  - level of compliance being tested for
  @param  num_pe - number of PEs to run this test on

  @result  consolidated status of all the tests
**/
uint32_t
val_peripheral_execute_tests(uint32_t level, uint32_t num_pe)
{

  uint32_t status, i;

  for (i=0 ; i<MAX_TEST_SKIP_NUM ; i++){
      if (g_skip_test_num[i] == AVS_PER_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "      USER Override - Skipping all Peripheral tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  status = d001_entry(num_pe);
  status |= d002_entry(num_pe);
  status |= d003_entry(num_pe);
  status |= m001_entry(num_pe);

  if (status == AVS_STATUS_FAIL) {
      val_print(AVS_PRINT_ERR, "\n    One or more Peripheral tests have failed...\n", status);
  }
  return status;
}
#endif

/**
  @brief  Return the Index of the entry in the peripheral info table
          which matches the input type and the input instance number
          Instance number is '0' based
          1. Caller       - VAL
          2. Prerequisite - val_peripheral_create_info_table
  @param  type     - Type of peripheral whose index needs to be returned
  @param  instance - Instance number is '0' based.

  @result  Index of peripheral matching type and instance
**/

uint32_t
val_peripheral_get_entry_index(uint32_t type, uint32_t instance)
{
  uint32_t  i = 0;

  while (g_peripheral_info_table->info[i].type != 0xFF) {
      if (type == PERIPHERAL_TYPE_NONE || g_peripheral_info_table->info[i].type == type) {
          if (instance == 0)
             return i;
          else
             instance--;

      }
      i++;
  }
  return 0xFFFF;
}

/**
  @brief  Single entry point to return all Peripheral related information.
          1. Caller       - Test Suite
          2. Prerequisite - val_peripheral_create_info_table
  @param  info_type - Type of peripheral whose index needs to be returned
  @param  instance  - id (0' based) for which the info has to be returned

  @result  64-bit data of peripheral matching type and instance
**/
uint64_t
val_peripheral_get_info(PERIPHERAL_INFO_e info_type, uint32_t instance)
{
  uint32_t i;

  if (g_peripheral_info_table == NULL)
      return 0;

  switch(info_type) {
      case NUM_USB:
          return g_peripheral_info_table->header.num_usb;
      case NUM_SATA:
          return g_peripheral_info_table->header.num_sata;
      case NUM_UART:
          return g_peripheral_info_table->header.num_uart;
      case NUM_ALL:
          return g_peripheral_info_table->header.num_all;
      case USB_BASE0:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_USB, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].base0;
      case USB_FLAGS:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_USB, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].flags;
      case USB_GSIV:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_USB, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].irq;
      case USB_BDF:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_USB, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].bdf;
      case SATA_BASE0:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_SATA, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].base0;
      case SATA_BASE1:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_SATA, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].base1;
      case SATA_FLAGS:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_SATA, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].flags;
      case SATA_BDF:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_SATA, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].bdf;
      case SATA_GSIV:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_SATA, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].irq;
      case UART_BASE0:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_UART, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].base0;
      case UART_GSIV:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_UART, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].irq;
      case UART_FLAGS:
          i = val_peripheral_get_entry_index(PERIPHERAL_TYPE_UART, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].flags;
      case ANY_FLAGS:
          i = val_peripheral_get_entry_index (PERIPHERAL_TYPE_NONE, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].flags;
      case ANY_GSIV:
          i = val_peripheral_get_entry_index (PERIPHERAL_TYPE_NONE, instance);
          if (i != 0xFFFF)
            return g_peripheral_info_table->info[i].irq;
      case ANY_BDF:
          i = val_peripheral_get_entry_index (PERIPHERAL_TYPE_NONE, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].bdf;
      case MAX_PASIDS:
          i = val_peripheral_get_entry_index (PERIPHERAL_TYPE_NONE, instance);
          if (i != 0xFFFF)
              return g_peripheral_info_table->info[i].max_pasids;
      default:
          break;
  }
  return 0;
}

/*
 * val_create_peripheralinfo_table:
 *    Caller         Application layer.
 *    Prerequisite   Memory allocated and passed as argument.
 *    Description    This function will call PAL layer to fill all relevant peripheral
 *                   information into the g_peripheral_info_table pointer.
 */
/**
  @brief  This API calls PAL layer to fill all relevant peripheral
          information into the g_peripheral_info_table pointer
          1. Caller       - Application layer
          2. Prerequisite - Memory allocated and passed as argument
  @param  info_table - pointer to a memory where peripheral data is filled

  @result  None
**/

void
val_peripheral_create_info_table(uint64_t *peripheral_info_table)
{

  g_peripheral_info_table = (PERIPHERAL_INFO_TABLE *)peripheral_info_table;

  pal_peripheral_create_info_table(g_peripheral_info_table);

  val_print(AVS_PRINT_TEST, " Peripheral: Num of USB controllers   :    %d \n",
    val_peripheral_get_info(NUM_USB, 0));
  val_print(AVS_PRINT_TEST, " Peripheral: Num of SATA controllers  :    %d \n",
    val_peripheral_get_info(NUM_SATA, 0));
  val_print(AVS_PRINT_TEST, " Peripheral: Num of UART controllers  :    %d \n",
    val_peripheral_get_info(NUM_UART, 0));

}


/**
  @brief  Free the memory allocated for Peripheral Info table
 **/
void
val_peripheral_free_info_table()
{
  pal_mem_free((void *)g_peripheral_info_table);
}

