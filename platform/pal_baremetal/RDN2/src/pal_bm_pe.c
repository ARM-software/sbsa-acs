/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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
#include "platform_override_fvp.h"
#include "pal_common_support.h"
#include "pal_pcie_enum.h"

#ifdef ENABLE_OOB
/* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "Include/IndustryStandard/Acpi64.h"
#include <Protocol/AcpiTable.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Cpu.h>

#endif

/**
  Conduits for service calls (SMC vs HVC).
**/
#define CONDUIT_SMC       0
#define CONDUIT_HVC       1
#define CONDUIT_NONE     -2

/* Populate phy_mpid_array with mpidr value of CPUs available
 * in the system. */
static const uint64_t phy_mpidr_array[PLATFORM_OVERRIDE_PE_CNT] = {
    PLATFORM_OVERRIDE_PE0_MPIDR,
#if (PLATFORM_OVERRIDE_PE_CNT > 1)
    PLATFORM_OVERRIDE_PE1_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 2)
    PLATFORM_OVERRIDE_PE2_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 3)
    PLATFORM_OVERRIDE_PE3_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 4)
    PLATFORM_OVERRIDE_PE4_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 5)
    PLATFORM_OVERRIDE_PE5_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 6)
    PLATFORM_OVERRIDE_PE6_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 7)
    PLATFORM_OVERRIDE_PE7_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 8)
    PLATFORM_OVERRIDE_PE8_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 9)
    PLATFORM_OVERRIDE_PE9_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 10)
    PLATFORM_OVERRIDE_PE10_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 11)
    PLATFORM_OVERRIDE_PE11_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 12)
    PLATFORM_OVERRIDE_PE12_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 13)
    PLATFORM_OVERRIDE_PE13_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 14)
    PLATFORM_OVERRIDE_PE14_MPIDR,
#elif (PLATFORM_OVERRIDE_PE_CNT > 15)
    PLATFORM_OVERRIDE_PE15_MPIDR,
#endif

};

uint32_t pal_get_pe_count(void)
{
    return PLATFORM_OVERRIDE_PE_CNT;
}

uint64_t *pal_get_phy_mpidr_list_base(void)
{
    return (uint64_t *)&phy_mpidr_array[0];
}

/**
  @brief  Install Exception Handler through BAREMETAL Interrupt registration

  @param  ExceptionType  - AARCH64 Exception type
  @param  esr            - Function pointer of the exception handler

  @return status of the API
**/
uint32_t
pal_pe_install_esr(uint32_t ExceptionType,  void (*esr)(uint64_t, void *))
{

#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

   /*
   *   1. Unregister the default exception handler
   *   2. Register the handler to receive interrupts
   */
  EFI_STATUS  Status;
  EFI_CPU_ARCH_PROTOCOL   *Cpu;

  // Get the CPU protocol that this driver requires.
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&Cpu);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Unregister the default exception handler.
  Status = Cpu->RegisterInterruptHandler (Cpu, ExceptionType, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Register to receive interrupts
  Status = Cpu->RegisterInterruptHandler (Cpu, ExceptionType, (EFI_CPU_INTERRUPT_HANDLER)esr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
#else
  (void) ExceptionType;
  (void) esr;
#endif
  return 1;
}

/**
  @brief Update the ELR to return from exception handler to a desired address

  @param  context - exception context structure
  @param  offset - address with which ELR should be updated

  @return  None
**/
void
pal_pe_update_elr(void *context, uint64_t offset)
{
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */

  ((EFI_SYSTEM_CONTEXT_AARCH64*)context)->ELR = offset;
#else
  (void)context;
  (void) offset;
#endif
}

/**
  @brief Get the Exception syndrome from Baremetal exception handler

  @param  context - exception context structure

  @return  ESR
**/
uint64_t
pal_pe_get_esr(void *context)
{
  /*TO DO - Baremetal
   * Place holder to return ESR from context saving structure
   */
  (void)context;
  return 0;
}

/**
  @brief Get the FAR from Baremetal exception handler

  @param  context - exception context structure

  @return  FAR
**/
uint64_t
pal_pe_get_far(void *context)
{
  /* TO DO - Baremetal
   * Place holder to return FAR from context saving structure
   */
  (void)context;
  return 0;
}

void
DataCacheCleanInvalidateVA(uint64_t addr);

void
DataCacheCleanVA(uint64_t addr);

void
DataCacheInvalidateVA(uint64_t addr);


/**
  @brief   Checks whether PSCI is implemented if so,
           using which conduit (HVC or SMC).

  @param

  @retval  CONDUIT_NONE:          PSCI is not implemented
  @retval  CONDUIT_SMC:           PSCI is implemented and uses SMC as
                                  the conduit.
  @retval  CONDUIT_HVC:           PSCI is implemented and uses HVC as
                                  the conduit.
**/
uint32_t
pal_psci_get_conduit(void)
{
#ifdef ENABLE_OOB
  return CONDUIT_HVC;
#endif
  return CONDUIT_NONE;
}

