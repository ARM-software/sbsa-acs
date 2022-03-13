/** @file
 * Copyright (c) 2022, Arm Limited or its affiliates. All rights reserved.
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

#include "include/pal_common_support.h"
#include "include/platform_override_struct.h"

extern PLATFORM_OVERRIDE_GIC_INFO_TABLE platform_gic_cfg;

/**
  @brief  Populate information about the GIC sub-system at the input address.

  @param  GicTable  Address of the memory region where this information is to be filled in

  @return None
**/
void
pal_gic_create_info_table(GIC_INFO_TABLE *GicTable)
{

  uint32_t Index;
  uint32_t InfoIndex = 0;

  if (GicTable == NULL) {
    return;
  }

  GicTable->header.gic_version    = platform_gic_cfg.gic_version;
  GicTable->header.num_gicrd      = platform_gic_cfg.num_gicrd;
  GicTable->header.num_gicd       = platform_gic_cfg.num_gicd;
  GicTable->header.num_its        = platform_gic_cfg.num_gicits;
  GicTable->header.num_gich       = platform_gic_cfg.num_gich;
  GicTable->header.num_msi_frames  = platform_gic_cfg.num_msiframes;

  for (Index = 0; Index < platform_gic_cfg.num_gicc; Index++) {
    GicTable->gic_info[InfoIndex].type   = PLATFORM_OVERRIDE_GICC_TYPE;
    GicTable->gic_info[InfoIndex++].base = platform_gic_cfg.gicc_base[Index];
  }

  for (Index = 0; Index < platform_gic_cfg.num_gicrd; Index++) {
    GicTable->gic_info[InfoIndex].type     = PLATFORM_OVERRIDE_GICR_GICRD_TYPE;
    GicTable->gic_info[InfoIndex].base     = platform_gic_cfg.gicrd_base[Index];
    GicTable->gic_info[InfoIndex++].length = platform_gic_cfg.gicrd_length;
  }

  for (Index = 0; Index < platform_gic_cfg.num_gicd; Index++) {
    GicTable->gic_info[InfoIndex].type   = PLATFORM_OVERRIDE_GICD_TYPE;
    GicTable->gic_info[InfoIndex++].base = platform_gic_cfg.gicd_base[Index];
  }

  for (Index = 0; Index < platform_gic_cfg.num_gicits; Index++) {
    GicTable->gic_info[InfoIndex].type     = PLATFORM_OVERRIDE_GICITS_TYPE;
    GicTable->gic_info[InfoIndex].base     = platform_gic_cfg.gicits_base[Index];
    GicTable->gic_info[InfoIndex++].entry_id = platform_gic_cfg.gicits_id[Index];
  }

  for (Index = 0; Index < platform_gic_cfg.num_gich; Index++) {
    GicTable->gic_info[InfoIndex].type     = PLATFORM_OVERRIDE_GICH_TYPE;
    GicTable->gic_info[InfoIndex].base     = platform_gic_cfg.gich_base[Index];
    GicTable->gic_info[InfoIndex++].length = 0;
  }

  for (Index = 0; Index < platform_gic_cfg.num_msiframes; Index++) {
    GicTable->gic_info[InfoIndex].type       = PLATFORM_OVERRIDE_GICMSIFRAME_TYPE;
    GicTable->gic_info[InfoIndex].base       = platform_gic_cfg.gicmsiframe_base[Index];
    GicTable->gic_info[InfoIndex].entry_id   = platform_gic_cfg.gicmsiframe_id[Index];
    GicTable->gic_info[InfoIndex].flags      = platform_gic_cfg.gicmsiframe_flags[Index];
    GicTable->gic_info[InfoIndex].spi_count  = platform_gic_cfg.gicmsiframe_spi_count[Index];
    GicTable->gic_info[InfoIndex++].spi_base   = platform_gic_cfg.gicmsiframe_spi_base[Index];
  }

  GicTable->gic_info[InfoIndex].type = 0xFF;  //Indicate end of data

}


/**
  @brief  Installs an Interrupt Service Routine for int_id.
          Enable the interrupt in the GIC Distributor and GIC CPU Interface and hook
          the interrupt service routine for the Interrupt ID.

  @param  int_id  Interrupt ID which needs to be enabled and service routine installed for
  @param  isr     Function pointer of the Interrupt service routine

  @return Status of the operation
**/
uint32_t
pal_gic_install_isr(uint32_t int_id,  void (*isr)())
{
  /* This API installs the interrupt service routine for interrupt int_id.
   * For SPIs, PPIs & SGIs
   * Configure interrupt Trigger edge/level.
   * Program Interrupt routing.
   * Enable Interrupt.
   * Install isr for int_id.
   */

  return 0;
}

/**
  @brief  Indicate that processing of interrupt is complete by writing to
          End of interrupt register in the GIC CPU Interface

  @param  int_id  Interrupt ID which needs to be acknowledged that it is complete

  @return Status of the operation
**/
uint32_t
pal_gic_end_of_interrupt(uint32_t int_id)
{

  return 0;
}


/**
 @Registers the interrupt handler for a given IRQ.

 @param irq_num: hardware IRQ number
 @param mapped_irq_num: mapped IRQ number
 @param isr: interrupt service routine that returns the status

**/
uint32_t
pal_gic_request_irq (
  uint32_t IrqNum,
  uint32_t MappedIrqNum,
  void *Isr
  )
{
    return 0;
}

/**
 @Frees the registered interrupt handler for agiven IRQ.

 @param Irq_Num: hardware IRQ number
 @param MappedIrqNum: mapped IRQ number

**/
void
pal_gic_free_irq (
  uint32_t IrqNum,
  uint32_t MappedIrqNum
  )
{

}

/**
  @brief  Indicate that processing of interrupt is complete by writing to
          End of interrupt register in the GIC CPU Interface

  @param  int_id  Interrupt ID which needs to be acknowledged that it is complete

  @return Status of the operation
**/
uint32_t
pal_gic_set_intr_trigger(uint32_t int_id, INTR_TRIGGER_INFO_TYPE_e trigger_type)
{

  return 0;
}
