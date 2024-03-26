/** @file
 * Copyright (c) 2016-2023, Arm Limited or its affiliates. All rights reserved.
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
#include "include/sbsa_avs_common.h"
#include "include/sbsa_std_smc.h"
#include "sys_arch_src/gic/sbsa_exception.h"

#include "include/val_interface.h"
#include "include/pal_interface.h"

int32_t gPsciConduit;

/* Global variable to store mpidr of primary PE */
uint64_t g_primary_mpidr = PAL_INVALID_MPID;

/**
  @brief   Pointer to the memory location of the PE Information table
**/
PE_INFO_TABLE *g_pe_info_table;

/**
  @brief   Pointer to the memory location of the cache Information table
**/
CACHE_INFO_TABLE *g_cache_info_table;

/**
  @brief   global structure to pass and retrieve arguments for the SMC call
**/
ARM_SMC_ARGS g_smc_args;

/* global variable to store primary PE index */
uint32_t g_primary_pe_index = 0;

/**
  @brief   This API will call PAL layer to fill in the PE information
           into the g_pe_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   pe_info_table  pre-allocated memory pointer for pe_info
  @return  Error if Input param is NULL or num_pe is 0.
**/
uint32_t
val_pe_create_info_table(uint64_t *pe_info_table)
{
  gPsciConduit = pal_psci_get_conduit();
  if (gPsciConduit == CONDUIT_UNKNOWN) {
      val_print(AVS_PRINT_WARN, " FADT not found, assuming SMC as PSCI conduit\n", 0);
      gPsciConduit = CONDUIT_SMC;
  } else if (gPsciConduit == CONDUIT_NONE) {
      val_print(AVS_PRINT_WARN, " PSCI not supported, assuming SMC as conduit for tests\n"
                                " Multi-PE and wakeup tests likely to fail\n", 0);
      gPsciConduit = CONDUIT_SMC;
  } else if (gPsciConduit == CONDUIT_HVC) {
      val_print(AVS_PRINT_INFO, " Using HVC as PSCI conduit\n", 0);
  } else {
      val_print(AVS_PRINT_INFO, " Using SMC as PSCI conduit\n", 0);
  }

  if (pe_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "Input memory for PE Info table cannot be NULL\n", 0);
      return AVS_STATUS_ERR;
  }

  g_pe_info_table = (PE_INFO_TABLE *)pe_info_table;

  pal_pe_create_info_table(g_pe_info_table);
  val_data_cache_ops_by_va((addr_t)&g_pe_info_table, CLEAN_AND_INVALIDATE);

  val_print(AVS_PRINT_TEST, " PE_INFO: Number of PE detected       : %4d\n", val_pe_get_num());

  if (val_pe_get_num() == 0) {
      val_print(AVS_PRINT_ERR, "\n *** CRITICAL ERROR: Num PE is 0x0 ***\n", 0);
      return AVS_STATUS_ERR;
  }
  /* store primary PE index for debug message printing purposes on
     multi PE tests */
  g_primary_pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  val_print(AVS_PRINT_DEBUG, " PE_INFO: Primary PE index       : %4d \n", g_primary_pe_index);
  return AVS_STATUS_PASS;
}

/**
  @brief  Free the memory allocated for the pe_info_table

  @param  None

  @return None
**/
void
val_pe_free_info_table()
{
  pal_mem_free((void *)g_pe_info_table);
}

/**
  @brief   This API returns the number of PE from the g_pe_info_table.
           1. Caller       -  Application layer, test Suite.
           2. Prerequisite -  val_pe_create_info_table.
  @param   none
  @return  the number of pe discovered
**/
uint32_t
val_pe_get_num()
{
  if (g_pe_info_table == NULL) {
      return 0;
  }
  return g_pe_info_table->header.num_of_pe;
}

/**
  @brief   This API reads MPIDR system regiser and return the Affinity bits
           1. Caller       -  Test Suite, VAL
           2. Prerequisite -  None
  @param   None
  @return  Affinity Bits of MPIDR
**/
uint64_t
val_pe_get_mpid()
{
  uint64_t data;

  #ifdef TARGET_LINUX
    data = 0;
  #else
    data = val_pe_reg_read(MPIDR_EL1);
  #endif
  /* Return the Affinity bits */
  data = data & MPIDR_AFF_MASK;
  return data;

}

/**
  @brief   This API returns the MPIDR value for the PE indicated by index
           1. Caller       -  Test Suite, VAL
           2. Prerequisite -  val_create_peinfo_table
  @param   index - the index of the PE whose mpidr value is required.
  @return  MPIDR value
**/
uint64_t
val_pe_get_mpid_index(uint32_t index)
{

  PE_INFO_ENTRY *entry;

  if (index > g_pe_info_table->header.num_of_pe) {
        val_report_status(index, RESULT_FAIL(g_sbsa_level, 0, 0xFF), NULL);
        return 0xFFFFFF;
  }

  entry = g_pe_info_table->pe_info;

  return entry[index].mpidr;

}


/**
  @brief   This API returns the index of the PE whose MPIDR matches with the input MPIDR
           1. Caller       -  Test Suite, VAL
           2. Prerequisite -  val_create_peinfo_table
  @param   mpid - the mpidr value of pE whose index is returned.
  @return  Index of PE
**/
uint32_t
val_pe_get_index_mpid(uint64_t mpid)
{

  PE_INFO_ENTRY *entry;
  uint32_t i = g_pe_info_table->header.num_of_pe;

  entry = g_pe_info_table->pe_info;

  while (i > 0) {
    val_data_cache_ops_by_va((addr_t)&entry->mpidr, INVALIDATE);
    val_data_cache_ops_by_va((addr_t)&entry->pe_num, INVALIDATE);

    if (entry->mpidr == mpid) {
      return entry->pe_num;
    }
    entry++;
    i--;
  }

  return 0x0;  //Return index 0 as a safe failsafe value
}

/**
  @brief   This API returns the index of the PE whose ACPI UID matches with the input UID
           1. Caller       -  Test Suite, VAL
           2. Prerequisite -  val_create_peinfo_table
  @param   mpid - the mpidr value of pE whose index is returned.
  @return  Index of PE
**/
uint32_t
val_pe_get_index_uid(uint32_t uid)
{

  PE_INFO_ENTRY *entry;
  uint32_t i = g_pe_info_table->header.num_of_pe;

  entry = g_pe_info_table->pe_info;

  while (i > 0) {
    if (entry->acpi_proc_uid == uid) {
      return entry->pe_num;
    }
    entry++;
    i--;
  }

  return 0x0;  //Return index 0 as a safe failsafe value
}


/**
  @brief   This API returns the ACPI UID of the PE whose MPIDR matches with the input MPIDR
           1. Caller       -  Test Suite, VAL
           2. Prerequisite -  val_create_pe_info_table
  @param   mpidr - the MPIDR value of PE whose UID is returned.
  @return  ACPI UID of the processor.
**/
uint32_t
val_pe_get_uid(uint64_t mpidr)
{

  PE_INFO_ENTRY *entry;
  uint32_t i = g_pe_info_table->header.num_of_pe;

  entry = g_pe_info_table->pe_info;

  while (i > 0) {
    if (entry->mpidr == mpidr) {
      return entry->acpi_proc_uid;
    }
    entry++;
    i--;
  }

  return INVALID_PE_INFO;
}

/**
  @brief   'C' Entry point for Secondary PE.
           Uses PSCI_CPU_OFF to switch off PE after payload execution.
           1. Caller       -  PAL code
           2. Prerequisite -  Stack pointer for this PE is setup by PAL
  @param   None
  @return  None
**/
void
val_test_entry(void)
{
  uint64_t test_arg;
  ARM_SMC_ARGS smc_args;
  void (*vector)(uint64_t args);

  val_get_test_data(val_pe_get_index_mpid(val_pe_get_mpid()), (uint64_t *)&vector, &test_arg);
  vector(test_arg);

  // We have completed our TEST code. So, switch off the PE now
  smc_args.Arg0 = ARM_SMC_ID_PSCI_CPU_OFF;
  smc_args.Arg1 = val_pe_get_mpid();
  pal_pe_call_smc(&smc_args, gPsciConduit);
}


/**
  @brief   This API initiates the execution of a test on a secondary PE.
           Uses PSCI_CPU_ON to wake a secondary PE
           1. Caller       -  Test Suite
           2. Prerequisite -  val_create_peinfo_table
  @param   index - Index of the PE to be woken up
  @param   payload - Function pointer of the test to be executed on the PE
  @param   test_input - arguments to be passed to the test.
  @return  None
**/
void
val_execute_on_pe(uint32_t index, void (*payload)(void), uint64_t test_input)
{

  int timeout = TIMEOUT_LARGE;
  if (index > g_pe_info_table->header.num_of_pe) {
      val_print(AVS_PRINT_ERR, "Input Index exceeds Num of PE %x\n", index);
      val_report_status(index, RESULT_FAIL(g_sbsa_level, 0, 0xFF), NULL);
      return;
  }

  do {
      g_smc_args.Arg0 = ARM_SMC_ID_PSCI_CPU_ON_AARCH64;

      /* Set the TEST function pointer in a shared memory location. This location is
         read by the Secondary PE (val_test_entry()) and executes the test. */
      g_smc_args.Arg1 = val_pe_get_mpid_index(index);

      val_set_test_data(index, (uint64_t)payload, test_input);
      pal_pe_execute_payload(&g_smc_args);

  } while (g_smc_args.Arg0 == (uint64_t)ARM_SMC_PSCI_RET_ALREADY_ON && timeout--);

  if (g_smc_args.Arg0 == (uint64_t)ARM_SMC_PSCI_RET_ALREADY_ON)
      val_print(AVS_PRINT_ERR, "\n       PSCI_CPU_ON: cpu already on  ", 0);
  else {
      if(g_smc_args.Arg0 == 0) {
          val_print(AVS_PRINT_INFO, "\n       PSCI_CPU_ON: success  ", 0);
          return;
      }
      else
          val_print(AVS_PRINT_ERR, "\n       PSCI_CPU_ON: failure  ", 0);

  }
  val_set_status(index, RESULT_FAIL(g_sbsa_level, 0, 0x120 - (int)g_smc_args.Arg0));
}

/**
  @brief   This API installs the Exception handler pointed
           by the function pointer to the input exception type.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   exception_type - one of the four exceptions defined by AARCH64
  @param   esr            - Function pointer of the exception handler
  @return  0 if success or ERROR for invalid Exception type.
**/
uint32_t
val_pe_install_esr(uint32_t exception_type, void (*esr)(uint64_t, void *))
{

  if (exception_type > 3) {
      val_print(AVS_PRINT_ERR, "Invalid Exception type %x\n", exception_type);
      return AVS_STATUS_ERR;
  }

#ifndef TARGET_LINUX
  if (pal_target_is_bm())
      val_gic_sbsa_install_esr(exception_type, esr);
  else
      pal_pe_install_esr(exception_type, esr);
#endif

  return 0;
}


/**
  @brief  Save context data (LR, SP and ELR in case of unexpected exception)

  @param  sp Stack Pointer
  @param  elr ELR register

  @return None
**/
void
val_pe_context_save(uint64_t sp, uint64_t elr)
{
    g_stack_pointer = sp;
    g_exception_ret_addr = elr;
    g_ret_addr = *(uint64_t *)(g_stack_pointer+8);
}

/**
  @brief  Restore context data (LR, SP for return to a known location)

  @param  sp Stack Pointer

  @return None
**/
void
val_pe_context_restore(uint64_t sp)
{
    (void) sp;
    sp = 0;
    *(uint64_t *)(g_stack_pointer+8) = g_ret_addr;
}

/**
  @brief  Initialise exception vector with the default handler

  @param  esr Exception Handler function pointer

  @return None
**/
void
val_pe_initialize_default_exception_handler(void (*esr)(uint64_t, void *))
{
    val_pe_install_esr(EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, esr);
}

/**
  @brief  Default handler which, if installed into exception vector, will be
          called in case of unexpected exceptions

  @param  interrupt_type Type of Interrupt(IRQ/FIQ/ASYNC/SERROR)
  @param  context To restore the context

  @return None
**/
void
val_pe_default_esr(uint64_t interrupt_type, void *context)
{
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    val_print(AVS_PRINT_WARN, "\n        Unexpected exception occured of type %d", interrupt_type);

#ifndef TARGET_LINUX
    if (pal_target_is_bm()) {
        val_print(AVS_PRINT_WARN, "\n        FAR reported = 0x%llx", sbsa_gic_get_far());
        val_print(AVS_PRINT_WARN, "\n        ESR reported = 0x%llx", sbsa_gic_get_esr());
    } else {
        val_print(AVS_PRINT_WARN, "\n        FAR reported = 0x%llx", val_pe_get_far(context));
        val_print(AVS_PRINT_WARN, "\n        ESR reported = 0x%llx", val_pe_get_esr(context));
    }
#endif
    val_set_status(index, RESULT_FAIL(g_sbsa_level, 0, 01));
    val_pe_update_elr(context, g_exception_ret_addr);
}

/**
  @brief  Cache clean operation on a defined address range

  @param  start_addr Start Address
  @param  length Length of the block

  @return None
**/
void
val_pe_cache_clean_range(uint64_t start_addr, uint64_t length)
{
#ifndef TARGET_LINUX
  uint64_t aligned_addr, end_addr, line_length;

  line_length = 2 << ((val_pe_reg_read(CTR_EL0) >> 16) & 0xf);
  aligned_addr = start_addr - (start_addr & (line_length-1));
  end_addr = start_addr + length;

  while(aligned_addr < end_addr){
      val_data_cache_ops_by_va(aligned_addr, CLEAN);
      aligned_addr += line_length;
  }
#endif
}

/**
  @brief   This API returns the index of primary PE on which system is booted.
           1. Caller       -  Test Suite, VAL
           2. Prerequisite -  val_create_peinfo_table
  @param   None
  @return  Primary PE index.
**/
uint32_t
val_pe_get_primary_index(void)
{
  return g_primary_pe_index;
}

/**
  @brief   This API will call PAL layer to fill in the PPTT ACPI table information
           into the g_cache_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   cache_info_table  pre-allocated memory pointer for cache info.
  @return  Error if Input parameter is NULL
**/
void
val_cache_create_info_table(uint64_t *cache_info_table)
{
  if (cache_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "\n   Pre-allocated memory pointer is NULL\n", 0);
      return;
  }

  g_cache_info_table = (CACHE_INFO_TABLE *)cache_info_table;
#ifndef TARGET_LINUX
  pal_cache_create_info_table(g_cache_info_table, g_pe_info_table);

  if (g_cache_info_table->num_of_cache != 0) {
      val_print(AVS_PRINT_TEST,
                " CACHE_INFO: Number of cache nodes    : %4d\n",
                g_cache_info_table->num_of_cache);
  }

#endif
}

/**
  @brief   This API frees the memory allocated for cache info table.
  @param   None
  @return  None
**/
void
val_cache_free_info_table(void)
{
  pal_mem_free((void *)g_cache_info_table);
}

/**
  @brief  This API returns info of the cache indexed in cache info table.
  @param type - requested info type.
  @param cache_index - index of the cache in cache info table.
  @return info value in 64-bit unsigned int if success,
          else returns INVALID_CACHE_INFO indicating failure.
**/
uint64_t
val_cache_get_info(CACHE_INFO_e type, uint32_t cache_index)
{
  CACHE_INFO_ENTRY *entry;
  char *cache_info_type[] = {"cache_type", "cache_size", "cache_identifier"};

  if (cache_index >= g_cache_info_table->num_of_cache) {
      val_print(AVS_PRINT_ERR, "\n       invalid cache index: %d", cache_index);
      return 0;
  }
  entry = &(g_cache_info_table->cache_info[cache_index]);
  switch (type) {
  case CACHE_TYPE:
      if (entry->flags.cache_type_valid)
          return entry->cache_type;
      break;
  case CACHE_SIZE:
      if (entry->flags.size_property_valid)
          return entry->size;
      break;
  case CACHE_ID:
      if (entry->flags.cache_id_valid)
          return entry->cache_id;
      break;
  case CACHE_NEXT_LEVEL_IDX:
      return entry->next_level_index;
  case CACHE_PRIVATE_FLAG:
      return entry->is_private;
  default:
      val_print(AVS_PRINT_ERR,
                "\n      cache option not supported %d\n", type);
      return INVALID_CACHE_INFO;
  }

  val_print(AVS_PRINT_ERR,
   "\n       cache %d has invalid ", cache_index);
  val_print(AVS_PRINT_ERR, cache_info_type[type], 0);
  return INVALID_CACHE_INFO;
}

/**
  @brief  This API returns index of last-level cache in cache info table
          for the current PE.

  @return index of the last-level cache.
**/
uint32_t
val_cache_get_llc_index(void)
{
  uint32_t curr_cache_idx;
  uint32_t next_lvl_idx;
  uint32_t llc_idx = CACHE_INVALID_IDX;
  if (g_cache_info_table->num_of_cache) {
      /* get first level private cache index for current PE */
      /* setting res_index to 0 since PE should have atleast one L1 cache */
      curr_cache_idx = val_cache_get_pe_l1_cache_res(0);

      /* get to last level cache in the cache info chain */
      while (curr_cache_idx != CACHE_INVALID_NEXT_LVL_IDX) {
        /* check if next level cache is present */
        next_lvl_idx = val_cache_get_info(CACHE_NEXT_LEVEL_IDX, curr_cache_idx);
        if (next_lvl_idx == CACHE_INVALID_NEXT_LVL_IDX) {
            llc_idx = curr_cache_idx;
            break;
        }
        else
            curr_cache_idx = next_lvl_idx;
      }

      return llc_idx;
  }
  else {
      val_print(AVS_PRINT_DEBUG, "\n       CACHE INFO table invalid", 0);
      return CACHE_TABLE_EMPTY;
  }
}

/**
  @brief  This API returns level 1 cache index for resource index requested.
  @param res_index level 1 private resource index.
  @return return index of cache in the cache info table.
**/
uint32_t
val_cache_get_pe_l1_cache_res(uint32_t res_index)
{
  PE_INFO_ENTRY *entry;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  entry = &(g_pe_info_table->pe_info[index]);
  if (res_index < MAX_L1_CACHE_RES)
      return entry->level_1_res[res_index];
  else {
      val_print(AVS_PRINT_ERR,
               "\n   Requested resource index exceeds maximum index value %d\n", MAX_L1_CACHE_RES);
      return DEFAULT_CACHE_IDX;
  }
}

#ifdef TARGET_BM_BOOT
/**
 *   @brief    Returns mpidr of primary cpu set during boot.
 *   @param    void
 *   @return   primary mpidr
**/
uint64_t val_get_primary_mpidr(void)
{
    return g_primary_mpidr;
}

/**
 *   @brief    Convert mpidr to logical cpu number
 *   @param    mpidr    - mpidr value
 *   @return   Logical cpu number
**/
// This API is only used for baremetal boot at which point PE info table is not yet created.
uint32_t val_get_pe_id(uint64_t mpidr)
{
    uint32_t pe_index = 0;
    uint32_t total_pe_num = pal_get_pe_count();
    uint64_t *phy_mpidr_list = pal_get_phy_mpidr_list_base();

    mpidr = mpidr & PAL_MPIDR_AFFINITY_MASK;

    for (pe_index = 0; pe_index < total_pe_num; pe_index++)
    {
        if (mpidr == phy_mpidr_list[pe_index])
            return pe_index;
    }

    /* In case virtual mpidr returned for realm */
    for (pe_index = 0; pe_index < total_pe_num; pe_index++)
    {
        if (mpidr == pe_index)
            return pe_index;
    }

    return PAL_INVALID_MPID;
}
#endif //TARGET_BM_BOOT
