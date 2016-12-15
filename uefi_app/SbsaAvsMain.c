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

#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellCEntryLib.h>
#include  <Library/ShellLib.h>
#include  <Library/UefiBootServicesTableLib.h>

#include "val/include/val_interface.h"

#include "SbsaAvs.h"


UINT32  g_sbsa_level;
UINT32  g_print_level;
UINT32  g_execute_secure;
UINT32  g_skip_test_num;
UINT32  g_sbsa_tests_total;
UINT32  g_sbsa_tests_pass;
UINT32  g_sbsa_tests_fail;

EFI_STATUS
createPeInfoTable (
)
{

  EFI_STATUS Status;

  UINT64   *PeInfoTable;

/* allowing room for growth, at present each entry is 16 bytes, so we can support upto 511 PEs with 8192 bytes*/
  Status = gBS->AllocatePool ( EfiBootServicesData,
                               8192,
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
                               1024,
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
                              1024, 
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
                              512, 
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
                              64,
                              (VOID **) &PcieInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_pcie_create_info_table(PcieInfoTable);

  Status = gBS->AllocatePool (EfiBootServicesData,
                              128,
                              (VOID **) &IoVirtInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_smmu_create_info_table(IoVirtInfoTable);
  
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
                              1024,
                              (VOID **) &PeripheralInfoTable);

  if (EFI_ERROR(Status))
  {
    Print(L"Allocate Pool failed %x \n", Status);
    return Status;
  }
  val_peripheral_create_info_table(PeripheralInfoTable);

  Status = gBS->AllocatePool (EfiBootServicesData,
                              4096,
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
  val_smmu_free_info_table();
  val_peripheral_free_info_table();
  val_free_shared_mem();
}



STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v", TypeValue},    // -v # Verbosity of the Prints. 0 shows all prints, 5 shows Errors
  {L"-l", TypeValue},    // -l # Level of compliance to be tested for.
  {L"-s", TypeFlag},     // -l # Binary Flag to enable the execution of secure tests.
  {L"-skip", TypeValue}, //test number to skip execution
  {NULL,     TypeMax}
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
  UINT32             Status;


  //
  // Process Command Line arguments
  //
  Status = ShellInitialize();
  Status = ShellCommandLineParse (ParamList, &ParamPackage, NULL, TRUE);
  if (EFI_ERROR(Status)) {
    Print(L"Shell command line parse error %x \n", Status);
    return SHELL_INVALID_PARAMETER;
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
  CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-skip");
  if (CmdLineArg == NULL) {
    g_skip_test_num = 10000;
  } else {
    g_skip_test_num = StrDecimalToUintn(CmdLineArg);
  }

  //
  // Initialize global counters
  //
  g_sbsa_tests_total = 0;
  g_sbsa_tests_pass  = 0;
  g_sbsa_tests_fail  = 0;

  Print(L"\n\n SBSA Compliance Suite \n");
  Print(L"    Version %d.%d  \n", SBSA_ACS_MAJOR_VER, SBSA_ACS_MINOR_VER);


  Print(L"\n Starting Compliance verification for Level %2d (Print level is %2d)\n\n", g_sbsa_level, g_print_level);

    
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

  if (g_execute_secure == TRUE) {
    Print(L"\n      ***  Starting Secure FW tests ***  \n");
    val_secure_execute_tests(g_sbsa_level, val_pe_get_num());
    Print(L"\n      ***  Secure FW tests Completed ***  \n");
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

  Print(L"\n      *** Starting IO Virtualization tests ***  \n");
  Status |= val_smmu_execute_tests(g_sbsa_level, val_pe_get_num());

  Print(L"\n      *** Starting Power and Wakeup semantic tests ***  \n");
  Status |= val_wakeup_execute_tests(g_sbsa_level, val_pe_get_num());

  Print(L"\n      *** Starting Peripheral tests ***  \n");
  Status |= val_peripheral_execute_tests(g_sbsa_level, val_pe_get_num());

  Print(L"\n     ------------------------------------------------------- \n");
  Print(L"     Total Tests run  = %4d;  Tests Passed  = %4d  Tests Failed = %4d \n",
             g_sbsa_tests_total, g_sbsa_tests_pass, g_sbsa_tests_fail);
  Print(L"     --------------------------------------------------------- \n");
  freeSbsaAvsMem();

  Print(L"\n      *** SBSA Compliance Test Complete. Reset the System. *** \n\n");
  return(0);
}
