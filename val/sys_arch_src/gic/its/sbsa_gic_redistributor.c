/** @file
 * Copyright (c) 2021, Arm Limited or its affiliates. All rights reserved.
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

#include "sbsa_gic_its.h"


static uint64_t ConfigBase;

uint32_t
ArmGicSetItsConfigTableBase(
    uint64_t    GicDistributorBase,
    uint64_t    GicRedistributorBase
  )
{
  /* Allocate Memory for Redistributor Configuration Table */
  /* Set GICR_PROPBASER with the Config table base */

  uint32_t                Pages;
  uint32_t                ConfigTableSize;
  uint64_t                write_value;
  uint64_t                Address;
  uint32_t                gicr_propbaser_idbits;

  /* Get Memory size by reading the GICR_PROPBASER.IDBits field */
  gicr_propbaser_idbits = ARM_GICR_PROPBASER_IDbits(
                          val_mmio_read64(GicRedistributorBase + ARM_GICR_PROPBASER));
  ConfigTableSize = ((1 << (gicr_propbaser_idbits+1)) - ARM_LPI_MINID);

  Pages = SIZE_TO_PAGES(ConfigTableSize) + 1;

  Address = (uint64_t)val_aligned_alloc(SIZE_4KB, PAGES_TO_SIZE(Pages));

  if (!Address) {
    val_print(AVS_PRINT_ERR,  "ITS : Could Not get Mem Config Table. Test may not pass.\n", 0);
    return 1;
  }

  val_memory_set((void *)Address, PAGES_TO_SIZE(Pages), 0);

  write_value = val_mmio_read64(GicRedistributorBase + ARM_GICR_PROPBASER);
  write_value = write_value & (~ARM_GICR_PROPBASER_PA_MASK);
  write_value = write_value | (Address & ARM_GICR_PROPBASER_PA_MASK);

  val_mmio_write64(GicRedistributorBase + ARM_GICR_PROPBASER, write_value);

  ConfigBase = Address;

  return 0;
}


uint32_t
ArmGicSetItsPendingTableBase(
    uint64_t    GicDistributorBase,
    uint64_t    GicRedistributorBase
  )
{
  /* Allocate Memory for Pending Table for each Redistributor*/
  /* Set GICR_PENDBASER with the Config table base */

  uint32_t                Pages;
  uint32_t                PendingTableSize;
  uint64_t                write_value;
  uint32_t                gicr_propbaser_idbits;
  uint64_t                Address;


  /* Get Memory size by reading the GICD_TYPER.IDBits, GICR_PROPBASER.IDBits field */
  gicr_propbaser_idbits = ARM_GICR_PROPBASER_IDbits(
                          val_mmio_read64(GicRedistributorBase + ARM_GICR_PROPBASER));

  PendingTableSize = ((1 << (gicr_propbaser_idbits+1))/8);

  Pages = SIZE_TO_PAGES(PendingTableSize) + 1;

  Address = (uint64_t)val_aligned_alloc(SIZE_64KB, PAGES_TO_SIZE(Pages));

  if (!Address) {
    val_print(AVS_PRINT_ERR, "ITS : Could Not get Memory Pending Table. Test may not pass.\n", 0);
    return 1;
  }

  val_memory_set((void *)Address, PAGES_TO_SIZE(Pages), 0);

  write_value = val_mmio_read64(GicRedistributorBase + ARM_GICR_PENDBASER);
  write_value = write_value & (~ARM_GICR_PENDBASER_PA_MASK);
  write_value = write_value | (Address & ARM_GICR_PENDBASER_PA_MASK);

  val_mmio_write64(GicRedistributorBase + ARM_GICR_PENDBASER, write_value);

  return 0;
}


void ClearConfigTable(uint32_t IntID)
{
  val_mmio_write8(ConfigBase + (IntID - ARM_LPI_MINID), LPI_DISABLE);
}


void SetConfigTable(uint32_t IntID, uint32_t Priority)
{
  uint8_t    value;

  value = (Priority & LPI_PRIORITY_MASK) | LPI_ENABLE;
  val_mmio_write8(ConfigBase + (IntID - ARM_LPI_MINID), value);
}


void EnableLPIsRD(uint64_t GicRedistributorBase)
{
  uint32_t    value;
  value = val_mmio_read(GicRedistributorBase + ARM_GICR_CTLR);

  val_mmio_write(GicRedistributorBase + ARM_GICR_CTLR,
              (value | ARM_GICR_CTLR_ENABLE_LPIS));
}


uint32_t
ArmGicRedistributorConfigurationForLPI(
    uint64_t    GicDistributorBase,
    uint64_t    GicRedistributorBase
  )
{
  uint32_t    Status;
  /* Set Configuration Table Base */

  Status = ArmGicSetItsConfigTableBase(GicDistributorBase, GicRedistributorBase);
  if ((Status)) {
    return Status;
  }

  /* Set Pending Table Base For Each Redistributor */
  Status = ArmGicSetItsPendingTableBase(GicDistributorBase, GicRedistributorBase);
  if ((Status)) {
    return Status;
  }

  return Status;
}
