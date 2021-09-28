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
#include "include/sbsa_avs_pe.h"
#include "gic_v3.h"
#include "gic_v2.h"

/**
  @brief  Initializes the GIC
  @param  none
  @return init success or failure
**/
void
val_sbsa_gic_init(void)
{
  uint32_t gic_version;

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  if ((gic_version == 3) || (gic_version == 4))
      v3_Init();
  else
      v2_Init();
}

/**
  @brief  Enable Interrupt Source
  @param  interrupt id
  @return none
**/
void
val_sbsa_gic_enableInterruptSource(uint32_t int_id)
{
  uint32_t gic_version;

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  if ((gic_version == 3) || (gic_version == 4))
      v3_EnableInterruptSource(int_id);
  else
      v2_EnableInterruptSource(int_id);
}

/**
  @brief  Disable Interrupt Source
  @param  interrupt id
  @return none
**/
void
val_sbsa_gic_disableInterruptSource(uint32_t int_id)
{
  uint32_t gic_version;

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  if ((gic_version == 3) || (gic_version == 4))
      v3_DisableInterruptSource(int_id);
  else
      v2_DisableInterruptSource(int_id);
}

/**
  @brief  Acknowledges interrupt
  @param  none
  @return none
**/
uint32_t
val_sbsa_gic_acknowledgeInterrupt(void)
{
  uint32_t gic_version;

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  if ((gic_version == 3) || (gic_version == 4))
      return v3_AcknowledgeInterrupt();
  else
      return v2_AcknowledgeInterrupt();
}

/**
  @brief  End of interrupt
  @param  interrupt
  @return none
**/
void
val_sbsa_gic_endofInterrupt(uint32_t int_id)
{
  uint32_t gic_version;

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  if ((gic_version == 3) || (gic_version == 4))
      v3_EndofInterrupt(int_id);
  else
      v2_EndofInterrupt(int_id);
}

/**
  @brief  API used to find extended SPI support in system.
  @param  none
  @return 0 if not supported, 1 supported
**/
uint32_t
val_sbsa_gic_espi_support(void)
{
  uint32_t gic_version;

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  if (gic_version >= 3)
      return (v3_read_gicdTyper() >> GICD_TYPER_ESPI_SHIFT) & GICD_TYPER_ESPI_MASK;
  else
      return 0;
}

/**
  @brief  API used to find  max espi value
  @param  none
  @return max espi value
**/
uint32_t
val_sbsa_gic_max_espi_val(void)
{
  uint32_t gic_version;
  uint32_t espi_range;

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  if (gic_version >= 3) {
      espi_range = (v3_read_gicdTyper() >> GICD_TYPER_ESPI_RANGE_SHIFT) &
                                                                    GICD_TYPER_ESPI_RANGE_MASK;
      return (32 * (espi_range + 1) + 4095);
  }
  else
      return 0;
}


/**
  @brief  API used to check whether int_id is a espi interrupt
  @param  interrupt
  @return 1: espi interrupt, 0: non-espi interrupt
**/
uint32_t
val_sbsa_gic_check_espi_interrupt(uint32_t int_id)
{
  if (val_sbsa_gic_espi_support() && v3_is_extended_spi(int_id))
    return 1;
  else
    return 0;
}

/**
  @brief  API used to clear espi interrupt
  @param  interrupt
  @return none
**/
void val_sbsa_gic_clear_espi_interrupt(uint32_t int_id)
{
  v3_clear_extended_spi_interrupt(int_id);
}


/**
  @brief  API used to find extended PPI support in system.
  @param  none
  @return 0 if not supported, 1 supported
**/
uint32_t
val_sbsa_gic_eppi_support(void)
{
  uint32_t gic_version;

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  if (gic_version >= 3) {
      if ((v3_read_gicr_typer() >> GICD_TYPER_EPPI_NUM_SHIFT) & GICD_TYPER_EPPI_NUM_MASK)
        return 1;
      else
         return 0;
  }
  else
      return 0;
}

/**
  @brief  API used to find  max eppi value
  @param  none
  @return max eppi value
**/
uint32_t
val_sbsa_gic_max_eppi_val(void)
{
  uint32_t gic_version;
  uint32_t ppi_range;

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  if (gic_version >= 3) {
      ppi_range = ((v3_read_gicr_typer() >> GICD_TYPER_EPPI_NUM_SHIFT) & GICD_TYPER_EPPI_NUM_MASK);
      if (ppi_range == 1)
        return 1087;
      else if (ppi_range == 2)
        return 1119;
      else
        return 0; /* return 0 if EPPI not supported*/
   }
  else
      return 0;
}

/**
  @brief  API used to check whether int_id is a eppi interrupt
  @param  interrupt
  @return 1: eppi interrupt
**/
uint32_t
val_sbsa_gic_check_eppi_interrupt(uint32_t int_id)
{
  if (val_sbsa_gic_eppi_support() && v3_is_extended_ppi(int_id))
    return 1;
  else
    return 0;
}
