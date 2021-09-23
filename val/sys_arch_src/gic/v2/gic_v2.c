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
#include "gic_v2.h"
#include "gic.h"
#include "sbsa_exception.h"

/**
  @brief  Acknowledges the interrupt
  @param  none
  @return none
**/
uint32_t
v2_AcknowledgeInterrupt(void)
{
  return val_mmio_read(val_get_cpuif_base() + GIC_ICCIAR);
}

/**
  @brief  Ends the interrupt
  @param  interrupt id
  @return none
**/
void
v2_EndofInterrupt(uint32_t int_id)
{
  val_mmio_write(val_get_cpuif_base() + GIC_ICCEIOR, int_id);
}

/**
  @brief  Disables the interrupt source
  @param  interrupt id
  @return none
**/
void
v2_DisableInterruptSource(uint32_t int_id)
{
  uint32_t                regOffset;
  uint32_t                regShift;

  /* Calculate register offset and bit position */
  regOffset = int_id / 32;
  regShift = int_id % 32;

  val_mmio_write(val_get_gicd_base() + GICD_ICENABLER + (4 * regOffset), 1 << regShift);
}

/**
  @brief  Enables the interrupt source
  @param  interrupt id
  @return none
**/
void
v2_EnableInterruptSource(uint32_t int_id)
{
  uint32_t                regOffset;
  uint32_t                regShift;

  /* Calculate register offset and bit position */
  regOffset = int_id / 32;
  regShift = int_id % 32;

  val_mmio_write(val_get_gicd_base() + GICD_ISENABLER + (4 * regOffset), 1 << regShift);
}

/**
  @brief  Initializes the GIC v2
  @param  none
  @return init success or failure
**/
void
v2_Init(void)
{
  uint32_t   max_num_interrupts;
  uint32_t   index;
  uint64_t   gicd_base;
  uint32_t   cpuif_base;
  uint64_t   cpuTarget;
  uint32_t   regOffset;
  uint32_t   regShift;


  /* Get the distributor base */
  gicd_base = val_get_gicd_base();

  /* Get the cpu interface base */
  cpuif_base = val_get_cpuif_base();

  /* Get the max interrupt */
  max_num_interrupts = val_get_max_intid();

  val_print(AVS_PRINT_DEBUG, "\n GIC_INIT: D base %x\n", gicd_base);
  val_print(AVS_PRINT_DEBUG, "\n GIC_INIT: CPU IF base %x\n", cpuif_base);
  val_print(AVS_PRINT_DEBUG, "\n GIC_INIT: Interrupts %d\n", max_num_interrupts);

  /* Disable all interrupt */
  for (index = 0; index < max_num_interrupts; index++) {
    v2_DisableInterruptSource(index);
  }

  /* Set vector table */
  sbsa_gic_vector_table_init();

  if (val_pe_reg_read(CurrentEL) == AARCH64_EL2) {
    /* Route exception to EL2 */
    GicWriteHcr(1 << 27);
  }

  GicClearDaif();

  /* Set default priority */
  for (index = 0; index < max_num_interrupts; index++) {
      /* Calculate register offset and bit position */
      regOffset = index / 4;
      regShift = (index % 4) * 8;
      val_mmio_write(val_get_gicd_base() + GICD_IPRIORITYR + (4 * regOffset),
                    (val_mmio_read(val_get_gicd_base() + GICD_IPRIORITYR + (4 * regOffset)) &
                     ~(0xff << regShift)) | (GIC_DEFAULT_PRIORITY << regShift));
  }

  cpuTarget = val_mmio_read(val_get_gicd_base() + GIC_ICDIPTR);

  /* Route SPI to primary PE */
  if (cpuTarget != 0) {
      for (index = 8; index < (max_num_interrupts / 4); index++) {
          val_mmio_write(gicd_base + GIC_ICDIPTR + (index * 4), cpuTarget);
      }
  }

  /* Initialize cpu interface */
  val_mmio_write(cpuif_base + GIC_ICCBPR, 0x7);
  val_mmio_write(cpuif_base + GIC_ICCPMR, 0xff);
  val_mmio_write(cpuif_base + GIC_ICCICR, 0x1);

  /* enable distributor */
  val_mmio_write(val_get_gicd_base() + GICD_CTRL, 0x1);
}
