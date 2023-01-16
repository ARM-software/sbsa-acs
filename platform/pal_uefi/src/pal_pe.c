/** @file
 * Copyright (c) 2016-2018, 2021-2023 Arm Limited or its affiliates. All rights reserved.
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
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/Cpu.h>

#include "include/pal_uefi.h"

UINT64 pal_get_pptt_ptr(void);

static   EFI_ACPI_6_1_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *gMadtHdr;
UINT8   *gSecondaryPeStack;
UINT64  gMpidrMax;
static UINT32 g_num_pe;
extern INT32 gPsciConduit;

#define SIZE_STACK_SECONDARY_PE  0x100		//256 bytes per core
#define UPDATE_AFF_MAX(src,dest,mask)  ((dest & mask) > (src & mask) ? (dest & mask) : (src & mask))

#define PPTT_STRUCT_OFFSET 0x24
#define PPTT_PE_PRIV_RES_OFFSET 0x14
#define ADD_PTR(t, p, l) ((t*)((UINT8*)p + l))
#define GET_ADDR_DIFF(a, b) ((UINT8*)a - (UINT8*)b)

UINT64
pal_get_madt_ptr();

UINT64
pal_get_fadt_ptr (
  VOID
  );

VOID
ArmCallSmc (
  IN OUT ARM_SMC_ARGS *Args,
  IN     INT32          Conduit
  );

/**
  @brief   Queries the FADT ACPI table to check whether PSCI is implemented and,
           if so, using which conduit (HVC or SMC).

  @param

  @retval  CONDUIT_UNKNOWN:       The FADT table could not be discovered.
  @retval  CONDUIT_NONE:          PSCI is not implemented
  @retval  CONDUIT_SMC:           PSCI is implemented and uses SMC as
                                  the conduit.
  @retval  CONDUIT_HVC:           PSCI is implemented and uses HVC as
                                  the conduit.
**/
INT32
pal_psci_get_conduit (
  VOID
  )
{
  EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE  *Fadt;

  Fadt = (EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE *)pal_get_fadt_ptr ();
  if (!Fadt) {
    return CONDUIT_UNKNOWN;
  } else if (!(Fadt->ArmBootArch & EFI_ACPI_6_1_ARM_PSCI_COMPLIANT)) {
    return CONDUIT_NONE;
  } else if (Fadt->ArmBootArch & EFI_ACPI_6_1_ARM_PSCI_USE_HVC) {
    return CONDUIT_HVC;
  } else {
    return CONDUIT_SMC;
  }
}

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
  @brief   Return the number of PEs in the System.
  @param   None
  @return  num_of_pe
**/
UINT32
pal_pe_get_num()
{

  return (UINT32)g_num_pe;
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
  UINT8 *Buffer;
  UINT32 NumPe, Aff0, Aff1, Aff2, Aff3, StackSize;

  Aff0 = ((mpidr & 0x00000000ff) >>  0);
  Aff1 = ((mpidr & 0x000000ff00) >>  8);
  Aff2 = ((mpidr & 0x0000ff0000) >> 16);
  Aff3 = ((mpidr & 0xff00000000) >> 32);

  NumPe = ((Aff3+1) * (Aff2+1) * (Aff1+1) * (Aff0+1));

  if (gSecondaryPeStack == NULL) {
      // AllocatePool guarantees 8b alignment, but stack pointers must be 16b
      // aligned for aarch64. Pad the size with an extra 8b so that we can
      // force-align the returned buffer to 16b. We store the original address
      // returned if we do have to align we still have the proper address to
      // free.

      StackSize = (NumPe * SIZE_STACK_SECONDARY_PE) + CPU_STACK_ALIGNMENT;
      Status = gBS->AllocatePool ( EfiBootServicesData,
                    StackSize,
                    (VOID **) &Buffer);
      if (EFI_ERROR(Status)) {
          sbsa_print(AVS_PRINT_ERR, L"\n FATAL - Allocation for Seconday stack failed %x \n", Status);
      }
      pal_pe_data_cache_ops_by_va((UINT64)&Buffer, CLEAN_AND_INVALIDATE);

      // Check if we need alignment
      if ((UINT8*)(((UINTN) Buffer) & (0xFll))) {
        // Needs alignment, so just store the original address and return +1
        ((UINTN*)Buffer)[0] = (UINTN)Buffer;
        gSecondaryPeStack = (UINT8*)(((UINTN*)Buffer)+1);
      } else {
        // None needed. Just store the address with padding and return.
        ((UINTN*)Buffer)[1] = (UINTN)Buffer;
        gSecondaryPeStack = (UINT8*)(((UINTN*)Buffer)+2);
      }
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
  UINT32                        TableLength = 0, i;
  UINT32                        Length = 0;
  UINT64                        MpidrAff0Max = 0, MpidrAff1Max = 0, MpidrAff2Max = 0, MpidrAff3Max = 0;


  if (PeTable == NULL) {
    sbsa_print(AVS_PRINT_ERR, L" Input PE Table Pointer is NULL. Cannot create PE INFO \n");
    return;
  }

  gMadtHdr = (EFI_ACPI_6_1_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *) pal_get_madt_ptr();

  if (gMadtHdr != NULL) {
    TableLength =  gMadtHdr->Header.Length;
    sbsa_print(AVS_PRINT_INFO, L" MADT is at %x and length is %x \n", gMadtHdr, TableLength);
  } else {
    sbsa_print(AVS_PRINT_ERR, L" MADT not found \n");
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
      Ptr->gmain_gsiv = Entry->VGICMaintenanceInterrupt;
      Ptr->acpi_proc_uid = Entry->AcpiProcessorUid;
      for (i = 0; i < MAX_L1_CACHE_RES; i++)
        Ptr->level_1_res[i] = DEFAULT_CACHE_IDX; //initialize cache index fields with all 1's
      sbsa_print(AVS_PRINT_DEBUG, L" MPIDR %llx PE num %x \n", Ptr->mpidr, Ptr->pe_num);
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
  g_num_pe = PeTable->header.num_of_pe;
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
  @param  Conduit  SMC or HVC

  @return  None
**/
VOID
pal_pe_call_smc(ARM_SMC_ARGS *ArmSmcArgs, INT32 Conduit)
{
  ArmCallSmc (ArmSmcArgs, Conduit);
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
  pal_pe_call_smc(ArmSmcArgs, gPsciConduit);
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

/**
  @brief  This API prints cache info table and cache entry indices for each pe.
  @param  CacheTable Pointer to cache info table.
  @param  PeTable Pointer to pe info table.
  @return None
**/
VOID
pal_cache_dump_info_table(CACHE_INFO_TABLE *CacheTable, PE_INFO_TABLE *PeTable)
{
  UINT32 i, j;
  CACHE_INFO_ENTRY *curr_entry;
  PE_INFO_ENTRY *pe_entry;
  curr_entry = CacheTable->cache_info;
  pe_entry = PeTable->pe_info;

  /*Iterate cache info table and print cache info entries*/
  for (i = 0 ; i < CacheTable->num_of_cache ; i++) {
    sbsa_print(AVS_PRINT_INFO, L"\nCache info * Index %d *", i);
    sbsa_print(AVS_PRINT_INFO, L"\n  Offset:                  0x%llx", curr_entry->my_offset);
    sbsa_print(AVS_PRINT_INFO, L"\n  Type:                    0x%llx", curr_entry->cache_type);
    sbsa_print(AVS_PRINT_INFO, L"\n  Cache ID:                0x%llx", curr_entry->cache_id);
    sbsa_print(AVS_PRINT_INFO, L"\n  Size:                    0x%llx", curr_entry->size);
    sbsa_print(AVS_PRINT_INFO, L"\n  Next level index:        %d", curr_entry->next_level_index);
    sbsa_print(AVS_PRINT_INFO, L"\n  Private flag:            0x%llx\n", curr_entry->is_private);
    curr_entry++;
  }

  sbsa_print(AVS_PRINT_INFO, L"\nPE level one cache index info");
  /*Iterate PE info table and print level one cache index info*/
  for (i = 0 ; i < PeTable->header.num_of_pe; i++) {
    sbsa_print(AVS_PRINT_INFO, L"\nPE Index * %d *", i);
    sbsa_print(AVS_PRINT_INFO, L"\n  Level 1 Cache index(s) :");

    for (j = 0; pe_entry->level_1_res[j] != DEFAULT_CACHE_IDX && j < MAX_L1_CACHE_RES; j++) {
      sbsa_print(AVS_PRINT_INFO, L" %d,", pe_entry->level_1_res[j]);
    }
    sbsa_print(AVS_PRINT_INFO, L"\n");
    pe_entry++;
  }
}

/**
  @brief  This function parses and stores cache info in cache info table
          Caller - pal_cache_create_info_table
  @param  CacheTable Pointer to cache info table.
  @param  cache_type_struct Pointer to PPTT cache structure that needs to be parsed.
  @param  offset Offset of the cache structure in PPTT ACPI table.
  @param  is_private Flag indicating whether the cache is private.
  @return Index to the cache info entry where parsed info is stored.
**/

UINT32
pal_cache_store_info(CACHE_INFO_TABLE *CacheTable,
                     EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE *cache_type_struct,
                     UINT32 offset, UINT32 is_private)
{
  CACHE_INFO_ENTRY *curr_entry;
  curr_entry = &(CacheTable->cache_info[CacheTable->num_of_cache]);
  CacheTable->num_of_cache++;

  curr_entry->my_offset = offset;
  curr_entry->flags.size_property_valid = cache_type_struct->Flags.SizePropertyValid;
  curr_entry->flags.cache_type_valid = cache_type_struct->Flags.CacheTypeValid;
  curr_entry->flags.cache_id_valid = cache_type_struct->Flags.CacheIdValid;
  curr_entry->size = cache_type_struct->Size;
  curr_entry->cache_type = cache_type_struct->Attributes.CacheType;
  curr_entry->cache_id = cache_type_struct->CacheId;
  curr_entry->is_private = is_private;

  /* set default next level index to invalid */
  curr_entry->next_level_index = CACHE_INVALID_NEXT_LVL_IDX;

  return CacheTable->num_of_cache - 1;
}

/**
  @brief  This function checks whether the cache info for a particular cache already stored.
          Caller - pal_cache_create_info_table
  @param  CacheTable Pointer to cache info table.
  @param  offset Offset of the cache structure in PPTT ACPI table.
  @param  found_index  pointer to a variable, to return index if cache info already present.
  @return 0 if cache info not present, 1 otherwise
**/
UINT32
pal_cache_find(CACHE_INFO_TABLE *CacheTable, UINT32 offset, UINT32 *found_index)
{
  CACHE_INFO_ENTRY *curr_entry;
  UINT32 i;

  curr_entry = CacheTable->cache_info;
  for (i = 0 ; i < CacheTable->num_of_cache ; i++) {
    /* match cache offset of the entry with input offset*/
    if(curr_entry->my_offset == offset) {
      *found_index = i;
      return 1;
    }
    curr_entry++;
  }
  return 0;
}

/**
  @brief  This function stores level 1 cache info entry index(s) to pe info table.
          Caller - pal_cache_create_info_table
  @param  PeTable Pointer to pe info table.
  @param  acpi_uid ACPI UID of the pe entry, to which index(s) to be stored.
  @param  cache_index index of the level 1 cache entry.
  @param  res_index private resource index of pe private cache.
  @return None
**/
VOID
pal_cache_store_pe_res(PE_INFO_TABLE *PeTable, UINT32 acpi_uid,
                       UINT32 cache_index, UINT32 res_index)
{
  PE_INFO_ENTRY *entry;
  entry = PeTable->pe_info;
  UINT32 i;

  if (res_index < MAX_L1_CACHE_RES) {
    for (i = 0 ; i < PeTable->header.num_of_pe; i++) {
      if (entry->acpi_proc_uid == acpi_uid) {
        entry->level_1_res[res_index] = cache_index;
      }
      entry++;
    }
  }
  else
    sbsa_print(AVS_PRINT_ERR,
      L"\n  The input resource index is greater than supported value %d", MAX_L1_CACHE_RES);
}


/**
  @brief  Parses ACPI PPTT table and populates the local cache info table.
          Prerequisite - pal_pe_create_info_table
  @param  CacheTable Pointer to pre-allocated memory for cache info table.
  @return None
**/

VOID
pal_cache_create_info_table(CACHE_INFO_TABLE *CacheTable, PE_INFO_TABLE *PeTable)
{
  EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER *PpttHdr;
  UINT32 TableLength = 0;
  EFI_ACPI_6_4_PPTT_STRUCTURE_HEADER *pptt_struct, *pptt_end ;
  EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR *pe_type_struct, *temp_pe_struct;
  EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE *cache_type_struct;
  UINT32 i, j, status=0;
  UINT32 offset;
  UINT32 index;
  UINT32 next_index;

  if (CacheTable == NULL) {
    sbsa_print(AVS_PRINT_ERR, L" Unable to create cache info table, input pointer is NULL \n");
    return;
  }

  /* initialize cache info table entries */
  CacheTable->num_of_cache = 0;

  PpttHdr = (EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER *) pal_get_pptt_ptr();
  if (PpttHdr == NULL) {
    sbsa_print(AVS_PRINT_ERR, L" PPTT Table not found\n");
    return;
  }
  else {
    TableLength = PpttHdr->Header.Length;
    sbsa_print(AVS_PRINT_INFO, L"PPTT table found at 0x%llx with length 0x%x\n",
               PpttHdr, TableLength);
  }

/* Pointer to first PPTT structure in PPTT ACPI table */
  pptt_struct = ADD_PTR(EFI_ACPI_6_4_PPTT_STRUCTURE_HEADER, PpttHdr, PPTT_STRUCT_OFFSET);

/* PPTT end boundary */
  pptt_end =  ADD_PTR(EFI_ACPI_6_4_PPTT_STRUCTURE_HEADER, PpttHdr, TableLength);

/* iterate PPTT structs in PPTT ACPI Table */
  while (pptt_struct < pptt_end) {
    if (pptt_struct->Type == EFI_ACPI_6_4_PPTT_TYPE_PROCESSOR) {
      pe_type_struct = (EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR *) pptt_struct;
      /* check whether the PPTT PE structure corresponds to a actual PE and not a group */
      if (pe_type_struct->Flags.NodeIsALeaf == 1) {
        /* Parse PE private cache resources*/
        for (i = 0 ; i < pe_type_struct->NumberOfPrivateResources ; i++) {
          offset = *(ADD_PTR(UINT32, pe_type_struct, PPTT_PE_PRIV_RES_OFFSET + i*4));
          cache_type_struct =  ADD_PTR(EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE, PpttHdr, offset);
          index = pal_cache_store_info(CacheTable, cache_type_struct, offset, CACHE_TYPE_PRIVATE);
          pal_cache_store_pe_res(PeTable, pe_type_struct->AcpiProcessorId, index, i);
          /* parse next level(s) of current private PE cache  */
          while(cache_type_struct->NextLevelOfCache != 0) {
            offset = cache_type_struct->NextLevelOfCache;
            cache_type_struct =  ADD_PTR(EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE, PpttHdr, offset);
            /* check if cache PPTT struct is already parsed*/
            status = pal_cache_find(CacheTable, offset, &next_index);
            /* if cache structure is already parsed update the previous cache info with index
               of found cache entry in cache_info_table, else parse the cache structure*/
            if (status) {
              CacheTable->cache_info[index].next_level_index = next_index;
              break;
            }
            else {
              CacheTable->cache_info[index].next_level_index = CacheTable->num_of_cache;
              index = pal_cache_store_info(CacheTable, cache_type_struct,
                                           offset, CACHE_TYPE_PRIVATE);
            }
          }

          /* if a cache entry is already present in info table, then it means next level cache(s)
             for that cache is already parsed in past iteration, else parse parent PE group */
          if (status) continue;
          temp_pe_struct = pe_type_struct;

          /* Keep on parsing PPTT PE group structures until root */
          while (temp_pe_struct->Parent != 0 ) {
            temp_pe_struct = ADD_PTR(EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR, PpttHdr,
                                     temp_pe_struct->Parent);
            /* If a group has cache resources parse it */
            for (j = 0 ; j < temp_pe_struct->NumberOfPrivateResources;j++) {
              offset = *(ADD_PTR(UINT32, temp_pe_struct, PPTT_PE_PRIV_RES_OFFSET + j*4));
              cache_type_struct =  ADD_PTR(EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE, PpttHdr, offset);
              /* Next level cache type should unified type(0x2 or 0x3) or same as previous type*/
              if (cache_type_struct->Attributes.CacheType > 0x1 ||
                  cache_type_struct->Attributes.CacheType ==
                  CacheTable->cache_info[index].cache_type ) {
                status = pal_cache_find(CacheTable, offset, &next_index);
                /* if cache structure is already parsed update the previous cache info with index
                   of found cache entry in cache_info_table, else parse the cache structure */
                if (status) {
                  CacheTable->cache_info[index].next_level_index = next_index;
                  break;
                }
                else {
                  CacheTable->cache_info[index].next_level_index = CacheTable->num_of_cache;
                  index = pal_cache_store_info(CacheTable, cache_type_struct, offset,
                                               CACHE_TYPE_SHARED);
                }
              }
            }
            /* If cache entry already found in info table, then it means next level cache(s)
               for that cache is already parsed in past iteration, else parse parent PE group
               of current group */
            if(status) break;
          }
        }
      }
    }
    pptt_struct = ADD_PTR(EFI_ACPI_6_4_PPTT_STRUCTURE_HEADER, pptt_struct, pptt_struct->Length);
  }
  pal_cache_dump_info_table(CacheTable, PeTable);
}


