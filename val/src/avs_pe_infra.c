/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

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


/**
  @brief   Pointer to the memory location of the PE Information table
**/
PE_INFO_TABLE *g_pe_info_table;
/**
  @brief   global structure to pass and retrieve arguments for the SMC call
**/
ARM_SMC_ARGS g_smc_args;


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

  if (pe_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "Input memory for PE Info table cannot be NULL \n", 0);
      return AVS_STATUS_ERR;
  }

  g_pe_info_table = (PE_INFO_TABLE *)pe_info_table;

  pal_pe_create_info_table(g_pe_info_table);
  val_data_cache_ops_by_va((addr_t)&g_pe_info_table, CLEAN_AND_INVALIDATE);

  val_print(AVS_PRINT_TEST, " PE_INFO: Number of PE detected       : %4d \n", val_pe_get_num());

  if (val_pe_get_num() == 0) {
      val_print(AVS_PRINT_ERR, "\n *** CRITICAL ERROR: Num PE is 0x0 ***\n", 0);
      return AVS_STATUS_ERR;
  }
  return AVS_STATUS_PASS;
}

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
  @brief   This API reads MPIDR system regiser and return the affx bits
           1. Caller       -  Test Suite, VAL
           2. Prerequisite -  None
  @param   None
  @return  32-bit Affinity value
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
  //
  //return the affx bits
  //
  data = (((data >> 32) & 0xFF) << 24) | (data & 0xFFFFFF);
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
        val_report_status(index, RESULT_FAIL(g_sbsa_level, 0, 0xFF));
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
    if (entry->mpidr == mpid) {
      return entry->pe_num;
    }
    entry++;
    i--;
  }

  return 0x0;  //Return index 0 as a safe failsafe value
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
  pal_pe_call_smc(&smc_args);
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

  if (index > g_pe_info_table->header.num_of_pe) {
      val_print(AVS_PRINT_ERR, "Input Index exceeds Num of PE %x \n", index);
      val_report_status(index, RESULT_FAIL(g_sbsa_level, 0, 0xFF));
      return;
  }


  g_smc_args.Arg0 = ARM_SMC_ID_PSCI_CPU_ON_AARCH64;

  /* Set the TEST function pointer in a shared memory location. This location is 
     read by the Secondary PE (val_test_entry()) and executes the test. */
  g_smc_args.Arg1 = val_pe_get_mpid_index(index);

  val_set_test_data(index, (uint64_t)payload, test_input);
  pal_pe_execute_payload(&g_smc_args);

  if(g_smc_args.Arg0 == 0)
      val_print(AVS_PRINT_INFO, "       PSCI status is success  \n", 0);
  else if(g_smc_args.Arg0 == (uint64_t)ARM_SMC_PSCI_RET_ALREADY_ON){
      val_print(AVS_PRINT_INFO, "       PSCI status is already on  \n", 0);
      val_set_status(index, RESULT_FAIL(g_sbsa_level, 0, 0x124));
      return;
  }

}
