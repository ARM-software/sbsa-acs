/** @file
 * Copyright (c) 2016-2018, Arm Limited or its affiliates. All rights reserved.
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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_gic.h"
#include "include/sbsa_avs_gic_support.h"
#include "include/sbsa_avs_common.h"

GIC_INFO_TABLE  *g_gic_info_table;

/**
  @brief   This API executes all the GIC tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_gic_create_info_table()
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_gic_execute_tests(uint32_t level, uint32_t num_pe)
{

  uint32_t status, i;

  for (i=0 ; i<MAX_TEST_SKIP_NUM ; i++){
      if (g_skip_test_num[i] == AVS_GIC_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "      USER Override - Skipping all GIC tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  status = g001_entry(num_pe);
  status |= g002_entry(num_pe);
  status |= g003_entry(num_pe);
  status |= g004_entry(num_pe);

  val_print_test_end(status, "GIC");

  return status;

}


/**
  @brief   This API will call PAL layer to fill in the GIC information
           into the g_gic_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   gic_info_table  pre-allocated memory pointer for gic_info
  @return  Error if Input param is NULL
**/
uint32_t
val_gic_create_info_table(uint64_t *gic_info_table)
{

  if (gic_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "Input for Create Info table cannot be NULL \n", 0);
      return AVS_STATUS_ERR;
  }

  g_gic_info_table = (GIC_INFO_TABLE *)gic_info_table;

  pal_gic_create_info_table(g_gic_info_table);

  val_print(AVS_PRINT_TEST, " GIC_INFO: Number of GICD             : %4d \n", g_gic_info_table->header.num_gicd);
  val_print(AVS_PRINT_TEST, " GIC_INFO: Number of ITS              : %4d \n", g_gic_info_table->header.num_its);

  if (g_gic_info_table->header.num_gicd == 0) {
      val_print(AVS_PRINT_ERR,"\n ** CRITICAL ERROR: GIC Distributor count is 0 **\n", 0);
      return AVS_STATUS_ERR;
  }
  return AVS_STATUS_PASS;
}

void
val_gic_free_info_table()
{
  pal_mem_free((void *)g_gic_info_table);
}

/**
  @brief   This API returns the base address of the GIC Distributor.
           The assumption is we have only 1 GIC Distributor. IS this true?
           1. Caller       -  VAL
           2. Prerequisite -  val_gic_create_info_table
  @param   None
  @return  Address of GIC Distributor
**/
addr_t
val_get_gicd_base(void)
{

  GIC_INFO_ENTRY  *gic_entry;

  if (g_gic_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "GIC INFO table not available \n", 0);
      return 0;
  }

  gic_entry = g_gic_info_table->gic_info;

  while (gic_entry->type != 0xFF) {
    if (gic_entry->type == ENTRY_TYPE_GICD) {
        return gic_entry->base;
    }
    gic_entry++;
  }

  return 0;
}

/**
  @brief   This function is a single point of entry to retrieve
           all GIC related information.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   type   the type of information being requested
  @return  32-bit data
**/
uint32_t
val_gic_get_info(uint32_t type)
{
  if (g_gic_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "\n   Get GIC info called before gic info table is filled ",        0);
      return 0;
  }

  switch (type) {

      case GIC_INFO_VERSION:
          if (g_gic_info_table->header.gic_version != 0) {
             val_print(AVS_PRINT_INFO, "\n       gic version from ACPI table = %d ", g_gic_info_table->header.gic_version);
                return g_gic_info_table->header.gic_version;
          }

          return ((val_mmio_read(val_get_gicd_base() + 0xFE8) >> 4) & 0xF);

      case GIC_INFO_SEC_STATES:
          return ((val_mmio_read(val_get_gicd_base() + 0x0) >> 6) & 0x1);

      case GIC_INFO_NUM_ITS:
          return g_gic_info_table->header.num_its;

      default:
          val_print(AVS_PRINT_ERR, "\n    GIC Info - TYPE not recognized %d  ", type);
          break;
  }
  return AVS_STATUS_ERR;
}

/**
  @brief   This API returns the max interrupt ID supported by the GIC Distributor
           1. Caller       -  VAL
           2. Prerequisite -  val_gic_create_info_table
  @param   None
  @return  Maximum Interrupt ID
**/
uint32_t
val_get_max_intid(void)
{
  return 32 * ((val_mmio_read(val_get_gicd_base() + 0x004) & 0x1F) + 1);
}

/**
  @brief   This function routes interrupt to specific PE.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID to be routed
  @param   mpidr MPIDR_EL1 reg value of the PE to which the interrupt should be routed
  @return  status
**/
uint32_t val_gic_route_interrupt_to_pe(uint32_t int_id, uint64_t mpidr)
{
  if (int_id > 31) {
      mpidr &= 0xF80FFFFFF;
      val_mmio_write(val_get_gicd_base() + GICD_IROUTER + (8 * int_id), mpidr);
  }
  else{
      val_print(AVS_PRINT_ERR, "\n    Only SPIs can be routed, interrupt with INTID = %d cannot be routed", int_id);
  }

  return 0;
}

/**
  @brief   This function will return '1' if an interrupt is either pending or active.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID
  @return  pending/active status
**/
uint32_t val_gic_get_interrupt_state(uint32_t int_id)
{
  uint32_t reg_offset = int_id / 32;
  uint32_t reg_shift  = int_id % 32;
  uint32_t mask = (1 << reg_shift);
  uint32_t active, pending;

  pending = val_mmio_read(val_get_gicd_base() + GICD_ISPENDR + (4 * reg_offset));
  active = val_mmio_read(val_get_gicd_base() + GICD_ISACTIVER0 + (4 * reg_offset));

  return ((mask & active) || (mask & pending));
}

/**
  @brief   This function will clear an interrupt that is pending or active.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID
  @return  none
**/
void val_gic_clear_interrupt(uint32_t int_id)
{
    uint32_t reg_offset = int_id / 32;
    uint32_t reg_shift  = int_id % 32;

    if ((int_id > 31) && (int_id < 1020)) {
        val_mmio_write(val_get_gicd_base() + GICD_ICPENDR0 + (4 * reg_offset), (1 << reg_shift));
        val_mmio_write(val_get_gicd_base() + GICD_ICACTIVER0 + (4 * reg_offset), (1 << reg_shift));
    }
    else
        val_print(AVS_PRINT_ERR, "\n    Invalid SPI interrupt ID number %d", int_id);
}

/**
  @brief   This function will initialize CPU interface registers required for interrupt
           routing to a given PE
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   none
  @return  none
**/
void val_gic_cpuif_init(void)
{
  val_gic_reg_write(ICC_IGRPEN1_EL1, 0x1);
  val_gic_reg_write(ICC_BPR1_EL1, 0x7);
  val_gic_reg_write(ICC_PMR_EL1, 0xff);
}

/**
  @brief   This function will Get the trigger type Edge/Level
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID
  @return  Status
**/
uint32_t val_gic_get_intr_trigger_type(uint32_t int_id, INTR_TRIGGER_INFO_TYPE_e *trigger_type)
{
  uint32_t reg_value;
  uint32_t reg_offset;
  uint32_t config_bit_shift;

  if (int_id > val_get_max_intid()) {
    val_print(AVS_PRINT_ERR, "\n       Invalid Interrupt ID number 0x%x ", int_id);
    return AVS_STATUS_ERR;
  }

  reg_offset = int_id / GICD_ICFGR_INTR_STRIDE;
  config_bit_shift  = GICD_ICFGR_INTR_CONFIG1(int_id);

  reg_value = val_mmio_read(val_get_gicd_base() + GICD_ICFGR + (4 * reg_offset));

  if ((reg_value & (1 << config_bit_shift)) == 0)
    *trigger_type = INTR_TRIGGER_INFO_LEVEL_HIGH;
  else
    *trigger_type = INTR_TRIGGER_INFO_EDGE_RISING;

  return 0;
}

/**
  @brief   This function will Set the trigger type Edge/Level based on the GTDT table
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID
  @param   trigger_type Interrupt Trigger Type
  @return  none
**/
void val_gic_set_intr_trigger(uint32_t int_id, INTR_TRIGGER_INFO_TYPE_e trigger_type)
{
  uint32_t status;

  val_print(AVS_PRINT_DEBUG, "\n    Setting Trigger type as %d  ", trigger_type);
  status = pal_gic_set_intr_trigger(int_id, trigger_type);

  if (status)
    val_print(AVS_PRINT_ERR, "\n    Error Could Not Configure Trigger Type", 0);
}
