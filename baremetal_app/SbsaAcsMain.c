/** @file
 * Copyright (c) 2022-2024, Arm Limited or its affiliates. All rights reserved.
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

#include "val/common/include/val_interface.h"
#include "val/common/include/pal_interface.h"
#include "val/common/include/acs_val.h"
#include "val/common/include/acs_memory.h"
#include "val/common/include/acs_pe.h"
#include "val/sbsa/include/sbsa_val_interface.h"
#include "val/sbsa/include/sbsa_acs_pe.h"

#include "SbsaAcs.h"

uint32_t  g_sbsa_level;
uint32_t  g_print_level;
uint32_t  g_execute_nist;
uint32_t  g_print_mmio;
uint32_t  g_curr_module;
uint32_t  g_enable_module;
uint32_t  g_acs_tests_total;
uint32_t  g_acs_tests_pass;
uint32_t  g_acs_tests_fail;
uint64_t  g_stack_pointer;
uint64_t  g_exception_ret_addr;
uint64_t  g_ret_addr;
uint32_t  g_wakeup_timeout;
uint32_t  *g_skip_test_num;
uint32_t  *g_execute_tests;
uint32_t  *g_execute_modules;
uint32_t  g_sys_last_lvl_cache;

extern uint32_t g_skip_array[];
extern uint32_t g_num_skip;
extern uint32_t g_test_array[];
extern uint32_t g_num_tests;
extern uint32_t g_module_array[];
extern uint32_t g_num_modules;
extern uint32_t g_sbsa_run_fr;

uint32_t
createPeInfoTable(
)
{

  uint32_t Status;
  uint64_t *PeInfoTable;

  PeInfoTable = val_aligned_alloc(SIZE_4K, sizeof(PE_INFO_TABLE) +
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

  GicInfoTable = val_aligned_alloc(SIZE_4K, sizeof(GIC_INFO_TABLE)
                  + ((PLATFORM_OVERRIDE_GICITS_COUNT
                  + PLATFORM_OVERRIDE_GICRD_COUNT + PLATFORM_OVERRIDE_GICC_COUNT
                  + PLATFORM_OVERRIDE_GICD_COUNT + gic_info_end_index) * sizeof(GIC_INFO_ENTRY)));

  Status = val_gic_create_info_table(GicInfoTable);

  return Status;

}

void
createTimerInfoTable(
)
{
  uint64_t   *TimerInfoTable;

  TimerInfoTable = val_aligned_alloc(SIZE_4K, sizeof(TIMER_INFO_TABLE)
                   + (PLATFORM_OVERRIDE_TIMER_COUNT * sizeof(TIMER_INFO_GTBLOCK)));

  val_timer_create_info_table(TimerInfoTable);
}

void
createWatchdogInfoTable(
)
{
  uint64_t *WdInfoTable;

  WdInfoTable = val_aligned_alloc(SIZE_4K, sizeof(WD_INFO_TABLE)
                + (PLATFORM_OVERRIDE_WD_TIMER_COUNT * sizeof(WD_INFO_BLOCK)));

  val_wd_create_info_table(WdInfoTable);
}


void
createPcieVirtInfoTable(
)
{
  uint64_t   *PcieInfoTable;
  uint64_t   *IoVirtInfoTable;

  PcieInfoTable = val_aligned_alloc(SIZE_4K, sizeof(PCIE_INFO_TABLE)
                  + (PLATFORM_OVERRIDE_NUM_ECAM * sizeof(PCIE_INFO_BLOCK)));
  val_pcie_create_info_table(PcieInfoTable);

  IoVirtInfoTable = val_aligned_alloc(SIZE_4K, sizeof(IOVIRT_INFO_TABLE)
                    + ((IOVIRT_ITS_COUNT + IOVIRT_SMMUV3_COUNT + IOVIRT_RC_COUNT
                    + IOVIRT_SMMUV2_COUNT + IOVIRT_NAMED_COMPONENT_COUNT + IOVIRT_PMCG_COUNT)
                    * sizeof(IOVIRT_BLOCK)) + (IOVIRT_MAX_NUM_MAP * sizeof(ID_MAP)));
  val_iovirt_create_info_table(IoVirtInfoTable);
}

void
createPeripheralInfoTable(
)
{
  uint64_t   *PeripheralInfoTable;
  uint64_t   *MemoryInfoTable;

  PeripheralInfoTable = val_aligned_alloc(SIZE_4K, sizeof(PERIPHERAL_INFO_TABLE)
                        + (PLATFORM_OVERRIDE_PERIPHERAL_COUNT * sizeof(PERIPHERAL_INFO_BLOCK)));
  val_peripheral_create_info_table(PeripheralInfoTable);

  MemoryInfoTable = val_aligned_alloc(SIZE_4K, sizeof(MEMORY_INFO_TABLE)
                    + (PLATFORM_OVERRIDE_MEMORY_ENTRY_COUNT * sizeof(MEM_INFO_BLOCK)));
  val_memory_create_info_table(MemoryInfoTable);
}

void
createPmuInfoTable(
)
{
  uint64_t   *PmuInfoTable;

  PmuInfoTable = val_aligned_alloc(SIZE_4K, sizeof(PMU_INFO_TABLE)
                  + PLATFORM_OVERRIDE_PMU_NODE_CNT * sizeof(PMU_INFO_BLOCK));
  val_pmu_create_info_table(PmuInfoTable);

}

void
createRasInfoTable(
)
{
  uint64_t   *RasInfoTable;

  RasInfoTable = val_aligned_alloc(SIZE_4K, sizeof(RAS_INFO_TABLE)
                  + (PLATFORM_OVERRIDE_NUM_PE_RAS_NODES + PLATFORM_OVERRIDE_NUM_MC_RAS_NODES)
                  * sizeof(RAS_NODE_INFO) + PLATFORM_OVERRIDE_NUM_RAS_NODES
                  * sizeof(RAS_INTERFACE_INFO)
                  + PLATFORM_OVERRIDE_NUM_RAS_NODES * sizeof(RAS_INTERRUPT_INFO));
  val_ras_create_info_table(RasInfoTable);

}

void
createCacheInfoTable(
)
{
  uint64_t   *CacheInfoTable;

  CacheInfoTable = val_aligned_alloc(SIZE_4K, sizeof(CACHE_INFO_TABLE)
                   + PLATFORM_OVERRIDE_CACHE_CNT * sizeof(CACHE_INFO_ENTRY));
  val_cache_create_info_table(CacheInfoTable);

}

void
createMpamInfoTable(
)
{
  uint64_t *MpamInfoTable;

  MpamInfoTable = val_aligned_alloc(SIZE_4K, sizeof(MPAM_INFO_TABLE)
                                    + PLATFORM_MPAM_MSC_COUNT * sizeof(MPAM_MSC_NODE)
                                    + PLATFORM_MPAM_MSC_COUNT * sizeof(MPAM_RESOURCE_NODE));
  val_mpam_create_info_table(MpamInfoTable);

}

void
createHmatInfoTable(
)
{
  uint64_t      *HmatInfoTable;

  HmatInfoTable = val_aligned_alloc(SIZE_4K, sizeof(HMAT_INFO_TABLE)
                                    + PLATFORM_OVERRIDE_HMAT_MEM_ENTRIES * sizeof(HMAT_BW_ENTRY));
  val_hmat_create_info_table(HmatInfoTable);

}

void
createSratInfoTable(
)
{
  uint64_t      *SratInfoTable;

  SratInfoTable = val_aligned_alloc(SIZE_4K,
                                    PLATFORM_OVERRIDE_NUM_SRAT_ENTRIES * sizeof(SRAT_INFO_ENTRY)
                                    + PLATFORM_OVERRIDE_MEM_AFF_CNT * sizeof(SRAT_MEM_AFF_ENTRY)
                                    + PLATFORM_OVERRIDE_GICC_AFF_CNT * sizeof(SRAT_GICC_AFF_ENTRY));
  val_srat_create_info_table(SratInfoTable);

}

/**
  @brief  This API allocates memory for info table and
          calls create info table function passed as parameter.

  @param  create_info_tbl_func  - function pointer to val_*_create_info_table
  @param  info_table_size       - memory size to be allocated.

  @return  None
**/

void
createInfoTable(
  void(*create_info_tbl_func)(uint64_t *),
  uint64_t info_table_size,
  char8_t *table_name
  )
{
  uint64_t      *InfoTable;

  val_print(ACS_PRINT_DEBUG, "\n Allocating memory for ", 0);
  val_print(ACS_PRINT_DEBUG, table_name, 0);
  val_print(ACS_PRINT_DEBUG, " info table", 0);

  InfoTable = val_aligned_alloc(SIZE_4K, info_table_size);


  (*create_info_tbl_func)(InfoTable);

}

void
createRas2InfoTable(
)
{
  uint64_t ras2_size = sizeof(RAS2_INFO_TABLE)
                       + PLATFORM_OVERRIDE_NUM_RAS2_BLOCK * sizeof(RAS2_BLOCK)
                       + PLATFORM_OVERRIDE_NUM_RAS2_MEM_BLOCK * sizeof(RAS2_MEM_INFO);
  createInfoTable(val_ras2_create_info_table, ras2_size, "RAS2");

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
  val_pmu_free_info_table();
  val_cache_free_info_table();
  val_mpam_free_info_table();
  val_hmat_free_info_table();
  val_srat_free_info_table();
  val_ras2_free_info_table();
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

  g_print_level = PLATFORM_OVERRIDE_SBSA_PRINT_LEVEL;
  if (g_print_level < ACS_PRINT_INFO)
  {
      val_print(ACS_PRINT_ERR, "Print Level %d is not supported.\n", g_print_level);
      val_print(ACS_PRINT_ERR, "Setting Print level to %d\n", ACS_PRINT_INFO);
      g_print_level = ACS_PRINT_INFO;
  } else if (g_print_level > ACS_PRINT_ERR) {
      val_print(ACS_PRINT_ERR, "Print Level %d is not supported.\n", g_print_level);
      val_print(ACS_PRINT_ERR, "Setting Print level to %d\n", ACS_PRINT_ERR);
      g_print_level = ACS_PRINT_ERR;
  }

#ifdef TARGET_BM_BOOT
  /* Write page tables */
  if (val_setup_mmu())
      return ACS_STATUS_FAIL;

  /* Enable Stage-1 MMU */
  if (val_enable_mmu())
      return ACS_STATUS_FAIL;
#endif

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

  if (g_sbsa_run_fr)
    g_sbsa_level = SBSA_FR_LEVEL;

  val_print(ACS_PRINT_TEST, "\n\n SBSA Architecture Compliance Suite\n", 0);
  val_print(ACS_PRINT_TEST, "    Version %d.", SBSA_ACS_MAJOR_VER);
  val_print(ACS_PRINT_TEST, "%d.", SBSA_ACS_MINOR_VER);
  val_print(ACS_PRINT_TEST, "%d\n", SBSA_ACS_SUBMINOR_VER);

  val_print(ACS_PRINT_TEST, "\n Starting tests for level %2d", g_sbsa_level);
  val_print(ACS_PRINT_TEST, " (Print level is %2d)\n\n", g_print_level);

  val_print(ACS_PRINT_TEST, " Creating Platform Information Tables\n", 0);

  g_skip_test_num   = &g_skip_array[0];
  if (g_num_tests) {
      g_execute_tests   = &g_test_array[0];
  }

  if (g_num_modules) {
      g_execute_modules = &g_module_array[0];
  }

  g_execute_nist = FALSE;
  g_print_mmio = FALSE;
  g_wakeup_timeout = PLATFORM_OVERRIDE_TIMEOUT;
  g_sys_last_lvl_cache = PLATFORM_OVERRRIDE_SLC;

  //
  // Initialize global counters
  //
  g_acs_tests_total = 0;
  g_acs_tests_pass  = 0;
  g_acs_tests_fail  = 0;

  Status = createPeInfoTable();
  if (Status)
    return Status;

  Status = createGicInfoTable();
  if (Status)
    return Status;

  createTimerInfoTable();

  createWatchdogInfoTable();

  createCacheInfoTable();

  createMpamInfoTable();

  createHmatInfoTable();

  createSratInfoTable();

  createPcieVirtInfoTable();

  createPeripheralInfoTable();

  createPmuInfoTable();

  createRasInfoTable();

  createRas2InfoTable();

  val_allocate_shared_mem();

  /* Initialise exception vector, so any unexpected exception gets handled
   *  by default SBSA exception handler.
   */
  branch_label = &&print_test_status;
  val_pe_context_save(AA64ReadSp(), (uint64_t)branch_label);
  val_pe_initialize_default_exception_handler(val_pe_default_esr);

  /***         Starting PE tests                     ***/
  Status = val_sbsa_pe_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Memory tests                 ***/
  Status |= val_sbsa_memory_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting GIC tests                    ***/
  Status |= val_sbsa_gic_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting SMMU tests                   ***/
  if (g_sbsa_level > 3)
    Status |= val_sbsa_smmu_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Watchdog tests               ***/
  if (g_sbsa_level > 5)
    Status |= val_sbsa_wd_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting PCIe tests                   ***/
  Status |= val_sbsa_pcie_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting Exerciser tests              ***/
  Status |= val_sbsa_exerciser_execute_tests(g_sbsa_level);

  /***         Starting MPAM tests                   ***/
  if (g_sbsa_level > 6)
    Status |= val_sbsa_mpam_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting PMU tests                    ***/
  if (g_sbsa_level > 6)
    Status |= val_sbsa_pmu_execute_tests(g_sbsa_level, val_pe_get_num());

  /***         Starting RAS tests                    ***/
  if (g_sbsa_level > 6)
    Status |= val_sbsa_ras_execute_tests(g_sbsa_level, val_pe_get_num());

    /***         Starting ETE tests                    ***/
  if (g_sbsa_level > 7)
    Status |= val_sbsa_ete_execute_tests(g_sbsa_level, val_pe_get_num());

print_test_status:
  val_print(ACS_PRINT_TEST, "\n     -------------------------------------------------------\n", 0);
  val_print(ACS_PRINT_TEST, "     Total Tests run  = %4d;", g_acs_tests_total);
  val_print(ACS_PRINT_TEST, "  Tests Passed  = %4d", g_acs_tests_pass);
  val_print(ACS_PRINT_TEST, "  Tests Failed = %4d\n", g_acs_tests_fail);
  val_print(ACS_PRINT_TEST, "     ---------------------------------------------------------\n", 0);

  freeSbsaAvsMem();

  val_print(ACS_PRINT_TEST, "\n      **  For complete SBSA test coverage, it is ", 0);
  val_print(ACS_PRINT_TEST, "\n            necessary to also run the BSA test    **\n\n", 0);
  val_print(ACS_PRINT_TEST, "\n      *** SBSA tests complete. Reset the system. ***\n\n", 0);


  val_pe_context_restore(AA64WriteSp(g_stack_pointer));
  while (1);
  return 0;
}
