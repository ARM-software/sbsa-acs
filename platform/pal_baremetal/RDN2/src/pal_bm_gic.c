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

#include "pal_common_support.h"

#ifdef ENABLE_OOB
/* Below code is not applicable for Bare-metal
 * Only for FVP OOB experience
 */
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "Include/IndustryStandard/Acpi61.h"
#include <Protocol/AcpiTable.h>
#include <Protocol/HardwareInterrupt.h>
#include <Protocol/HardwareInterrupt2.h>


EFI_HARDWARE_INTERRUPT_PROTOCOL *gInterrupt = NULL;
EFI_HARDWARE_INTERRUPT2_PROTOCOL *gInterrupt2 = NULL;

#endif

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

#ifdef ENABLE_OOB
  /* Below code is not applicable for Bare-metal
   * Only for FVP OOB experience
   */

  EFI_STATUS  Status;

 // Find the interrupt controller protocol.
  Status = gBS->LocateProtocol (&gHardwareInterruptProtocolGuid, NULL, (VOID **)&gInterrupt);
  if (EFI_ERROR(Status)) {
    return 0xFFFFFFFF;
  }

  //First disable the interrupt to enable a clean handoff to our Interrupt handler.
  gInterrupt->DisableInterruptSource(gInterrupt, int_id);

  //Register our handler
  Status = gInterrupt->RegisterInterruptSource (gInterrupt, int_id, isr);
  if (EFI_ERROR(Status)) {
    Status =  gInterrupt->RegisterInterruptSource (gInterrupt, int_id, NULL);  //Deregister existing handler
    Status = gInterrupt->RegisterInterruptSource (gInterrupt, int_id, isr);  //register our Handler.
    //Even if this fails. there is nothing we can do in UEFI mode
  }

#else
  (void) int_id;
  (void) isr;
#endif

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

#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

 EFI_STATUS  Status;

 // Find the interrupt controller protocol.
  Status = gBS->LocateProtocol (&gHardwareInterruptProtocolGuid, NULL, (VOID **)&gInterrupt);
  if (EFI_ERROR(Status)) {
    return 0xFFFFFFFF;
  }

  //EndOfInterrupt.
  gInterrupt->EndOfInterrupt(gInterrupt, int_id);
#else
  (void) int_id;
#endif

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
  (void) IrqNum;
  (void) MappedIrqNum;
  (void) Isr;
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
  (void) IrqNum;
  (void) MappedIrqNum;
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
  (void) int_id;
  (void) trigger_type;
  return 0;
}
