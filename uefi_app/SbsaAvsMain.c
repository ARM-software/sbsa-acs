/** @file
 * Copyright (c) 2016-2025, Arm Limited or its affiliates. All rights reserved.
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

#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellCEntryLib.h>
#include  <Library/ShellLib.h>
#include  <Library/UefiBootServicesTableLib.h>

#include "val/sbsa/include/sbsa_val_interface.h"
#include "val/common/include/val_interface.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/common/include/acs_pe.h"
#include "val/common/include/acs_val.h"
#include "val/common/include/acs_memory.h"

#include "SbsaAvs.h"

UINT32  g_pcie_p2p;
UINT32  g_pcie_cache_present;
UINT32  g_sbsa_level;
UINT32  g_sbsa_only_level = 0;
UINT32  g_print_level;
UINT32  g_execute_nist;
UINT32  g_print_mmio = FALSE;
UINT32  g_curr_module = 0;
UINT32  g_enable_module = 0;
UINT32  *g_skip_test_num;
UINT32  g_num_skip = 0;
UINT32  *g_execute_tests;
UINT32  g_num_tests = 0;
UINT32  *g_execute_modules;
UINT32  g_num_modules = 0;
UINT32  g_acs_tests_total;
UINT32  g_acs_tests_pass;
UINT32  g_acs_tests_fail;
UINT32  g_crypto_support = TRUE;
UINT64  g_stack_pointer;
UINT64  g_exception_ret_addr;
UINT64  g_ret_addr;
UINT32  g_wakeup_timeout;
UINT32  g_sys_last_lvl_cache;
SHELL_FILE_HANDLE g_acs_log_file_handle;
/* VE systems run acs at EL1 and in some systems crash is observed during acess
   of EL1 phy and virt timer, Below command line option is added only for debug
   purpose to complete SBSA run on these systems */
UINT32  g_el1physkip = FALSE;

#define SBSA_LEVEL_PRINT_FORMAT(level, only) ((level > SBSA_MAX_LEVEL_SUPPORTED) ? \
    ((only) != 0 ? "\n Starting tests for only level FR " : "\n Starting tests for level FR ") : \
    ((only) != 0 ? "\n Starting tests for only level %2d " : "\n Starting tests for level %2d "))

STATIC VOID FlushImage (VOID)
{
  EFI_LOADED_IMAGE_PROTOCOL   *ImageInfo;
  EFI_STATUS Status;
  Status = gBS->HandleProtocol (gImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&ImageInfo);
  if(EFI_ERROR (Status))
  {
    return;
  }

  val_pe_cache_clean_range((UINT64)ImageInfo->ImageBase, (UINT64)ImageInfo->ImageSize);
}

UINT32
createPeInfoTable (
)
{
  UINT32 Status;
  UINT64 *PeInfoTable;

  PeInfoTable = val_aligned_alloc(SIZE_4K, PE_INFO_TBL_SZ);

  Status = val_pe_create_info_table(PeInfoTable);

  return Status;
}

UINT32
createGicInfoTable (
)
{
  UINT32 Status;
  UINT64 *GicInfoTable;

  GicInfoTable = val_aligned_alloc(SIZE_4K, GIC_INFO_TBL_SZ);

  Status = val_gic_create_info_table(GicInfoTable);

  return Status;
}

VOID
createTimerInfoTable(
)
{
  UINT64 *TimerInfoTable;

  TimerInfoTable = val_aligned_alloc(SIZE_4K, TIMER_INFO_TBL_SZ);

  val_timer_create_info_table(TimerInfoTable);
}

VOID
createWatchdogInfoTable(
)
{
  UINT64 *WdInfoTable;

  WdInfoTable = val_aligned_alloc(SIZE_4K, WD_INFO_TBL_SZ);

  val_wd_create_info_table(WdInfoTable);
}


VOID
createPcieVirtInfoTable(
)
{
  UINT64 *PcieInfoTable;
  UINT64 *IoVirtInfoTable;

  PcieInfoTable = val_aligned_alloc(SIZE_4K, PCIE_INFO_TBL_SZ);

  val_pcie_create_info_table(PcieInfoTable);

  IoVirtInfoTable = val_aligned_alloc(SIZE_4K, IOVIRT_INFO_TBL_SZ);

  val_iovirt_create_info_table(IoVirtInfoTable);
}

VOID
createPeripheralInfoTable(
)
{
  UINT64 *PeripheralInfoTable;
  UINT64 *MemoryInfoTable;

  PeripheralInfoTable = val_aligned_alloc(SIZE_4K, PERIPHERAL_INFO_TBL_SZ);

  val_peripheral_create_info_table(PeripheralInfoTable);

  MemoryInfoTable = val_aligned_alloc(SIZE_4K, MEM_INFO_TBL_SZ);

  val_memory_create_info_table(MemoryInfoTable);
}

VOID
createPmuInfoTable(
)
{
  UINT64 *PmuInfoTable;

  PmuInfoTable = val_aligned_alloc(SIZE_4K, PMU_INFO_TBL_SZ);

  val_pmu_create_info_table(PmuInfoTable);
}

UINT32
createRasInfoTable(
)
{
  UINT32 status;
  UINT64 *RasInfoTable;

  RasInfoTable = val_aligned_alloc(SIZE_4K, RAS_INFO_TBL_SZ);

  status = val_ras_create_info_table(RasInfoTable);

  return status;
}

VOID
createCacheInfoTable(
)
{
  UINT64 *CacheInfoTable;

  CacheInfoTable = val_aligned_alloc(SIZE_4K, CACHE_INFO_TBL_SZ);

  val_cache_create_info_table(CacheInfoTable);
}

VOID
createMpamInfoTable(
)
{
  UINT64 *MpamInfoTable;

  MpamInfoTable = val_aligned_alloc(SIZE_4K, MPAM_INFO_TBL_SZ);

  val_mpam_create_info_table(MpamInfoTable);
}

VOID
createHmatInfoTable(
)
{
  UINT64 *HmatInfoTable;

  HmatInfoTable = val_aligned_alloc(SIZE_4K, HMAT_INFO_TBL_SZ);

  val_hmat_create_info_table(HmatInfoTable);
}

VOID
createSratInfoTable(
)
{
  UINT64 *SratInfoTable;

  SratInfoTable = val_aligned_alloc(SIZE_4K, SRAT_INFO_TBL_SZ);

  val_srat_create_info_table(SratInfoTable);
}

VOID
createPccInfoTable(
)
{
  UINT64 *PccInfoTable;

  PccInfoTable = val_aligned_alloc(SIZE_4K, PCC_INFO_TBL_SZ);

  val_pcc_create_info_table(PccInfoTable);
}

VOID
createRas2InfoTable(
)
{
  UINT64 *Ras2InfoTable;

  Ras2InfoTable = val_aligned_alloc(SIZE_4K, RAS2_FEAT_INFO_TBL_SZ);

  val_ras2_create_info_table(Ras2InfoTable);
}

VOID
freeSbsaAvsMem()
{
  val_pe_free_info_table();
  val_gic_free_info_table();
  val_timer_free_info_table();
  val_wd_free_info_table();
  val_pcie_free_info_table();
  val_iovirt_free_info_table();
  val_peripheral_free_info_table();
  val_pmu_free_info_table();
  val_cache_free_info_table();
  val_mpam_free_info_table();
  val_hmat_free_info_table();
  val_srat_free_info_table();
  val_ras2_free_info_table();
  val_pcc_free_info_table();
  val_free_shared_mem();
}

VOID
HelpMsg (
  VOID
  )
{
   Print (L"\nUsage: Sbsa.efi [-v <n>] | [-l <n>] | [-only] | [-fr] | [-f <filename>] | "
         "[-skip <n>] | [-nist] | [-t <n>] | [-m <n>]\n"
         "Options:\n"
         "-v      Verbosity of the Prints\n"
         "        1 shows all prints, 5 shows Errors\n"
         "-mmio   Pass this flag to enable pal_mmio_read/write prints, use with -v 1\n"
         "        Refer to section 4 of SBSA_ACS_User_Guide\n"
         "        To skip a module, use Module ID as mentioned in user guide\n"
         "-l      Level of compliance to be tested for\n"
         "        As per SBSA specification, Valid level are 3 to 7\n"
         "-only   To only run tests belonging to a specific level of compliance\n"
         "        -l (level) or -fr option needs to be specified for using this flag\n"
         "-f      Name of the log file to record the test results in\n"
         "-fr     Should be passed without level option to run future requirement tests\n"
         "        If level option is passed, then fr option is ignored\n"
         "-skip   Test(s) to be skipped\n"
         "        Refer to section 4 of SBSA_ACS_User_Guide\n"
         "        To skip a module, use Module ID as mentioned in user guide\n"
         "        To skip a particular test within a module, use the exact testcase number\n"
         "-nist   Enable the NIST Statistical test suite\n"
         "-t      If Test ID(s) set, will only run the specified test, all others will be skipped.\n"
         "-m      If Module ID(s) set, will only run the specified module, all others will be skipped.\n"
         "-no_crypto_ext  Pass this flag if cryptography extension not supported due to export restrictions\n"
         "-p2p    Pass this flag to indicate that PCIe Hierarchy Supports Peer-to-Peer\n"
         "-cache  Pass this flag to indicate that if the test system supports PCIe address translation cache\n"
         "-timeout  Set timeout multiple for wakeup tests\n"
         "        1 - min value  5 - max value\n"
         "-slc    Provide system last level cache type\n"
         "        1 - PPTT PE-side cache,  2 - HMAT mem-side cache\n"
         "         defaults to 0, if not set depicting SLC type unknown\n"
         "-el1physkip Skips EL1 register checks\n"
  );
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v"    , TypeValue},    // -v    # Verbosity of the Prints. 1 shows all prints, 5 shows Errors
  {L"-l"    , TypeValue},    // -l    # Level of compliance to be tested for.
  {L"-only" , TypeValue},    // -only # To only run tests for a Specific level of compliance.
  {L"-f"    , TypeValue},    // -f    # Name of the log file to record the test results in.
  {L"-fr"   , TypeValue},    // -fr   # To run SBSA ACS till SBSA Future Requirement tests
  {L"-skip" , TypeValue},    // -skip # test(s) to skip execution
  {L"-help" , TypeFlag},     // -help # help : info about commands
  {L"-h"    , TypeFlag},     // -h    # help : info about commands
  {L"-nist" , TypeFlag},     // -nist # Binary Flag to enable the execution of NIST STS
  {L"-mmio" , TypeValue},    // -mmio # Enable pal_mmio prints
  {L"-t"    , TypeValue},    // -t    # Test to be run
  {L"-m"    , TypeValue},    // -m    # Module to be run
  {L"-p2p", TypeFlag},       // -p2p  # Peer-to-Peer is supported
  {L"-cache", TypeFlag},     // -cache# PCIe address translation cache is supported
  {L"-no_crypto_ext", TypeFlag},  //-no_crypto_ext # Skip tests which have export restrictions
  {L"-timeout" , TypeValue}, // -timeout # Set timeout multiple for wakeup tests
  {L"-slc"  , TypeValue},    // -slc  # system last level cache type
  {L"-el1physkip", TypeFlag}, // -el1physkip # Skips EL1 register checks
  {NULL     , TypeMax}
  };

/***
  SBSA Compliance Suite Entry Point.

  Call the Entry points of individual modules.

  @retval  0         The application exited normally.
  @retval  Other     An error occurred.
***/
INTN
EFIAPI
ShellAppMainsbsa (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{

  LIST_ENTRY         *ParamPackage;
  CONST CHAR16       *CmdLineArg;
  CHAR16             *ProbParam;
  UINT32             Status;
  UINT32             MmioVerbosity;
  UINT32             i;
  VOID               *branch_label;


  //
  // Process Command Line arguments
  //
  Status = ShellInitialize();
  Status = ShellCommandLineParse (ParamList, &ParamPackage, &ProbParam, TRUE);
  if (Status) {
    Print(L"Shell command line parse error %x\n", Status);
    Print(L"Unrecognized option %s passed\n", ProbParam);
    HelpMsg();
    return SHELL_INVALID_PARAMETER;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-skip")) {
    CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-skip");
    if (CmdLineArg == NULL)
    {
      Print(L"Invalid parameter passed for -skip\n", 0);
      HelpMsg();
      return SHELL_INVALID_PARAMETER;
    }
    else
    {
      Status = gBS->AllocatePool(EfiBootServicesData,
                                 StrLen(CmdLineArg),
                                 (VOID **) &g_skip_test_num);
      if (EFI_ERROR(Status))
      {
        Print(L"Allocate memory for -skip failed\n", 0);
        return 0;
      }

      g_skip_test_num[0] = StrDecimalToUintn((CONST CHAR16 *)(CmdLineArg+0));
      for (i = 0; i < StrLen(CmdLineArg); i++) {
      if(*(CmdLineArg+i) == L',') {
          g_skip_test_num[++g_num_skip] = StrDecimalToUintn((CONST CHAR16 *)(CmdLineArg+i+1));
        }
      }

      g_num_skip++;
    }
  }

  // Options with Values
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-l");
  if (CmdLineArg == NULL) {
    g_sbsa_level = G_SBSA_LEVEL;
    if (ShellCommandLineGetFlag (ParamPackage, L"-fr")) {
      g_sbsa_level = SBSA_MAX_LEVEL_SUPPORTED + 1;
      if (ShellCommandLineGetFlag (ParamPackage, L"-only"))
        g_sbsa_only_level = g_sbsa_level;
    }
  } else {
    g_sbsa_level = StrDecimalToUintn(CmdLineArg);
    if (g_sbsa_level > SBSA_MAX_LEVEL_SUPPORTED) {
      Print(L"SBSA Level %d is not supported.\n", g_sbsa_level);
      HelpMsg();
      return SHELL_INVALID_PARAMETER;
    }
    if (g_sbsa_level < SBSA_MIN_LEVEL_SUPPORTED) {
      Print(L"SBSA Level %d is not supported.\n", g_sbsa_level);
      HelpMsg();
      return SHELL_INVALID_PARAMETER;
    }
    if (ShellCommandLineGetFlag (ParamPackage, L"-only")) {
        g_sbsa_only_level = g_sbsa_level;
    }
  }

    // Options with Values
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-v");
  if (CmdLineArg == NULL) {
    g_print_level = G_PRINT_LEVEL;
  } else {
    g_print_level = StrDecimalToUintn(CmdLineArg);
    if (g_print_level > 5) {
      g_print_level = G_PRINT_LEVEL;
    }
  }

  // Options with Values
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-timeout");
  if (CmdLineArg == NULL) {
    g_wakeup_timeout = 1;
  } else {
    g_wakeup_timeout = StrDecimalToUintn(CmdLineArg);
    Print(L"Wakeup timeout multiple %d.\n", g_wakeup_timeout);
    if (g_wakeup_timeout > 5)
        g_wakeup_timeout = 5;
  }

    // Options with Values
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-f");
  if (CmdLineArg == NULL) {
    g_acs_log_file_handle = NULL;
  } else {
    Status = ShellOpenFileByName(CmdLineArg, &g_acs_log_file_handle,
             EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ | EFI_FILE_MODE_CREATE, 0x0);
    if(EFI_ERROR(Status)) {
         Print(L"Failed to open log file %s\n", CmdLineArg);
     g_acs_log_file_handle = NULL;
    }
  }

  /* get System Last-level cache info */
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-slc");
  if (CmdLineArg == NULL) {
    g_sys_last_lvl_cache = 0; /* default value. SLC unknown */
  } else {
    g_sys_last_lvl_cache = StrDecimalToUintn(CmdLineArg);
    if (g_sys_last_lvl_cache > 2 || g_sys_last_lvl_cache < 1) {
        Print(L"Invalid value provided for -slc, Value = %d\n", g_sys_last_lvl_cache);
        g_sys_last_lvl_cache = 0; /* default value. SLC unknown */
    }
  }

  // Options with Flags
  if ((ShellCommandLineGetFlag (ParamPackage, L"-help")) || (ShellCommandLineGetFlag (ParamPackage, L"-h"))){
     HelpMsg();
     return 0;
  }


  if (ShellCommandLineGetFlag (ParamPackage, L"-mmio")) {
    CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-mmio");
    if (CmdLineArg == NULL) {
      g_print_mmio = TRUE;
    } else {
      MmioVerbosity = StrDecimalToUintn((CONST CHAR16 *)(CmdLineArg+0));
      g_enable_module |= (1 << MmioVerbosity/100);
      for (i=0 ; i < StrLen(CmdLineArg) ; i++) {
        if (*(CmdLineArg + i) == L',') {
          MmioVerbosity = StrDecimalToUintn((CONST CHAR16 *)(CmdLineArg+i+1));
          g_enable_module |= (1 << MmioVerbosity/100);
        }
      }
    }
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-p2p")) {
    g_pcie_p2p = TRUE;
  } else {
    g_pcie_p2p = FALSE;
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-cache")) {
    g_pcie_cache_present = TRUE;
  } else {
    g_pcie_cache_present = FALSE;
  }

  // Options with Flags
  if (ShellCommandLineGetFlag (ParamPackage, L"-nist")) {
    g_execute_nist = TRUE;
  } else {
    g_execute_nist = FALSE;
  }

  // Options with Flags
  if (ShellCommandLineGetFlag (ParamPackage, L"-el1physkip")) {
    g_el1physkip = TRUE;
  }

  // Options with Flags
  if ((ShellCommandLineGetFlag (ParamPackage, L"-no_crypto_ext")))
     g_crypto_support = FALSE;

  if ((g_sbsa_level == 7) || (g_sbsa_only_level == 7))
      g_execute_nist = TRUE;

  // Options with Values
  if (ShellCommandLineGetFlag (ParamPackage, L"-t")) {
      CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-t");
      if (CmdLineArg == NULL)
      {
          Print(L"Invalid parameter passed for -t\n", 0);
          HelpMsg();
          return SHELL_INVALID_PARAMETER;
      }
      else
      {
          Status = gBS->AllocatePool(EfiBootServicesData,
                                     StrLen(CmdLineArg),
                                     (VOID **) &g_execute_tests);
          if (EFI_ERROR(Status))
          {
              Print(L"Allocate memory for -t failed\n", 0);
              return 0;
          }

          /* Check if the first value to -t is a decimal character. */
          if (!ShellIsDecimalDigitCharacter(*CmdLineArg)) {
              Print(L"Invalid parameter passed for -t\n", 0);
              HelpMsg();
              return SHELL_INVALID_PARAMETER;
          }

          g_execute_tests[0] = StrDecimalToUintn((CONST CHAR16 *)(CmdLineArg + 0));
          for (i = 0; i < StrLen(CmdLineArg); i++) {
              if (*(CmdLineArg + i) == L',') {
                  g_execute_tests[++g_num_tests] = StrDecimalToUintn(
                                                          (CONST CHAR16 *)(CmdLineArg + i + 1));
              }
          }

          g_num_tests++;
        }
  }

  // Options with Values
  if (ShellCommandLineGetFlag (ParamPackage, L"-m")) {
      CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-m");
      if (CmdLineArg == NULL)
      {
          Print(L"Invalid parameter passed for -m\n", 0);
          HelpMsg();
          return SHELL_INVALID_PARAMETER;
      }
      else
      {
          Status = gBS->AllocatePool(EfiBootServicesData,
                                     StrLen(CmdLineArg),
                                     (VOID **) &g_execute_modules);
          if (EFI_ERROR(Status))
          {
              Print(L"Allocate memory for -m failed\n", 0);
              return 0;
          }

          /* Check if the first value to -m is a decimal character. */
          if (!ShellIsDecimalDigitCharacter(*CmdLineArg)) {
              Print(L"Invalid parameter passed for -m\n", 0);
              HelpMsg();
              return SHELL_INVALID_PARAMETER;
          }

          g_execute_modules[0] = StrDecimalToUintn((CONST CHAR16 *)(CmdLineArg + 0));
          for (i = 0; i < StrLen(CmdLineArg); i++) {
              if (*(CmdLineArg + i) == L',') {
                  g_execute_modules[++g_num_modules] = StrDecimalToUintn(
                                                          (CONST CHAR16 *)(CmdLineArg + i + 1));
              }
          }

          g_num_modules++;
      }
  }

  //
  // Initialize global counters
  //
  g_acs_tests_total = 0;
  g_acs_tests_pass  = 0;
  g_acs_tests_fail  = 0;

  val_print(ACS_PRINT_TEST, "\n\n SBSA Architecture Compliance Suite\n", 0);
  val_print(ACS_PRINT_TEST, "    Version %d.", SBSA_ACS_MAJOR_VER);
  val_print(ACS_PRINT_TEST, "%d.", SBSA_ACS_MINOR_VER);
  val_print(ACS_PRINT_TEST, "%d\n", SBSA_ACS_SUBMINOR_VER);

  val_print(ACS_PRINT_TEST, SBSA_LEVEL_PRINT_FORMAT(g_sbsa_level, g_sbsa_only_level),
                                   (g_sbsa_level > SBSA_MAX_LEVEL_SUPPORTED) ? 0 : g_sbsa_level);

  val_print(ACS_PRINT_TEST, "(Print level is %2d)\n\n", g_print_level);

  val_print(ACS_PRINT_TEST, " Creating Platform Information Tables\n", 0);

  Status = createPeInfoTable();
  if (Status)
    return Status;

  Status = createGicInfoTable();
  if (Status)
    return Status;

  createTimerInfoTable();
  createWatchdogInfoTable();

  createCacheInfoTable();

  /* required before calling createMpamInfoTable() */
  createPccInfoTable();

  createMpamInfoTable();

  createHmatInfoTable();

  createSratInfoTable();

  createRas2InfoTable();

  createPcieVirtInfoTable();
  createPeripheralInfoTable();
  createPmuInfoTable();
  createRasInfoTable();

  val_allocate_shared_mem();

  // Initialise exception vector, so any unexpected exception gets handled by default SBSA exception handler
  branch_label = &&print_test_status;
  val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
  val_pe_initialize_default_exception_handler(val_pe_default_esr);
  FlushImage();

  /***         Starting PE tests                     ***/
  Status = val_sbsa_pe_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Memory tests                 ***/
  Status |= val_sbsa_memory_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting GIC tests                    ***/
  Status |= val_sbsa_gic_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting SMMU tests                   ***/
  Status |= val_sbsa_smmu_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Timer tests               ***/
  Status |= val_sbsa_timer_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Watchdog tests               ***/
  Status |= val_sbsa_wd_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting PCIe tests                   ***/
  Status |= val_sbsa_pcie_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Exerciser tests              ***/
  Status |= val_sbsa_exerciser_execute_tests(g_sbsa_level);

  /***         Starting MPAM tests                   ***/
  Status |= val_sbsa_mpam_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting PMU tests                    ***/
  Status |= val_sbsa_pmu_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting RAS tests                    ***/
  Status |= val_sbsa_ras_execute_tests(g_sbsa_level, val_pe_get_num());

#ifdef ENABLE_NIST
  /***         Starting NIST tests                   ***/
  if (g_execute_nist == TRUE) {
    Status |= val_sbsa_nist_execute_tests(g_sbsa_level, val_pe_get_num());
  }
#endif

  /***         Starting ETE tests                    ***/
  Status |= val_sbsa_ete_execute_tests(g_sbsa_level, val_pe_get_num());

print_test_status:
  val_print(ACS_PRINT_ERR, "\n     ---------------------------------------------------------\n", 0);
  val_print(ACS_PRINT_ERR, "     Total Tests run  = %4d;", g_acs_tests_total);
  val_print(ACS_PRINT_ERR, "  Tests Passed  = %4d", g_acs_tests_pass);
  val_print(ACS_PRINT_ERR, "  Tests Failed = %4d\n", g_acs_tests_fail);
  val_print(ACS_PRINT_ERR, "     ---------------------------------------------------------\n", 0);

  freeSbsaAvsMem();

  val_print(ACS_PRINT_ERR, "\n      **  For complete SBSA test coverage, it is ", 0);
  val_print(ACS_PRINT_ERR, "\n            necessary to also run the BSA test    **\n\n", 0);
  val_print(ACS_PRINT_ERR, "\n      *** SBSA tests complete. Reset the system. ***\n\n", 0);

  if(g_acs_log_file_handle) {
    ShellCloseFile(&g_acs_log_file_handle);
  }

  val_pe_context_restore(AA64WriteSp(g_stack_pointer));

  return(0);
}

#ifndef ENABLE_NIST
/***
  SBSA Compliance Suite Entry Point. This function is to
  support compilation of SBSA without NIST changes in edk2

  Call the Entry points of individual modules.

  @retval  0         The application exited normally.
  @retval  Other     An error occurred.
***/
INTN
EFIAPI
ShellAppMain(
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{
 return ShellAppMainsbsa(Argc, Argv);
}
#endif
