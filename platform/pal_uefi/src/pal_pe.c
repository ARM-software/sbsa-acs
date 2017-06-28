/** @file
 * Copyright (c) 2017, ARM Limited or its affiliates. All rights reserved.

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
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "Include/IndustryStandard/Acpi61.h"
#include <Protocol/AcpiTable.h>
#include <Protocol/Cpu.h>

#include "include/pal_uefi.h"

static   EFI_ACPI_6_1_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *gMadtHdr;
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
  EFI_STATUS Status;
  UINT32 NumPe, Aff0, Aff1, Aff2, Aff3;

  Aff0 = ((mpidr & 0x00000000ff) >>  0);
  Aff1 = ((mpidr & 0x000000ff00) >>  8);
  Aff2 = ((mpidr & 0x0000ff0000) >> 16);
  Aff3 = ((mpidr & 0xff00000000) >> 32);

  NumPe = ((Aff3+1) * (Aff2+1) * (Aff1+1) * (Aff0+1));

  if (gSecondaryPeStack == NULL) {
      Status = gBS->AllocatePool ( EfiBootServicesData,
                    (NumPe * SIZE_STACK_SECONDARY_PE),
                    (VOID **) &gSecondaryPeStack);
      if (EFI_ERROR(Status)) {
          sbsa_print(AVS_PRINT_ERR, L"\n FATAL - Allocation for Seconday stack failed %x \n", Status);
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
  EFI_ACPI_6_1_GIC_STRUCTURE    *Entry = NULL;
  PE_INFO_ENTRY                 *Ptr = NULL;
  UINT32                        TableLength = 0;
  UINT32                        Length = 0;
  UINT64                        MpidrAff0Max = 0, MpidrAff1Max = 0, MpidrAff2Max = 0, MpidrAff3Max = 0;


  if (PeTable == NULL) {
    sbsa_print(AVS_PRINT_ERR, L"Input PE Table Pointer is NULL. Cannot create PE INFO \n");
    return;
  }

  gMadtHdr = (EFI_ACPI_6_1_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *) pal_get_madt_ptr();

  if (gMadtHdr != NULL) {
    TableLength =  gMadtHdr->Header.Length;
    sbsa_print(AVS_PRINT_INFO, L" MADT is at %x and length is %x \n", gMadtHdr, TableLength);
  } else {
    sbsa_print(AVS_PRINT_ERR, L"MADT not found \n");
    return;
  }

  PeTable->header.num_of_pe = 0;

  Entry = (EFI_ACPI_6_1_GIC_STRUCTURE *) (gMadtHdr + 1);
  Length = sizeof (EFI_ACPI_6_1_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);
  Ptr = PeTable->pe_info;

  do {

    if (Entry->Type == EFI_ACPI_6_1_GIC) {
      //Fill in the cpu num and the mpidr in pe info table
      Ptr->mpidr    = Entry->MPIDR;
      Ptr->pe_num   = PeTable->header.num_of_pe;
      Ptr->pmu_gsiv = Entry->PerformanceInterruptGsiv;
      sbsa_print(AVS_PRINT_DEBUG, L"MPIDR %x PE num %x \n", Ptr->mpidr, Ptr->pe_num);
      pal_pe_data_cache_ops_by_va((UINT64)Ptr, CLEAN_AND_INVALIDATE);
      Ptr++;
      PeTable->header.num_of_pe++;

      MpidrAff0Max = UPDATE_AFF_MAX(MpidrAff0Max, Entry->MPIDR, 0x000000ff);
      MpidrAff1Max = UPDATE_AFF_MAX(MpidrAff1Max, Entry->MPIDR, 0x0000ff00);
      MpidrAff2Max = UPDATE_AFF_MAX(MpidrAff2Max, Entry->MPIDR, 0x00ff0000);
      MpidrAff3Max = UPDATE_AFF_MAX(MpidrAff3Max, Entry->MPIDR, 0xff00000000);
    }

    Length += Entry->Length;
    Entry = (EFI_ACPI_6_1_GIC_STRUCTURE *) ((UINT8 *)Entry + (Entry->Length));

  }while(Length < TableLength);

  gMpidrMax = MpidrAff0Max | MpidrAff1Max | MpidrAff2Max | MpidrAff3Max;
  pal_pe_data_cache_ops_by_va((UINT64)PeTable, CLEAN_AND_INVALIDATE);
  pal_pe_data_cache_ops_by_va((UINT64)&gMpidrMax, CLEAN_AND_INVALIDATE);
  PalAllocateSecondaryStack(gMpidrMax);

}

/**
  @brief  Install Exception Handler using UEFI CPU Architecture protocol's
          Register Interrupt Handler API

  @param  ExceptionType  - AARCH64 Exception type
  @param  esr            - Function pointer of the exception handler

  @return status of the API
**/
UINT32
pal_pe_install_esr(UINT32 ExceptionType,  VOID (*esr)(UINT64, VOID *))
{

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
  ArmCallSmc (ArmSmcArgs);
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
  ((EFI_SYSTEM_CONTEXT_AARCH64*)context)->ELR = offset;
}

/**
  @brief Get the Exception syndrome from UEFI exception handler

  @param  context - exception context structure

  @return  ESR
**/
UINT64
pal_pe_get_esr(VOID *context)
{
  return ((EFI_SYSTEM_CONTEXT_AARCH64*)context)->ESR;
}

/**
  @brief Get the FAR from UEFI exception handler

  @param  context - exception context structure

  @return  FAR
**/
UINT64
pal_pe_get_far(VOID *context)
{
  return ((EFI_SYSTEM_CONTEXT_AARCH64*)context)->FAR;
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
