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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_gic.h"
#include "include/sbsa_avs_gic_support.h"
#include "include/sbsa_avs_pe.h"
#include "gic_v3.h"
#include "gic.h"
#include "sbsa_exception.h"

/**
  @brief  API used to clear espi interrupt
  @param  interrupt
  @return none
**/
void v3_clear_extended_spi_interrupt(uint32_t int_id)
{
  uint32_t reg_offset = (int_id - EXTENDED_SPI_START_INTID) / 32;
  uint32_t reg_shift  = (int_id - EXTENDED_SPI_START_INTID) % 32;

  val_mmio_write(val_get_gicd_base() + GICD_ICPENDRE0 + (4 * reg_offset), (1 << reg_shift));
  val_mmio_write(val_get_gicd_base() + GICD_ICACTIVERE0 + (4 * reg_offset), (1 << reg_shift));
}

/**
  @brief  checks if given int id is espi
  @param  int_id
  @return true if ESPI
**/
uint32_t
v3_is_extended_spi(uint32_t int_id)
{
  if (int_id >= EXTENDED_SPI_START_INTID && int_id <= val_sbsa_gic_max_espi_val())
      return 1;
  else
      return 0;
}

/**
  @brief  checks if given int id is eppi
  @param  int_id
  @return true if EPPI
**/
uint32_t
v3_is_extended_ppi(uint32_t int_id)
{
  if (int_id >= EXTENDED_PPI_START_INTID && int_id <= val_sbsa_gic_max_eppi_val())
      return 1;
  else
      return 0;
}

/**
  @brief  Disables the interrupt source
  @param  interrupt id
  @return none
**/
void
v3_disable_extended_interrupt_source(uint32_t int_id)
{
  uint32_t                regOffset;
  uint32_t                regShift;
  uint64_t                cpuRd_base;

  if (v3_is_extended_spi(int_id)) {
      /* Calculate register offset and bit position */
      regOffset = (int_id - EXTENDED_SPI_START_INTID) / 32;
      regShift = (int_id - EXTENDED_SPI_START_INTID) % 32;
      val_mmio_write(val_get_gicd_base() + GICD_ICENABLERE + (4 * regOffset), 1 << regShift);
  } else {
      /* Calculate register offset and bit position */
      regOffset = (int_id - EXTENDED_PPI_REG_OFFSET) / 32;
      regShift = (int_id - EXTENDED_PPI_REG_OFFSET) % 32;
      cpuRd_base = v3_get_pe_gicr_base();
      if (cpuRd_base == 0) {
        return;
      }
      val_mmio_write(cpuRd_base + GICR_CTLR_FRAME_SIZE + GICR_ICENABLER + (4 * regOffset),
                   1 << regShift);
  }
}

/**
  @brief  Enables the interrupt source
  @param  interrupt id
  @return none
**/
void
v3_enable_extended_interrupt_source(uint32_t int_id)
{
  uint32_t                regOffset;
  uint32_t                regShift;
  uint64_t                cpuRd_base;

  if (v3_is_extended_spi(int_id)) {
      /* Calculate register offset and bit position */
      regOffset = (int_id - EXTENDED_SPI_START_INTID) / 32;
      regShift = (int_id - EXTENDED_SPI_START_INTID) % 32;
      val_mmio_write(val_get_gicd_base() + GICD_ICENABLERE + (4 * regOffset), 1 << regShift);
  } else {
      /* Calculate register offset and bit position */
      regOffset = (int_id - EXTENDED_PPI_REG_OFFSET) / 32;
      regShift = (int_id - EXTENDED_PPI_REG_OFFSET) % 32;
      cpuRd_base = v3_get_pe_gicr_base();
      if (cpuRd_base == 0) {
        return;
      }
      val_mmio_write(cpuRd_base + GICR_CTLR_FRAME_SIZE + GICR_ISENABLER + (4 * regOffset),
                   1 << regShift);
  }
}

/**
  @brief  Sets interrupt priority
  @param  interrupt id
  @param  priority
  @return none
**/
void
v3_set_extended_interrupt_priority(uint32_t int_id, uint32_t priority)
{
  uint32_t                regOffset;
  uint32_t                regShift;
  uint64_t                cpuRd_base;

  if (v3_is_extended_spi(int_id)) {
      /* Calculate register offset and bit position */
      regOffset = (int_id - EXTENDED_SPI_START_INTID) / 4;
      regShift = ((int_id - EXTENDED_SPI_START_INTID) % 4) * 8;

      val_mmio_write(val_get_gicd_base() + GICD_IPRIORITYRE + (4 * regOffset),
                    (val_mmio_read(val_get_gicd_base() + GICD_IPRIORITYRE + (4 * regOffset)) &
                     ~(0xff << regShift)) | priority << regShift);
  } else {
     /* Calculate register offset and bit position */
    regOffset = (int_id - EXTENDED_PPI_REG_OFFSET) / 4;
    regShift = ((int_id - EXTENDED_PPI_REG_OFFSET) % 4) * 8;

    cpuRd_base = v3_get_pe_gicr_base();
    if (cpuRd_base == 0) {
      return;
    }
    val_mmio_write(cpuRd_base + GICR_IPRIORITYR + (4 * regOffset),
                  (val_mmio_read(cpuRd_base + GICR_IPRIORITYR + (4 * regOffset)) &
                   ~(0xff << regShift)) | priority << regShift);
  }
}

/**
  @brief  Route interrupt to primary PE
  @param  interrupt id
  @return none
**/
void
v3_route_extended_interrupt(uint32_t int_id)
{
  uint64_t   gicd_base;
  uint64_t   cpuTarget;
  uint64_t   Mpidr;

  /* Get the distributor base */
  gicd_base = val_get_gicd_base();

  Mpidr = ArmReadMpidr();
  cpuTarget = Mpidr & (PE_AFF0 | PE_AFF1 | PE_AFF2 | PE_AFF3);

  val_mmio_write64(gicd_base + GICD_IROUTERn + (int_id * 8), cpuTarget);
}

/**
  @brief  Initializes the GIC v3 Extended Interrupts
  @param  none
  @return init success or failure
**/
void
v3_extended_init(void)
{
  uint32_t   max_num_espi_interrupts;
  uint32_t   max_num_eppi_interrupts;
  uint32_t   index;

  /* Get the max interrupt */
  max_num_espi_interrupts = val_sbsa_gic_max_espi_val();
  max_num_eppi_interrupts = val_sbsa_gic_max_eppi_val();

  val_print(AVS_PRINT_DEBUG, "\n GIC_INIT: Extended SPI Interrupts %d\n", max_num_espi_interrupts);
  val_print(AVS_PRINT_DEBUG, "\n GIC_INIT: Extended PPI Interrupts %d\n", max_num_eppi_interrupts);

  /* Disable all ESPI interrupt */
  for (index = EXTENDED_SPI_START_INTID; index <= max_num_espi_interrupts; index++) {
      v3_disable_extended_interrupt_source(index);
  }

  /* Disable all EPPI interrupt */
  for (index = EXTENDED_PPI_START_INTID; index <= max_num_eppi_interrupts; index++) {
      v3_disable_extended_interrupt_source(index);
  }

  /* Set default for ESPI priority */
  for (index = EXTENDED_SPI_START_INTID; index <= max_num_espi_interrupts; index++) {
      v3_set_extended_interrupt_priority(index, GIC_DEFAULT_PRIORITY);
  }

  /* Set default for EPPI priority */
  for (index = EXTENDED_PPI_START_INTID; index <= max_num_eppi_interrupts; index++) {
      v3_set_extended_interrupt_priority(index, GIC_DEFAULT_PRIORITY);
  }

  /* Route ESPI to primary PE */
  for (index = EXTENDED_SPI_START_INTID; index <= (max_num_espi_interrupts); index++) {
      v3_route_extended_interrupt(index);
  }

}
