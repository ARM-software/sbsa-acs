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

extern PLATFORM_OVERRIDE_PE_INFO_TABLE platform_pe_cfg;
UINT8   *gSecondaryPeStack;
UINT64  gMpidrMax;

#define SIZE_STACK_SECONDARY_PE  0x100		//256 bytes per core
#define UPDATE_AFF_MAX(src,dest,mask)  ((dest & mask) > (src & mask) ? (dest & mask) : (src & mask))

UINT64
pal_get_madt_ptr();

VOID
ArmCallSmc (
  IN OUT ARM_SMC_ARGS *Args
  );


/**
  @brief   Return the base address of the region allocated for Stack use for the Secondary
           PEs.
  @param   None
  @return  base address of the Stack
**/
UINT64
PalGetSecondaryStackBase()
{

  return (UINT64)gSecondaryPeStack;
}

/**
  @brief   Returns the Max of each 8-bit Affinity fields in MPIDR.
  @param   None
  @return  Max MPIDR
**/
UINT64
PalGetMaxMpidr()
{

  return gMpidrMax;
}

/**
  @brief  Allocate memory region for secondary PE stack use. SIZE of stack for each PE
          is a #define

  @param  Number of PEs

  @return  None
**/
VOID
PalAllocateSecondaryStack(UINT64 mpidr)
{
  UINT32 Status;
  UINT32 NumPe, Aff0, Aff1, Aff2, Aff3;

  Aff0 = ((mpidr & 0x00000000ff) >>  0);
  Aff1 = ((mpidr & 0x000000ff00) >>  8);
  Aff2 = ((mpidr & 0x0000ff0000) >> 16);
  Aff3 = ((mpidr & 0xff00000000) >> 32);

  NumPe = ((Aff3+1) * (Aff2+1) * (Aff1+1) * (Aff0+1));

  if (gSecondaryPeStack == NULL) {

      /* TO DO - Baremetal
       * Place holder to allocate memory to gSecondaryPeStack
       */

      }
      pal_pe_data_cache_ops_by_va((UINT64)&gSecondaryPeStack, CLEAN_AND_INVALIDATE);
  }

}

/**
  @brief  This API fills in the PE_INFO Table with information about the PEs in the
          system. This is achieved by parsing the ACPI - MADT table.

  @param  PeTable  - Address where the PE information needs to be filled.

  @return  None
**/
VOID
pal_pe_create_info_table(PE_INFO_TABLE *PeTable)
{
  UINT64 MpidrAff0Max = 0;
  UINT64 MpidrAff1Max = 0;
  UINT64 MpidrAff2Max = 0;
  UINT64 MpidrAff3Max = 0;
  UINT32 PeIndex = 0;

  if (PeTable == NULL) {
    return;
  }

  PeTable->header.num_of_pe = platform_pe_cfg.num_of_pe;
  if (PeTable->header.num_of_pe == 0) {
    return;
  }

  while (PeIndex < PeTable->header.num_of_pe) {

      PeTable->pe_info[PeIndex].mpidr = platform_pe_cfg.pe_info[PeIndex].mpidr;
      PeTable->pe_info[PeIndex].pe_num = PeIndex;
      PeTable->pe_info[PeIndex].pmu_gsiv = platform_pe_cfg.pe_info[PeIndex].pmu_gsiv;
      pal_pe_data_cache_ops_by_va((UINT64)(&PeTable->pe_info[PeIndex]), CLEAN_AND_INVALIDATE);

      MpidrAff0Max = UPDATE_AFF_MAX(MpidrAff0Max, PeTable->pe_info[PeIndex].mpidr, 0x000000ff);
      MpidrAff1Max = UPDATE_AFF_MAX(MpidrAff1Max, PeTable->pe_info[PeIndex].mpidr, 0x0000ff00);
      MpidrAff2Max = UPDATE_AFF_MAX(MpidrAff2Max, PeTable->pe_info[PeIndex].mpidr, 0x00ff0000);
      MpidrAff3Max = UPDATE_AFF_MAX(MpidrAff3Max, PeTable->pe_info[PeIndex].mpidr, 0xff00000000);

      PeIndex++;
  };

  gMpidrMax = MpidrAff0Max | MpidrAff1Max | MpidrAff2Max | MpidrAff3Max;
  pal_pe_data_cache_ops_by_va((UINT64)PeTable, CLEAN_AND_INVALIDATE);
  pal_pe_data_cache_ops_by_va((UINT64)&gMpidrMax, CLEAN_AND_INVALIDATE);
  PalAllocateSecondaryStack(gMpidrMax);

}

/**
  @brief  Install Exception Handler through BAREMETAL Interrupt registration

  @param  ExceptionType  - AARCH64 Exception type
  @param  esr            - Function pointer of the exception handler

  @return status of the API
**/
UINT32
pal_pe_install_esr(UINT32 ExceptionType,  VOID (*esr)(UINT64, VOID *))
{

  UINT32  Status;

  /* TO DO - Baremetal
   * Place holder to:
   *   1. Unregister the default exception handler
   *   2. Register the handler to receive interrupts
   */

  return Status;
}

/**
  @brief  Make the SMC call using AARCH64 Assembly code
          SMC calls can take up to 7 arguments and return up to 4 return values.
          Therefore, the 4 first fields in the ARM_SMC_ARGS structure are used
          for both input and output values.

  @param  Argumets to pass to the EL3 firmware

  @return  None
**/
VOID
pal_pe_call_smc(ARM_SMC_ARGS *ArmSmcArgs)
{
  /* TO DO - Baremetal
   * Place holder to call baremetal SMC API
   */
}

VOID
ModuleEntryPoint();

/**
  @brief  Make a PSCI CPU_ON call using SMC instruction.
          Pass PAL Assembly code entry as the start vector for the PSCI ON call

  @param  Argumets to pass to the EL3 firmware

  @return  None
**/
VOID
pal_pe_execute_payload(ARM_SMC_ARGS *ArmSmcArgs)
{
  ArmSmcArgs->Arg2 = (UINT64)ModuleEntryPoint;
  pal_pe_call_smc(ArmSmcArgs);
}

/**
  @brief Update the ELR to return from exception handler to a desired address

  @param  context - exception context structure
  @param  offset - address with which ELR should be updated

  @return  None
**/
VOID
pal_pe_update_elr(VOID *context, UINT64 offset)
{
  /* TO DO - Baremetal
   * Place holder to save offset into context saving structure ELR
   */
}

/**
  @brief Get the Exception syndrome from Baremetal exception handler

  @param  context - exception context structure

  @return  ESR
**/
UINT64
pal_pe_get_esr(VOID *context)
{
  /*TO DO - Baremetal
   * Place holder to return ESR from context saving structure
   */
}

/**
  @brief Get the FAR from Baremetal exception handler

  @param  context - exception context structure

  @return  FAR
**/
UINT64
pal_pe_get_far(VOID *context)
{
  /* TO DO - Baremetal
   * Place holder to return FAR from context saving structure
   */
}

VOID
DataCacheCleanInvalidateVA(UINT64 addr);

VOID
DataCacheCleanVA(UINT64 addr);

VOID
DataCacheInvalidateVA(UINT64 addr);

/**
  @brief Perform cache maintenance operation on an address

  @param addr - address on which cache ops to be performed
  @param type - type of cache ops

  @return  None
**/
VOID
pal_pe_data_cache_ops_by_va(UINT64 addr, UINT32 type)
{
  switch(type){
      case CLEAN_AND_INVALIDATE:
          DataCacheCleanInvalidateVA(addr);
      break;
      case CLEAN:
          DataCacheCleanVA(addr);
      break;
      case INVALIDATE:
          DataCacheInvalidateVA(addr);
      break;
      default:
          DataCacheCleanInvalidateVA(addr);
  }

}
