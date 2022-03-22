/** @file
 * Copyright (c) 2022, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/val_interface.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_val.h"

#include "val/include/sbsa_avs_memory.h"
#include "platform/pal_baremetal/FVP/include/platform_override_fvp.h"
#include "SbsaAvs.h"

uint32_t  g_sbsa_level;
uint32_t  g_enable_pcie_tests;
uint32_t  g_print_level;
uint32_t  g_execute_nist;
uint32_t  g_print_mmio;
uint32_t  g_curr_module;
uint32_t  g_enable_module;
uint32_t  g_sbsa_tests_total;
uint32_t  g_sbsa_tests_pass;
uint32_t  g_sbsa_tests_fail;
uint64_t  g_stack_pointer;
uint64_t  g_exception_ret_addr;
uint64_t  g_ret_addr;
uint32_t  g_skip_test_num[MAX_TEST_SKIP_NUM] = { 10000, 10000, 10000, 10000, 10000,
                                               10000, 10000, 10000, 10000 };

uint32_t
createPeInfoTable(
)
{

  uint32_t Status;
  uint64_t *PeInfoTable;

  PeInfoTable = val_memory_alloc(sizeof(PE_INFO_TABLE) +
                                (PLATFORM_OVERRIDE_PE_CNT * sizeof(PE_INFO_ENTRY)));

  Status = val_pe_create_info_table(PeInfoTable);

  return Status;

}

uint32_t
createGicInfoTable(
)
{
  uint32_t Status;
  uint64_t *GicInfoTable;
  uint32_t gic_info_end_index = 1; //Additional index for mem alloc to store the end value(0xff)

  GicInfoTable = val_memory_alloc(sizeof(GIC_INFO_TABLE) + ((PLATFORM_OVERRIDE_GICITS_COUNT
                  + PLATFORM_OVERRIDE_GICRD_COUNT + PLATFORM_OVERRIDE_GICC_COUNT
                  + PLATFORM_OVERRIDE_GICD_COUNT + gic_info_end_index) * sizeof(GIC_INFO_ENTRY)));

  Status = val_gic_create_info_table(GicInfoTable);

  return Status;

}

uint32_t
configureGicIts(
)
{
  uint32_t Status;

  Status = val_gic_its_configure();

  return Status;
}

void
createTimerInfoTable(
)
{
  uint64_t   *TimerInfoTable;

  TimerInfoTable = val_memory_alloc(sizeof(TIMER_INFO_TABLE) + (2 * sizeof(TIMER_INFO_GTBLOCK)));

  val_timer_create_info_table(TimerInfoTable);
}

void
createWatchdogInfoTable(
)
{
  uint64_t *WdInfoTable;

  WdInfoTable = val_memory_alloc(sizeof(WD_INFO_TABLE) + (2 * sizeof(WD_INFO_BLOCK)));

  val_wd_create_info_table(WdInfoTable);
}


void
createPcieVirtInfoTable(
)
{
  uint64_t   *PcieInfoTable;
  uint64_t   *IoVirtInfoTable;

  PcieInfoTable = val_memory_alloc(sizeof(PCIE_INFO_TABLE) + (1 * sizeof(PCIE_INFO_BLOCK)));
  val_pcie_create_info_table(PcieInfoTable);

  IoVirtInfoTable = val_memory_alloc(sizeof(IOVIRT_INFO_TABLE) + (4 * sizeof(IOVIRT_BLOCK))
                                                               + (16 * sizeof(ID_MAP)));
  val_iovirt_create_info_table(IoVirtInfoTable);
}

void
createPeripheralInfoTable(
)
{
  uint64_t   *PeripheralInfoTable;
  uint64_t   *MemoryInfoTable;

  PeripheralInfoTable = val_memory_alloc(sizeof(PERIPHERAL_INFO_TABLE) +
                                        (1 * sizeof(PERIPHERAL_INFO_BLOCK)));
  val_peripheral_create_info_table(PeripheralInfoTable);

  MemoryInfoTable = val_memory_alloc(sizeof(MEMORY_INFO_TABLE) + (4 * sizeof(MEM_INFO_BLOCK)));
  val_memory_create_info_table(MemoryInfoTable);
}

void
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

/***
  SBSA Compliance Suite Entry Point.

  Call the Entry points of individual modules.

  @retval  0         The application exited normally.
  @retval  Other     An error occurred.
***/
int32_t
ShellAppMainsbsa(
  )
{

  uint32_t             Status;
  void                 *branch_label;

  g_print_level = PLATFORM_OVERRIDE_PRINT_LEVEL;
  if (g_print_level < AVS_PRINT_INFO)
  {
      val_print(AVS_PRINT_ERR, "Print Level %d is not supported.\n", g_print_level);
      val_print(AVS_PRINT_ERR, "Setting Print level to %d\n", AVS_PRINT_INFO);
      g_print_level = AVS_PRINT_INFO;
  } else if (g_print_level > AVS_PRINT_ERR) {
      val_print(AVS_PRINT_ERR, "Print Level %d is not supported.\n", g_print_level);
      val_print(AVS_PRINT_ERR, "Setting Print level to %d\n", AVS_PRINT_ERR);
      g_print_level = AVS_PRINT_ERR;
  }

  g_sbsa_level = PLATFORM_OVERRIDE_SBSA_LEVEL;
  if (g_sbsa_level < SBSA_MIN_LEVEL_SUPPORTED)
  {
      val_print(g_print_level, "SBSA Level %d is not supported.\n", g_sbsa_level);
      val_print(g_print_level, "Setting SBSA level to %d\n", SBSA_MIN_LEVEL_SUPPORTED);
      g_sbsa_level = SBSA_MIN_LEVEL_SUPPORTED;
  } else if (g_sbsa_level > SBSA_MAX_LEVEL_SUPPORTED) {
      val_print(g_print_level, "SBSA Level %d is not supported.\n", g_sbsa_level);
      val_print(g_print_level, "Setting SBSA level to %d\n", SBSA_MAX_LEVEL_SUPPORTED);
      g_sbsa_level = SBSA_MAX_LEVEL_SUPPORTED;
  }

  g_execute_nist = FALSE;
  g_print_mmio = FALSE;
  g_enable_pcie_tests = 1;

  //
  // Initialize global counters
  //
  g_sbsa_tests_total = 0;
  g_sbsa_tests_pass  = 0;
  g_sbsa_tests_fail  = 0;

  val_print(g_print_level, "\n\n SBSA Architecture Compliance Suite \n", 0);
  val_print(g_print_level, "    Version %d.", SBSA_ACS_MAJOR_VER);
  val_print(g_print_level, "%d  \n", SBSA_ACS_MINOR_VER);

  val_print(g_print_level, "\n Starting tests for level %2d", g_sbsa_level);
  val_print(g_print_level, " (Print level is %2d)\n\n", g_print_level);


  val_print(g_print_level, " Creating Platform Information Tables \n", 0);
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

  /* Initialise exception vector, so any unexpected exception gets handled
   *  by default SBSA exception handler.
   */
  branch_label = &&print_test_status;
  val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
  val_pe_initialize_default_exception_handler(val_pe_default_esr);

  val_print(g_print_level, "\n      ***  Starting PE tests ***  \n", 0);
  Status = val_pe_execute_tests(g_sbsa_level, val_pe_get_num());

  val_print(g_print_level, "\n      ***  Starting GIC tests ***  \n", 0);
  Status |= val_gic_execute_tests(g_sbsa_level, val_pe_get_num());

  val_print(g_print_level, "\n      *** Starting Timer tests ***  \n", 0);
  Status |= val_timer_execute_tests(g_sbsa_level, val_pe_get_num());

  val_print(g_print_level, "\n      *** Starting Watchdog tests ***  \n", 0);
  Status |= val_wd_execute_tests(g_sbsa_level, val_pe_get_num());

  val_print(g_print_level, "\n      *** Starting PCIe tests ***  \n", 0);
  Status |= val_pcie_execute_tests(g_enable_pcie_tests, g_sbsa_level, val_pe_get_num());

  val_print(g_print_level, "\n      *** Starting Power and Wakeup semantic tests ***  \n", 0);
  Status |= val_wakeup_execute_tests(g_sbsa_level, val_pe_get_num());

  val_print(g_print_level, "\n      *** Starting Peripheral tests ***  \n", 0);
  Status |= val_peripheral_execute_tests(g_sbsa_level, val_pe_get_num());

  val_print(g_print_level, "\n      *** Starting IO Virtualization tests ***  \n", 0);
  Status |= val_smmu_execute_tests(g_sbsa_level, val_pe_get_num());

  /*
   * Configure Gic Redistributor and ITS to support
   * Generation of LPIs.
  */
  configureGicIts();

  val_print(g_print_level, "\n      *** Starting PCIe Exerciser tests ***  \n", 0);
  Status |= val_exerciser_execute_tests(g_sbsa_level);

print_test_status:
  val_print(AVS_PRINT_TEST, "\n     ------------------------------------------------------- \n", 0);
  val_print(AVS_PRINT_TEST, "     Total Tests run  = %4d;", g_sbsa_tests_total);
  val_print(AVS_PRINT_TEST, "  Tests Passed  = %4d", g_sbsa_tests_pass);
  val_print(AVS_PRINT_TEST, "  Tests Failed = %4d\n", g_sbsa_tests_fail);
  val_print(AVS_PRINT_TEST, "     --------------------------------------------------------- \n", 0);

  freeSbsaAvsMem();

  val_print(g_print_level, "\n      *** SBSA tests complete. Reset the system. *** \n\n", 0);

  val_pe_context_restore(AA64WriteSp(g_stack_pointer));

  return 0;
}

