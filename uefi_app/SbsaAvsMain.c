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


UINT32  g_sbsa_level;
UINT32  g_print_level;
UINT32  g_execute_secure;
UINT32  g_skip_test_num[3] = {10000, 10000, 10000};
UINT32  g_sbsa_tests_total;
UINT32  g_sbsa_tests_pass;
UINT32  g_sbsa_tests_fail;
UINT64  g_stack_pointer;
UINT64  g_exception_ret_addr;
UINT64  g_ret_addr;
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
  val_free_shared_mem();
}

VOID
HelpMsg (
  VOID
  )
{
  Print (L"\nUsage: Sbsa.efi [-v <n>] | [-l <n>] | [-f <filename>] | [-s] | [-skip <n>]\n"
         "Options:\n"
         "-v      Verbosity of the Prints\n"
         "        1 shows all prints, 5 shows Errors\n"
         "-l      Level of compliance to be tested for\n"
         "        As per SBSA spec, 0 to 3\n"
         "-f      Name of the log file to record the test results in\n"
         "-s      Enable the execution of secure tests\n"
         "-skip   Test(s) to be skipped\n"
         "        Refer to section 4 of SBSA_ACS_User_Guide\n"
         "        To skip a module, use Model_ID as mentioned in user guide\n"
         "        To skip a particular test within a module, use the exact testcase number\n"
  );
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v"    , TypeValue},    // -v    # Verbosity of the Prints. 1 shows all prints, 5 shows Errors
  {L"-l"    , TypeValue},    // -l    # Level of compliance to be tested for.
  {L"-f"    , TypeValue},    // -f    # Name of the log file to record the test results in.
  {L"-s"    , TypeFlag},     // -s    # Binary Flag to enable the execution of secure tests.
  {L"-skip" , TypeValue},    // -skip # test(s) to skip execution
  {L"-help" , TypeFlag},     // -help # help : info about commands
  {L"-h"    , TypeFlag},     // -h    # help : info about commands
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
ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{

  LIST_ENTRY         *ParamPackage;
  CONST CHAR16       *CmdLineArg;
  CHAR16             *ProbParam;
  UINT32             Status;
  UINT32             i,j=0;
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

  for (i=1 ; i<Argc ; i++) {
    if(StrCmp(Argv[i], L"-skip") == 0){
      CmdLineArg = Argv[i+1];
      i = 0;
      break;
    }
  }

  if (i == 0){
    for (i=0 ; i < StrLen(CmdLineArg) ; i++){
      g_skip_test_num[0] = StrDecimalToUintn((CONST CHAR16 *)(CmdLineArg+0));
      if(*(CmdLineArg+i) == L','){
        g_skip_test_num[++j] = StrDecimalToUintn((CONST CHAR16 *)(CmdLineArg+i+1));
      }
    }
  }

  if (ShellCommandLineGetFlag (ParamPackage, L"-s")) {
    g_execute_secure = TRUE;
  } else {
    g_execute_secure = FALSE;
  }

    // Options with Values
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-l");
  if (CmdLineArg == NULL) {
    g_sbsa_level = G_SBSA_LEVEL;
  } else {
    g_sbsa_level = StrDecimalToUintn(CmdLineArg);
    if (g_sbsa_level > 3) {
      g_sbsa_level = G_SBSA_LEVEL;
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


    // Options with Values
  if ((ShellCommandLineGetFlag (ParamPackage, L"-help")) || (ShellCommandLineGetFlag (ParamPackage, L"-h"))){
     HelpMsg();
     return 0;
  }

  //
  // Initialize global counters
  //
  g_sbsa_tests_total = 0;
  g_sbsa_tests_pass  = 0;
  g_sbsa_tests_fail  = 0;

  Print(L"\n\n SBSA Architecture Compliance Suite \n");
  Print(L"    Version %d.%d  \n", SBSA_ACS_MAJOR_VER, SBSA_ACS_MINOR_VER);


  Print(L"\n Starting tests for level %2d (Print level is %2d)\n\n", g_sbsa_level, g_print_level);


  Print(L" Creating Platform Information Tables \n");
  Status = createPeInfoTable();
  if (Status)
    return Status;

  Status = createGicInfoTable();
  if (Status)
    return Status;

  createTimerInfoTable();
  createWatchdogInfoTable();
  createPcieVirtInfoTable();
  createPeripheralInfoTable();

  val_allocate_shared_mem();

  // Initialise exception vector, so any unexpected exception gets handled by default SBSA exception handler
  branch_label = &&print_test_status;
  val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
  val_pe_initialize_default_exception_handler(val_pe_default_esr);
  FlushImage();

  if (g_execute_secure == TRUE) {
    Print(L"\n      ***  Starting Secure FW tests ***  \n");
    val_secure_execute_tests(g_sbsa_level, val_pe_get_num());
    Print(L"\n      ***  Secure FW tests completed ***  \n");
  }

  Print(L"\n      ***  Starting PE tests ***  \n");
  Status = val_pe_execute_tests(g_sbsa_level, val_pe_get_num());

  Print(L"\n      ***  Starting GIC tests ***  \n");
  Status |= val_gic_execute_tests(g_sbsa_level, val_pe_get_num());

  Print(L"\n      *** Starting Timer tests ***  \n");
  Status |= val_timer_execute_tests(g_sbsa_level, val_pe_get_num());

  Print(L"\n      *** Starting Watchdog tests ***  \n");
  Status |= val_wd_execute_tests(g_sbsa_level, val_pe_get_num());

  Print(L"\n      *** Starting PCIe tests ***  \n");
  Status |= val_pcie_execute_tests(g_sbsa_level, val_pe_get_num());

  Print(L"\n      *** Starting Power and Wakeup semantic tests ***  \n");
  Status |= val_wakeup_execute_tests(g_sbsa_level, val_pe_get_num());

  Print(L"\n      *** Starting Peripheral tests ***  \n");
  Status |= val_peripheral_execute_tests(g_sbsa_level, val_pe_get_num());

  Print(L"\n      *** Starting IO Virtualization tests ***  \n");
  Status |= val_smmu_execute_tests(g_sbsa_level, val_pe_get_num());

print_test_status:
  val_print(AVS_PRINT_TEST, "\n     ------------------------------------------------------- \n", 0);
  val_print(AVS_PRINT_TEST, "     Total Tests run  = %4d;", g_sbsa_tests_total);
  val_print(AVS_PRINT_TEST, "  Tests Passed  = %4d", g_sbsa_tests_pass);
  val_print(AVS_PRINT_TEST, "  Tests Failed = %4d\n", g_sbsa_tests_fail);
  val_print(AVS_PRINT_TEST, "     --------------------------------------------------------- \n", 0);

  freeSbsaAvsMem();

  if(g_sbsa_log_file_handle) {
    ShellCloseFile(&g_sbsa_log_file_handle);
  }

  Print(L"\n      *** SBSA tests complete. Reset the system. *** \n\n");

  val_pe_context_restore(AA64WriteSp(g_stack_pointer));

  return(0);
}
