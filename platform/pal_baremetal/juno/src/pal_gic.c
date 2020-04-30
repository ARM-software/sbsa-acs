/** @file
 * Copyright (c) 2019, Arm Limited or its affiliates. All rights reserved.
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


#include "include/platform_override.h"

extern PLATFORM_OVERRIDE_GIC_INFO_TABLE platform_gic_cfg;


/**
  @brief  Populate information about the GIC sub-system at the input address.

  @param  GicTable  Address of the memory region where this information is to be filled in

  @return None
**/
VOID
pal_gic_create_info_table(GIC_INFO_TABLE *GicTable)
{
  UINT32 Index;
  UINT32 InfoIndex = 0;

  if (GicTable == NULL) {
    return;
  }

  GicTable->header.gic_version = platform_gic_cfg.gic_version;
  GicTable->header.num_gicrd = platform_gic_cfg.num_gicrd;
  GicTable->header.num_gicd = platform_gic_cfg.num_gicd;
  GicTable->header.num_its = platform_gic_cfg.num_gicits;

  for (Index = 0; Index < platform_gic_cfg.num_gicc; Index++) {
    GicTable->gic_info[InfoIndex].type = PLATFORM_OVERRIDE_GICC_TYPE;
    GicTable->gic_info[InfoIndex++].base = platform_gic_cfg.gicc_base[Index];
  }

  for (Index = 0; Index < platform_gic_cfg.num_gicrd; Index++) {
    GicTable->gic_info[InfoIndex].type = PLATFORM_OVERRIDE_GICRD_TYPE;
    GicTable->gic_info[InfoIndex++].base = platform_gic_cfg.gicrd_base[Index];
  }

  for (Index = 0; Index < platform_gic_cfg.num_gicd; Index++) {
    GicTable->gic_info[InfoIndex].type = PLATFORM_OVERRIDE_GICD_TYPE;
    GicTable->gic_info[InfoIndex++].base = platform_gic_cfg.gicd_base[Index];
  }

  for (Index = 0; Index < platform_gic_cfg.num_gicits; Index++) {
    GicTable->gic_info[InfoIndex].type = PLATFORM_OVERRIDE_GICITS_TYPE;
    GicTable->gic_info[InfoIndex++].base = platform_gic_cfg.gicits_base[Index];
  }

  GicTable->gic_info[InfoIndex].type = 0xFF;  //Indicate end of data


}

/**
  @brief  Enable the interrupt in the GIC Distributor and GIC CPU Interface and hook
          the interrupt service routine for the IRQ to the Baremetal Framework

  @param  int_id  Interrupt ID which needs to be enabled and service routine installed for
  @param  isr     Function pointer of the Interrupt service routine

  @return Status of the operation
**/
UINT32
pal_gic_install_isr(UINT32 int_id,  VOID (*isr)())
{

  UINT32  Status;

  /* TO DO - Baremetal
   * Place holder to register the interrupt
   */

  return Status;
}

/**
  @brief  Indicate that processing of interrupt is complete by writing to
          End of interrupt register in the GIC CPU Interface

  @param  int_id  Interrupt ID which needs to be acknowledged that it is complete

  @return Status of the operation
**/
UINT32
pal_gic_end_of_interrupt(UINT32 int_id)
{

  UINT32  Status;

  /* TO DO - Baremetal
   * Place holder to set EOI in GICC
   */

  return Status;
}

/**
  @brief  Set Trigger type Edge/Level

  @param  int_id  Interrupt ID which needs to be enabled and service routine installed for
  @param  trigger_type  Interrupt Trigger Type Edge/Trigger

  @return Status of the operation
**/
UINT32
pal_gic_set_intr_trigger(UINT32 int_id, INTR_TRIGGER_INFO_TYPE_e trigger_type)
{

  UINT32  Status;

  /* TO Do - Baremetal
   * Place holder to set interrupt trigger type
   */

  return Status;
}
