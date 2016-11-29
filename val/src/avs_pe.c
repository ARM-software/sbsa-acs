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
  val_data_cache_ci_va((addr_t)&g_pe_info_table);

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
  @brief   This API will execute all PE tests designated for a given compliance level
           1. Caller       -  Application layer.
           2. Prerequisite -  val_pe_create_info_table, val_allocate_shared_mem
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pe_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status;

  if (g_skip_test_num == AVS_PE_TEST_NUM_BASE) {
      val_print(AVS_PRINT_TEST, "\n USER Override - Skipping all PE tests \n", 0);
      return AVS_STATUS_SKIP;
  }

  status = c001_entry();
  status |= c002_entry(num_pe);
  status |= c003_entry(num_pe);
  status |= c004_entry(num_pe);
  status |= c005_entry(num_pe);
  status |= c006_entry(num_pe);
  status |= c007_entry(num_pe);
  status |= c008_entry(num_pe);
  status |= c009_entry(num_pe);
  status |= c010_entry(num_pe);
  status |= c011_entry(num_pe);
  status |= c012_entry(num_pe);
  status |= c013_entry(num_pe);
  status |= c014_entry(num_pe);
  status |= c015_entry(num_pe);


  if (level > 2) {
      status |= c016_entry(num_pe);
      status |= c017_entry(num_pe);
  }

  if (level > 1) {
      status |= c018_entry(num_pe);
  }

  if (status != AVS_STATUS_PASS)
      val_print(AVS_PRINT_TEST, "\n      *** One or more PE tests have failed... *** \n", 0);
  else
      val_print(AVS_PRINT_TEST, "\n      All PE tests have passed!! \n", 0);

  return status;

}

/**
  @brief   This API provides a 'C' interface to call System register reads
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   reg_id  - the system register index for which data is returned
  @return  the value read from the system register. 
**/
uint64_t
val_pe_reg_read(uint32_t reg_id)
{

  switch(reg_id) {
      case MPIDR_EL1:
          return ArmReadMpidr();
      case ID_AA64PFR0_EL1:
          return ArmReadIdPfr0();
      case ID_AA64PFR1_EL1:
          return ArmReadIdPfr1();
      case ID_AA64MMFR0_EL1:
          return AA64ReadMmfr0();
      case ID_AA64MMFR1_EL1:
          return AA64ReadMmfr1();
      case ID_AA64MMFR2_EL1:
          return AA64ReadMmfr2();
      case CTR_EL0:
          return AA64ReadCtr();
      case ID_AA64ISAR0_EL1:
          return AA64ReadIsar0();
      case ID_AA64ISAR1_EL1:
          return AA64ReadIsar1();
      case SCTLR_EL3:
          return AA64ReadSctlr3();
      case SCTLR_EL2:
          return AA64ReadSctlr2();
      case PMCR_EL0:
          return AA64ReadPmcr();
      case ID_AA64DFR0_EL1:
          return AA64ReadIdDfr0();
      case ID_AA64DFR1_EL1:
          return AA64ReadIdDfr1();
      case CurrentEL:
          return AA64ReadCurrentEL();
      case MDCR_EL2:
          return AA64ReadMdcr2();
      case CCSIDR_EL1:
          return AA64ReadCcsidr();
      case CLIDR_EL1:
          return AA64ReadClidr();
      case ID_DFR0_EL1:
          return ArmReadDfr0();
      case ID_ISAR0_EL1:
          return ArmReadIsar0();
      case ID_ISAR1_EL1:
          return ArmReadIsar1();
      case ID_ISAR2_EL1:
          return ArmReadIsar2();
      case ID_ISAR3_EL1:
          return ArmReadIsar3();
      case ID_ISAR4_EL1:
          return ArmReadIsar4();
      case ID_ISAR5_EL1:
          return ArmReadIsar5();
      case ID_MMFR0_EL1:
          return ArmReadMmfr0();
      case ID_MMFR1_EL1:
          return ArmReadMmfr1();
      case ID_MMFR2_EL1:
          return ArmReadMmfr2();
      case ID_MMFR3_EL1:
          return ArmReadMmfr3();
      case ID_MMFR4_EL1:
          return ArmReadMmfr4();
      case ID_PFR0_EL1:
          return ArmReadPfr0();
      case ID_PFR1_EL1:
          return ArmReadPfr1();
      case MIDR_EL1:
          return ArmReadMidr();
      case MVFR0_EL1:
          return ArmReadMvfr0();
      case MVFR1_EL1:
          return ArmReadMvfr1();
      case MVFR2_EL1:
          return ArmReadMvfr2();
      case PMCEID0_EL0:
          return AA64ReadPmceid0();
      case PMCEID1_EL0:
          return AA64ReadPmceid1();
      case VMPIDR_EL2:
          return AA64ReadVmpidr();
      case VPIDR_EL2:
          return AA64ReadVpidr();
      case PMBIDR_EL1:
          return AA64ReadPmbidr();
      case PMSIDR_EL1:
          return AA64ReadPmsidr();
      case LORID_EL1:
          return AA64ReadLorid();
      case ERRIDR_EL1:
          return AA64ReadErridr();
      case ERR0FR_EL1:
          return AA64ReadErr0fr();
      case ERR1FR_EL1:
          return AA64ReadErr1fr();
      case ERR2FR_EL1:
          return AA64ReadErr2fr();
      case ERR3FR_EL1:
          return AA64ReadErr3fr();
      default:
           val_report_status(255, 0x87655678);
  }

  return 0x0;

}

/**
  @brief   This API provides a 'C' interface to call System register writes
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   reg_id  - the system register index for which data is written
  @param   write_data - the 64-bit data to write to the system register
  @return  None
**/
void
val_pe_reg_write(uint32_t reg_id, uint64_t write_data)
{

  switch(reg_id) {
      case PMCR_EL0:
          AA64WritePmcr(write_data);
          break;
      case PMOVSSET_EL0:
          AA64WritePmovsset(write_data);
          break;
      case PMOVSCLR_EL0:
          AA64WritePmovsclr(write_data);
          break;
      case PMINTENSET_EL1:
          AA64WritePmintenset(write_data);
          break;
      case PMINTENCLR_EL1:
          AA64WritePmintenclr(write_data);
          break;
      case MDCR_EL2:
          AA64WriteMdcr2(write_data);
          break;
      case PMSIRR_EL1:
          AA64WritePmsirr(write_data);
          break;
      case PMSCR_EL2:
          AA64WritePmscr2(write_data);
          break;
      case PMSFCR_EL1:
          AA64WritePmsfcr(write_data);
          break;
      case PMBPTR_EL1:
          AA64WritePmbptr(write_data);
          break;
      case PMBLIMITR_EL1:
          AA64WritePmblimitr(write_data);
          break;
      default:
           val_report_status(255, 0x87655678);
  }

}

/**
  @brief   This API indicates the presence of exception level 3
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   None
  @return  1 if EL3 is present, 0 if EL3 is not implemented 
**/
uint8_t
val_is_el3_enabled()
{
  uint64_t data;
  data = val_pe_reg_read(ID_AA64PFR0_EL1);
  return ((data >> 12) & 0xF);

}

/**
  @brief   This API indicates the presence of exception level 2
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   None
  @return  1 if EL2 is present, 0 if EL2 is not implemented 
**/
uint8_t
val_is_el2_enabled()
{

  uint64_t data;
  data = val_pe_reg_read(ID_AA64PFR0_EL1);
  return ((data >> 8) & 0xF);

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

  data = val_pe_reg_read(MPIDR_EL1);
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
  @brief   This API returns the PMU Overflow Signal Interrupt ID for a given PE index
           1. Caller       -  Test Suite, VAL
           2. Prerequisite -  val_create_peinfo_table
  @param   index - the index of PE whose PMU interrupt ID is returned.
  @return  PMU interrupt id
**/
uint32_t
val_pe_get_pmu_gsiv(uint32_t index)
{

  PE_INFO_ENTRY *entry;

  if (index > g_pe_info_table->header.num_of_pe) {
        val_report_status(index, RESULT_FAIL(g_sbsa_level, 0, 0xFF));
        return 0xFFFFFF;
  }

  entry = g_pe_info_table->pe_info;

  return entry[index].pmu_gsiv;

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
val_test_entry()
{
  uint64_t test_arg;
  ARM_SMC_ARGS smc_args;
  void (*vector)(uint64_t args);
//  void (*vector)(uint64_t args) = (void *)val_get_vector(val_pe_get_index_mpid(val_pe_get_mpid()), &test_arg);

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
val_execute_on_pe(uint32_t index, void (*payload)(), uint64_t test_input)
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
val_pe_install_esr(uint32_t exception_type, void (*esr)())
{

  if (exception_type > 3) {
      val_print(AVS_PRINT_ERR, "Invalid Exception type %x \n", exception_type);
      return AVS_STATUS_ERR;
  }

  pal_pe_install_esr(exception_type, esr);

  return 0;
}

/**
  @brief   This API will call an assembly sequence with interval
           as argument over which an SPE event is exected to be generated.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   interval - The interval after completion of which SPE event 
                      would be generated
  @param   address  - Address on which to trigger the SPE
  @return  None.
**/
void
val_pe_spe_program_under_profiling(uint64_t interval, addr_t address)
{
  SpeProgramUnderProfiling(interval, address);
}

/**
  @brief   This API disables the SPE interrupt generation logic.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   None
  @return  None
**/
void
val_pe_spe_disable(void)
{ 
  DisableSpe();
}

/**
  @brief   This API will check functional behavior of endianness.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   Memory location
  @return  pass/fail status
**/
uint32_t
val_pe_bigend_check(uint64_t *ptr)
{
  return BigEndianCheck(ptr);
}
