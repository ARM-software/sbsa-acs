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

#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellCEntryLib.h>
#include  <Library/ShellLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/CacheMaintenanceLib.h>
#include  <Protocol/LoadedImage.h>

#include "val/include/val_interface.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_val.h"

#include "SbsaAvs.h"

UINT32 g_pcie_p2p;
UINT32 g_pcie_cache_present;

UINT32  g_sbsa_level;
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
UINT32  g_sbsa_tests_total;
UINT32  g_sbsa_tests_pass;
UINT32  g_sbsa_tests_fail;
UINT64  g_stack_pointer;
UINT64  g_exception_ret_addr;
UINT64  g_ret_addr;
UINT32  g_wakeup_timeout;
SHELL_FILE_HANDLE g_sbsa_log_file_handle;

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

EFI_STATUS
createPeInfoTable (
)
{

  EFI_STATUS Status;

  UINT64   *PeInfoTable;

/* allowing room for growth, at present each entry is 16 bytes, so we can support upto 511 PEs with 8192 bytes*/
  Status = gBS->AllocatePool ( EfiBootServicesData,
                               PE_INFO_TBL_SZ,
                               (VOID **) &PeInfoTable );

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }

  Status = val_pe_create_info_table(PeInfoTable);

  return Status;

}

EFI_STATUS
createGicInfoTable (
)
{
  EFI_STATUS Status;
  UINT64     *GicInfoTable;

  Status = gBS->AllocatePool (EfiBootServicesData,
                               GIC_INFO_TBL_SZ,
                               (VOID **) &GicInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }

  Status = val_gic_create_info_table(GicInfoTable);

  return Status;

}

EFI_STATUS
createTimerInfoTable(
)
{
  UINT64   *TimerInfoTable;
  EFI_STATUS Status;

  Status = gBS->AllocatePool (EfiBootServicesData,
                              TIMER_INFO_TBL_SZ,
                              (VOID **) &TimerInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_timer_create_info_table(TimerInfoTable);

  return Status;
}

EFI_STATUS
createWatchdogInfoTable(
)
{
  UINT64   *WdInfoTable;
  EFI_STATUS Status;

  Status = gBS->AllocatePool (EfiBootServicesData,
                              WD_INFO_TBL_SZ,
                              (VOID **) &WdInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_wd_create_info_table(WdInfoTable);

  return Status;

}


EFI_STATUS
createPcieVirtInfoTable(
)
{
  UINT64   *PcieInfoTable;
  UINT64   *IoVirtInfoTable;

  EFI_STATUS Status;

  Status = gBS->AllocatePool (EfiBootServicesData,
                              PCIE_INFO_TBL_SZ,
                              (VOID **) &PcieInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_pcie_create_info_table(PcieInfoTable);

  Status = gBS->AllocatePool (EfiBootServicesData,
                              IOVIRT_INFO_TBL_SZ,
                              (VOID **) &IoVirtInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_iovirt_create_info_table(IoVirtInfoTable);

  return Status;
}

EFI_STATUS
createPeripheralInfoTable(
)
{
  UINT64   *PeripheralInfoTable;
  UINT64   *MemoryInfoTable;

  EFI_STATUS Status;

  Status = gBS->AllocatePool (EfiBootServicesData,
                              PERIPHERAL_INFO_TBL_SZ,
                              (VOID **) &PeripheralInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_peripheral_create_info_table(PeripheralInfoTable);

  Status = gBS->AllocatePool (EfiBootServicesData,
                              MEM_INFO_TBL_SZ,
                              (VOID **) &MemoryInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }

  val_memory_create_info_table(MemoryInfoTable);

  return Status;
}

EFI_STATUS
createPmuInfoTable(
)
{
  UINT64   *PmuInfoTable;
  EFI_STATUS Status;

  Status = gBS->AllocatePool (EfiBootServicesData,
                              PMU_INFO_TBL_SZ,
                              (VOID **) &PmuInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_pmu_create_info_table(PmuInfoTable);

  return Status;

}

EFI_STATUS
createRasInfoTable(
)
{
  UINT64   *RasInfoTable;
  EFI_STATUS Status;

  Status = gBS->AllocatePool(EfiBootServicesData,
                             RAS_INFO_TBL_SZ,
                             (VOID **) &RasInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_ras_create_info_table(RasInfoTable);

  return Status;

}

EFI_STATUS
createCacheInfoTable(
)
{
  UINT64   *CacheInfoTable;
  EFI_STATUS Status;

  Status = gBS->AllocatePool(EfiBootServicesData,
                              CACHE_INFO_TBL_SZ,
                              (VOID **) &CacheInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_cache_create_info_table(CacheInfoTable);

  return Status;
}

EFI_STATUS
createMpamInfoTable(
)
{
  UINT64      *MpamInfoTable;
  EFI_STATUS  Status;

  Status = gBS->AllocatePool(EfiBootServicesData,
                              MPAM_INFO_TBL_SZ,
                              (VOID **) &MpamInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_mpam_create_info_table(MpamInfoTable);

  return Status;
}

EFI_STATUS
createHmatInfoTable(
)
{
  UINT64      *HmatInfoTable;
  EFI_STATUS  Status;

  Status = gBS->AllocatePool(EfiBootServicesData,
                              HMAT_INFO_TBL_SZ,
                              (VOID **) &HmatInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_hmat_create_info_table(HmatInfoTable);

  return Status;
}

EFI_STATUS
createSratInfoTable(
)
{
  UINT64      *SratInfoTable;
  EFI_STATUS  Status;

  Status = gBS->AllocatePool(EfiBootServicesData,
                              SRAT_INFO_TBL_SZ,
                              (VOID **) &SratInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_srat_create_info_table(SratInfoTable);

  return Status;
}

/**
  @brief  This API allocates memory for info table and
          calls create info table function passed as parameter.

  @param  create_info_tbl_func  - function pointer to val_*_create_info_table
  @param  info_table_size       - memory size to be allocated.

  @return  None
**/

EFI_STATUS
createInfoTable(
  VOID(*create_info_tbl_func)(UINT64 *),
  UINT64 info_table_size,
  CHAR8 *table_name
  )
{
  UINT64      *InfoTable;
  EFI_STATUS  Status;

  val_print(AVS_PRINT_DEBUG, "\n Allocating memory for ", 0);
  val_print(AVS_PRINT_DEBUG, table_name, 0);
  val_print(AVS_PRINT_DEBUG, " info table", 0);

  Status = gBS->AllocatePool(EfiBootServicesData,
                              info_table_size,
                              (VOID **) &InfoTable);

  if (EFI_ERROR(Status))
  {
    val_print(AVS_PRINT_ERR, "\n Allocate memory for ", 0);
    val_print(AVS_PRINT_ERR, table_name, 0);
    val_print(AVS_PRINT_ERR, " info table failed : %x", Status);
    return Status;
  }

  (*create_info_tbl_func)(InfoTable);

  return Status;
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
  val_free_shared_mem();
}

VOID
HelpMsg (
  VOID
  )
{
   Print (L"\nUsage: Sbsa.efi [-v <n>] | [-l <n>] | [-f <filename>] | "
         "[-skip <n>] | [-nist] | [-t <n>] | [-m <n>]\n"
         "Options:\n"
         "-v      Verbosity of the Prints\n"
         "        1 shows all prints, 5 shows Errors\n"
         "-mmio   Pass this flag to enable pal_mmio_read/write prints, use with -v 1\n"
         "        Refer to section 4 of SBSA_ACS_User_Guide\n"
         "        To skip a module, use Model_ID as mentioned in user guide\n"
         "-l      Level of compliance to be tested for\n"
         "        As per SBSA spec, 3 to 7\n"
         "-f      Name of the log file to record the test results in\n"
         "-skip   Test(s) to be skipped\n"
         "        Refer to section 4 of SBSA_ACS_User_Guide\n"
         "        To skip a module, use Model_ID as mentioned in user guide\n"
         "        To skip a particular test within a module, use the exact testcase number\n"
         "-nist   Enable the NIST Statistical test suite\n"
         "-t      If set, will only run the specified test, all others will be skipped.\n"
         "-m      If set, will only run the specified module, all others will be skipped.\n"
         "-p2p    Pass this flag to indicate that PCIe Hierarchy Supports Peer-to-Peer\n"
         "-cache  Pass this flag to indicate that if the test system supports PCIe address translation cache\n"
         "-timeout  Set timeout multiple for wakeup tests\n"
         "        1 - min value  5 - max value\n"
         "-p      Option deprecated. PCIe SBSA 7.1(RCiEP) compliance tests are run from SBSA L6+\n"
  );
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v"    , TypeValue},    // -v    # Verbosity of the Prints. 1 shows all prints, 5 shows Errors
  {L"-l"    , TypeValue},    // -l    # Level of compliance to be tested for.
  {L"-f"    , TypeValue},    // -f    # Name of the log file to record the test results in.
  {L"-skip" , TypeValue},    // -skip # test(s) to skip execution
  {L"-help" , TypeFlag},     // -help # help : info about commands
  {L"-h"    , TypeFlag},     // -h    # help : info about commands
  {L"-nist" , TypeFlag},     // -nist # Binary Flag to enable the execution of NIST STS
  {L"-mmio" , TypeValue},    // -mmio # Enable pal_mmio prints
  {L"-t"    , TypeValue},    // -t    # Test to be run
  {L"-m"    , TypeValue},    // -m    # Module to be run
  {L"-p2p", TypeFlag},       // -p2p  # Peer-to-Peer is supported
  {L"-cache", TypeFlag},     // -cache# PCIe address translation cache is supported
  {L"-timeout" , TypeValue}, // -timeout # Set timeout multiple for wakeup tests
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
        Print(L"Allocate memory for -skip failed \n", 0);
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
  } else {
    g_sbsa_level = StrDecimalToUintn(CmdLineArg);
    if (g_sbsa_level > SBSA_MAX_LEVEL_SUPPORTED) {
      g_sbsa_level = G_SBSA_LEVEL;
    }
    if (g_sbsa_level < SBSA_MIN_LEVEL_SUPPORTED) {
      Print(L"SBSA Level %d is not supported.\n", g_sbsa_level);
      HelpMsg();
      return SHELL_INVALID_PARAMETER;
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
    g_sbsa_log_file_handle = NULL;
  } else {
    Status = ShellOpenFileByName(CmdLineArg, &g_sbsa_log_file_handle,
             EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ | EFI_FILE_MODE_CREATE, 0x0);
    if(EFI_ERROR(Status)) {
         Print(L"Failed to open log file %s\n", CmdLineArg);
     g_sbsa_log_file_handle = NULL;
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

  if (g_sbsa_level == 7)
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
              Print(L"Allocate memory for -t failed \n", 0);
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
              Print(L"Allocate memory for -m failed \n", 0);
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
  g_sbsa_tests_total = 0;
  g_sbsa_tests_pass  = 0;
  g_sbsa_tests_fail  = 0;

  val_print(AVS_PRINT_TEST, "\n\n SBSA Architecture Compliance Suite \n", 0);
  val_print(AVS_PRINT_TEST, "    Version %d.", SBSA_ACS_MAJOR_VER);
  val_print(AVS_PRINT_TEST, "%d.", SBSA_ACS_MINOR_VER);
  val_print(AVS_PRINT_TEST, "%d  \n", SBSA_ACS_SUBMINOR_VER);

  val_print(AVS_PRINT_TEST, "\n Starting tests for level %2d", g_sbsa_level);
  val_print(AVS_PRINT_TEST, " (Print level is %2d)\n\n", g_print_level);

  val_print(AVS_PRINT_TEST, " Creating Platform Information Tables \n", 0);

  Status = createPeInfoTable();
  if (Status)
    return Status;

  Status = createGicInfoTable();
  if (Status)
    return Status;

  createTimerInfoTable();
  createWatchdogInfoTable();

  Status = createCacheInfoTable();
  if (Status)
    Print(L" Failed to created Cache info table \n");

  Status = createMpamInfoTable();
  if (Status)
    Print(L" Failed to created Mpam info table \n");

  Status = createHmatInfoTable();
  if (Status)
    Print(L" Failed to created HMAT info table \n");

  Status = createSratInfoTable();
  if (Status)
    Print(L" Failed to created SRAT info table \n");

  Status = createInfoTable(val_ras2_create_info_table, RAS2_FEAT_INFO_TBL_SZ, "RAS2");
  if (Status)
    Print(L" Failed to created RAS2 feature info table \n");

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
  Status = val_pe_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Memory tests                 ***/
  Status |= val_memory_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting GIC tests                    ***/
  Status |= val_gic_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting SMMU tests                   ***/
  if (g_sbsa_level > 3)
    Status |= val_smmu_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Watchdog tests               ***/
  if (g_sbsa_level > 5)
    Status |= val_wd_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting PCIe tests                   ***/
  Status |= val_pcie_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Exerciser tests              ***/
  Status |= val_exerciser_execute_tests(g_sbsa_level);

  /***         Starting MPAM tests                   ***/
  if (g_sbsa_level > 6)
    Status |= val_mpam_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting PMU tests                    ***/
  if (g_sbsa_level > 6)
    Status |= val_pmu_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting RAS tests                    ***/
  if (g_sbsa_level > 6)
    Status |= val_ras_execute_tests(g_sbsa_level, val_pe_get_num());

#ifdef ENABLE_NIST
  /***         Starting NIST tests                   ***/
  if (g_execute_nist == TRUE) {
    Status |= val_nist_execute_tests(g_sbsa_level, val_pe_get_num());
  }
#endif

print_test_status:
  val_print(AVS_PRINT_TEST, "\n     ------------------------------------------------------- \n", 0);
  val_print(AVS_PRINT_TEST, "     Total Tests run  = %4d;", g_sbsa_tests_total);
  val_print(AVS_PRINT_TEST, "  Tests Passed  = %4d", g_sbsa_tests_pass);
  val_print(AVS_PRINT_TEST, "  Tests Failed = %4d\n", g_sbsa_tests_fail);
  val_print(AVS_PRINT_TEST, "     --------------------------------------------------------- \n", 0);

  freeSbsaAvsMem();

  val_print(AVS_PRINT_TEST, "\n      **  For complete SBSA test coverage, it is ", 0);
  val_print(AVS_PRINT_TEST, "\n            necessary to also run the BSA test    ** \n\n", 0);
  val_print(AVS_PRINT_TEST, "\n      *** SBSA tests complete. Reset the system. *** \n\n", 0);

  if(g_sbsa_log_file_handle) {
    ShellCloseFile(&g_sbsa_log_file_handle);
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
