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
#include "include/sbsa_avs_common.h"

#include "include/sbsa_avs_pcie.h"

#define WARN_STR_LEN 7
PCIE_INFO_TABLE *g_pcie_info_table;
pcie_device_bdf_table *g_pcie_bdf_table;
uint32_t pcie_bdf_table_list_flag;

uint64_t
pal_get_mcfg_ptr(void);

/**
  @brief   This API reads 32-bit data from PCIe config space pointed by Bus,
           Device, Function and register offset.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_pcie_create_info_table
  @param   bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
  @param   offset - Register offset within a device PCIe config space
  @param   *data  - 32-bit data read from the config space

  @return  success/failure
**/
uint32_t
val_pcie_read_cfg(uint32_t bdf, uint32_t offset, uint32_t *data)
{
  uint32_t bus     = PCIE_EXTRACT_BDF_BUS(bdf);
  uint32_t dev     = PCIE_EXTRACT_BDF_DEV(bdf);
  uint32_t func    = PCIE_EXTRACT_BDF_FUNC(bdf);
  uint32_t segment = PCIE_EXTRACT_BDF_SEG(bdf);
  uint32_t cfg_addr;
  addr_t   ecam_base = 0;
  uint32_t i = 0;


  if ((bus >= PCIE_MAX_BUS) || (dev >= PCIE_MAX_DEV) || (func >= PCIE_MAX_FUNC)) {
     val_print(AVS_PRINT_ERR, "\n       Invalid Bus/Dev/Func  %x", bdf);
     return PCIE_NO_MAPPING;
  }

  if (g_pcie_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "\n       Read_PCIe_CFG: PCIE info table is not created", 0);
      return PCIE_NO_MAPPING;
  }

  while (i < val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0))
  {

      if ((bus >= val_pcie_get_info(PCIE_INFO_START_BUS, i)) &&
           (bus <= val_pcie_get_info(PCIE_INFO_END_BUS, i)) &&
           (segment == val_pcie_get_info(PCIE_INFO_SEGMENT, i))) {
          ecam_base = val_pcie_get_info(PCIE_INFO_ECAM, i);
          break;
      }
      i++;
  }

  if (ecam_base == 0) {
      val_print(AVS_PRINT_ERR, "\n       Read PCIe_CFG: ECAM Base is zero for bdf %x", bdf);
      return PCIE_NO_MAPPING;
  }

  /* There are 8 functions / device, 32 devices / Bus and each has a 4KB config space */
  cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
               (dev * PCIE_MAX_FUNC * 4096) + (func * 4096);

  *data = pal_mmio_read(ecam_base + cfg_addr + offset);
  return 0;

}

/**
  @brief   Read 32bit data  from PCIe config space pointed by Bus,
           Device, Function and offset using UEFI PciIoProtocol interface
           1. Caller       -  Test Suite
           2. Prerequisite -  val_pcie_create_info_table
  @param   bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
  @param   offset - Register offset within a device PCIe config space
  @param   *data - 32-bit data read from the config space

  @return success/failure
**/
uint32_t
val_pcie_io_read_cfg(uint32_t bdf, uint32_t offset, uint32_t *data)
{
    return pal_pcie_io_read_cfg(bdf, offset, data);
}

/**
  @brief   This API writes 32-bit data to PCIe config space pointed by Bus,
           Device, Function and register offset.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_pcie_create_info_table
  @param   bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
  @param   offset - Register offset within a device PCIe config space
  @param   data   - data to be written to the config space

  @return  None
**/
void
val_pcie_write_cfg(uint32_t bdf, uint32_t offset, uint32_t data)
{
  uint32_t bus      = PCIE_EXTRACT_BDF_BUS(bdf);
  uint32_t dev      = PCIE_EXTRACT_BDF_DEV(bdf);
  uint32_t func     = PCIE_EXTRACT_BDF_FUNC(bdf);
  uint32_t segment  = PCIE_EXTRACT_BDF_SEG(bdf);
  uint32_t cfg_addr;
  addr_t   ecam_base = 0;
  uint32_t i = 0;


  if ((bus >= PCIE_MAX_BUS) || (dev >= PCIE_MAX_DEV) || (func >= PCIE_MAX_FUNC)) {
     val_print(AVS_PRINT_ERR, "Invalid Bus/Dev/Func  %x \n", bdf);
     return;
  }

  if (g_pcie_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "\n       Write PCIe_CFG: PCIE info table is not created", 0);
      return;
  }

  while (i < val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0))
  {

      if ((bus >= val_pcie_get_info(PCIE_INFO_START_BUS, i)) &&
           (bus <= val_pcie_get_info(PCIE_INFO_END_BUS, i)) &&
           (segment == val_pcie_get_info(PCIE_INFO_SEGMENT, i))) {
          ecam_base = val_pcie_get_info(PCIE_INFO_ECAM, i);
          break;
      }
      i++;
  }

  if (ecam_base == 0) {
      val_print(AVS_PRINT_ERR, "\n       Read PCIe_CFG: ECAM Base is zero ", 0);
      return;
  }

  /* There are 8 functions / device, 32 devices / Bus and each has a 4KB config space */
  cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
               (dev * PCIE_MAX_FUNC * 4096) + (func * 4096);

  pal_mmio_write(ecam_base + cfg_addr + offset, data);
}

/**
  @brief   Write 32bit data to PCIe config space pointed by Bus,
           Device, Function and offset using UEFI PciIoProtocol interface
           1. Caller       -  Test Suite
           2. Prerequisite -  val_pcie_create_info_table
  @param   bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)
  @param   offset - Register offset within a device PCIe config space
  @param   data - 32-bit data write to the config space

  @return success/failure
**/
void
val_pcie_io_write_cfg(uint32_t bdf, uint32_t offset, uint32_t data)
{
    return pal_pcie_io_write_cfg(bdf, offset, data);
}

/**
    @brief   Write 32-bit data to BAR space pointed by Bus,
             Device, Function and register offset using UEFI PciIoProtocol interface.

    @param   Bdf     - BDF value for the device
    @param   address - BAR memory address
    @param   data    - 32 bit value to writw BAR address
    @return  success/failure
**/
uint32_t
val_pcie_bar_mem_write(uint32_t bdf, uint64_t offset, uint32_t data)
{
    return pal_pcie_bar_mem_write(bdf, offset, data);
}

/**
    @brief   Reads 32-bit data from BAR space pointed by Bus,
             Device, Function and register offset using UEFI PciIoProtocol interface.

    @param   Bdf     - BDF value for the device
    @param   address - BAR memory address
    @param   *data   - 32 bit value at BAR address
    @return  success/failure
**/
uint32_t
val_pcie_bar_mem_read(uint32_t bdf, uint64_t offset, uint32_t *data)
{
    return pal_pcie_bar_mem_read(bdf, offset, data);
}

/**
  @brief   This API  returns function config space addr.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_pcie_create_info_table
  @param   bdf    - concatenated Bus(8-bits), device(8-bits) & function(8-bits)

  @return  function config space address
**/
uint64_t val_pcie_get_bdf_config_addr(uint32_t bdf)
{
  uint32_t bus      = PCIE_EXTRACT_BDF_BUS(bdf);
  uint32_t dev      = PCIE_EXTRACT_BDF_DEV(bdf);
  uint32_t func     = PCIE_EXTRACT_BDF_FUNC(bdf);
  uint32_t segment  = PCIE_EXTRACT_BDF_SEG(bdf);
  uint32_t cfg_addr;
  uint32_t num_ecam;
  addr_t   ecam_base = 0;
  uint32_t i = 0;

  if ((bus >= PCIE_MAX_BUS) || (dev >= PCIE_MAX_DEV) || (func >= PCIE_MAX_FUNC)) {
     val_print(AVS_PRINT_ERR, "Invalid Bus/Dev/Func  %x \n", bdf);
     return 0;
  }

  if (g_pcie_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "\n       PCIe_CFG: PCIE info table is not created", 0);
      return 0;
  }

  num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);
  while (i < num_ecam)
  {

      if ((bus >= val_pcie_get_info(PCIE_INFO_START_BUS, i)) &&
           (bus <= val_pcie_get_info(PCIE_INFO_END_BUS, i)) &&
           (segment == val_pcie_get_info(PCIE_INFO_SEGMENT, i))) {
          ecam_base = val_pcie_get_info(PCIE_INFO_ECAM, i);
          break;
      }
      i++;
  }

  if (ecam_base == 0) {
      val_print(AVS_PRINT_ERR, "\n       Read PCIe_CFG: ECAM Base is zero ", 0);
      return 0;
  }

  /* There are 8 functions / device, 32 devices / Bus and each has a 4KB config space */
  cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
               (dev * PCIE_MAX_FUNC * 4096) + (func * 4096);

 return ecam_base + cfg_addr;

}


/**
  @brief  This API performs the PCI enumeration

**/
void val_pcie_enumerate(void)
{

    pal_pcie_enumerate();

}

/**
  @brief   This API executes all the PCIe tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_pcie_create_info_table()
  @param   level       - level of compliance being tested for.
  @param   num_pe      - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_pcie_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = AVS_STATUS_PASS, i;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == AVS_PCIE_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "\n USER Override - Skipping all PCIe tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(AVS_PCIE_TEST_NUM_BASE);
  if (status) {
      val_print(AVS_PRINT_TEST, "\n USER Override - Skipping all PCIe tests \n", 0);
      return AVS_STATUS_SKIP;
  }

  if (pcie_bdf_table_list_flag == 1) {
    val_print(AVS_PRINT_WARN, "\n     *** Created device list with valid bdf doesn't match \
                    with the platform pcie device hierarchy, Skipping PCIE tests *** \n", 0);
    return AVS_STATUS_SKIP;
  }

  val_print_test_start("PCIe");
  g_curr_module = 1 << PCIE_MODULE;

  #if defined(TARGET_LINUX) || defined(TARGET_EMULATION)
    status |= p009_entry(num_pe);  /* This covers GIC rule */
  #endif

  if (level > 3) {
  /* Only the test p062 will be run at L4+ with the test number (AVS_PER_TEST_NUM_BASE + 1) */
  #ifndef TARGET_LINUX
    status = p062_entry(num_pe);
  #endif
  }

  if (level > 5) {

    status = p001_entry(num_pe);

    if (status == AVS_STATUS_FAIL) {
      val_print(AVS_PRINT_WARN, "\n     *** Skipping remaining PCIE tests *** \n", 0);
      return status;
    }

    #if defined(TARGET_LINUX) || defined(TARGET_EMULATION)
      status |= p005_entry(num_pe);
    #endif

    if (g_pcie_bdf_table->num_entries == 0) {
      val_print(AVS_PRINT_WARN, "\n     *** No Valid Devices Found, \
                Skipping remaining PCIE tests *** \n", 0);
      return AVS_STATUS_SKIP;
    }

    #ifndef TARGET_LINUX
      status |= p003_entry(num_pe);
      status |= p016_entry(num_pe);
      status |= p020_entry(num_pe);
      status |= p021_entry(num_pe);
      status |= p022_entry(num_pe); /* iEP/RP only */
      status |= p023_entry(num_pe);
      status |= p024_entry(num_pe);
      status |= p025_entry(num_pe);
      status |= p026_entry(num_pe);
      status |= p027_entry(num_pe);
      status |= p028_entry(num_pe);
      status |= p029_entry(num_pe);
      status |= p030_entry(num_pe);
      status |= p031_entry(num_pe);
      status |= p032_entry(num_pe);
      status |= p033_entry(num_pe);
      status |= p034_entry(num_pe);
      status |= p035_entry(num_pe);
      status |= p036_entry(num_pe); /* iEP/RP only */
      status |= p037_entry(num_pe); /* iEP/RP only */
      status |= p038_entry(num_pe); /* iEP/RP only */
      status |= p039_entry(num_pe); /* iEP/RP only */
      status |= p041_entry(num_pe);
      status |= p042_entry(num_pe);
      status |= p043_entry(num_pe); /* iEP/RP only */
      status |= p044_entry(num_pe); /* iEP/RP only */
      status |= p045_entry(num_pe); /* iEP/RP only */
      status |= p046_entry(num_pe);
      status |= p047_entry(num_pe); /* iEP/RP only */
      status |= p048_entry(num_pe); /* iEP/RP only */
      status |= p049_entry(num_pe);
      status |= p050_entry(num_pe);
      status |= p051_entry(num_pe); /* iEP/RP only */
      status |= p052_entry(num_pe);
      status |= p056_entry(num_pe); /* iEP/RP only */
      status |= p057_entry(num_pe);
      status |= p058_entry(num_pe);
      status |= p059_entry(num_pe);
      status |= p060_entry(num_pe);
      status |= p063_entry(num_pe); /* iEP/RP only */
    #endif
  }

  if (level > 6) {
  #ifndef TARGET_LINUX
    status |= p061_entry(num_pe);
  #endif
  }

  val_print_test_end(status, "PCIe");

  return status;
}

void
val_pcie_print_device_info(void)
{
  uint32_t bdf;
  uint32_t dp_type;
  uint32_t tbl_index;
  uint32_t ecam_index;
  uint64_t ecam_base;
  uint32_t ecam_start_bus;
  uint32_t ecam_end_bus;
  pcie_device_bdf_table *bdf_tbl_ptr;
  uint32_t num_rciep = 0, num_rcec = 0;
  uint32_t num_iep = 0, num_irp = 0;
  uint32_t num_ep = 0, num_rp = 0;
  uint32_t num_dp = 0, num_up = 0;
  uint32_t num_pcie_pci = 0, num_pci_pcie = 0;
  uint32_t bdf_counter;

  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  tbl_index = 0;
  ecam_index = 0;

  if (bdf_tbl_ptr->num_entries == 0)
  {
    val_print(AVS_PRINT_DEBUG, "  BDF Table: No RCiEP or iEP found\n", 0);
    return;
  }

  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      switch (dp_type)
      {
        case RCiEP:
            num_rciep++;
            break;
        case RCEC:
            num_rcec++;
            break;
        case EP:
            num_ep++;
            break;
        case RP:
            num_rp++;
            break;
        case iEP_EP:
            num_iep++;
            break;
        case iEP_RP:
            num_irp++;
            break;
        case UP:
            num_up++;
            break;
        case DP:
            num_dp++;
            break;
        case PCI_PCIE:
            num_pci_pcie++;
            break;
        case PCIE_PCI:
            num_pcie_pci++;
            break;
      }
  }

  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of RCiEP           : %4d \n", num_rciep);
  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of RCEC            : %4d \n", num_rcec);
  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of EP              : %4d \n", num_ep);
  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of RP              : %4d \n", num_rp);
  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of iEP_EP          : %4d \n", num_iep);
  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of iEP_RP          : %4d \n", num_irp);
  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of UP of switch    : %4d \n", num_up);
  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of DP of switch    : %4d \n", num_dp);
  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of PCI/PCIe Bridge : %4d \n", num_pci_pcie);
  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of PCIe/PCI Bridge : %4d \n", num_pcie_pci);

  while (ecam_index < val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0))
  {
      ecam_base = val_pcie_get_info(PCIE_INFO_ECAM, ecam_index);
      ecam_start_bus = val_pcie_get_info(PCIE_INFO_START_BUS, ecam_index);
      ecam_end_bus = val_pcie_get_info(PCIE_INFO_END_BUS, ecam_index);
      tbl_index = 0;
      bdf_counter = 0;

      val_print(AVS_PRINT_INFO, "\n  ECAM %d:", ecam_index);
      val_print(AVS_PRINT_INFO, "  Base 0x%llx\n", ecam_base);

      while (tbl_index < bdf_tbl_ptr->num_entries)
      {
          uint32_t seg_num;
          uint32_t bus_num;
          uint32_t dev_num;
          uint32_t func_num;
          uint32_t device_id;
          uint32_t vendor_id;
          uint32_t reg_value;
          uint32_t bdf;
          uint64_t dev_ecam_base;

          bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
          seg_num  = PCIE_EXTRACT_BDF_SEG(bdf);
          bus_num  = PCIE_EXTRACT_BDF_BUS(bdf);
          dev_num  = PCIE_EXTRACT_BDF_DEV(bdf);
          func_num = PCIE_EXTRACT_BDF_FUNC(bdf);

          val_pcie_read_cfg(bdf, TYPE01_VIDR, &reg_value);
          device_id = (reg_value >> TYPE01_DIDR_SHIFT) & TYPE01_DIDR_MASK;
          vendor_id = (reg_value >> TYPE01_VIDR_SHIFT) & TYPE01_VIDR_MASK;

          dev_ecam_base = val_pcie_get_ecam_base(bdf);

          if ((ecam_base == dev_ecam_base) && (bus_num >= ecam_start_bus)
              && (bus_num <= ecam_end_bus))
          {
              bdf_counter = 1;
              bdf = PCIE_CREATE_BDF(seg_num, bus_num, dev_num, func_num);
              val_print(AVS_PRINT_INFO, "  BDF: 0x%x\n", bdf);
              val_print(AVS_PRINT_INFO, "  Seg: 0x%x, ", seg_num);
              val_print(AVS_PRINT_INFO, "Bus: 0x%02x, ", bus_num);
              val_print(AVS_PRINT_INFO, "Dev: 0x%02x, ", dev_num);
              val_print(AVS_PRINT_INFO, "Func: 0x%x, ", func_num);
              val_print(AVS_PRINT_INFO, "Dev ID: 0x%04x, ", device_id);
              val_print(AVS_PRINT_INFO, "Vendor ID: 0x%04x\n", vendor_id);
          }
      }
      if (bdf_counter == 0)
          val_print(AVS_PRINT_INFO, "  No BDF devices in ECAM region index %d\n", ecam_index);

      ecam_index++;
  }
}

/**
  @brief   This API will call PAL layer to fill in the PCIe information
           into the g_pcie_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   pcie_info_table    Memory address where PCIe Information is populated

  @return  Error if Input param is NULL
**/
void
val_pcie_create_info_table(uint64_t *pcie_info_table)
{
  if (pcie_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "Input for Create Info table cannot be NULL \n", 0);
      return;
  }


  g_pcie_info_table = (PCIE_INFO_TABLE *)pcie_info_table;

  pal_pcie_create_info_table(g_pcie_info_table);

  val_print(AVS_PRINT_TEST, " PCIE_INFO: Number of ECAM regions    :    %lx \n", val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0));

  val_pcie_enumerate();

  /* Create the list of valid Pcie Device Functions */
  if (val_pcie_create_device_bdf_table()) {
      val_print(AVS_PRINT_ERR, "Create Bdf table failed.\n", 0);
      return;
  }

  if (pal_pcie_check_device_list()) {
    pcie_bdf_table_list_flag = 1;
    val_print(AVS_PRINT_ERR, "Pcie device list doesn't match \
                with platform pcie device hierarchy\n", 0);
  }

  val_pcie_print_device_info();

}

/**
  @brief  Sanity checks that all Endpoints must have a Rootport

  @param  None
  @return 0 if sanity check passes, 1 if sanity check fails
**/
static uint32_t val_pcie_populate_device_rootport(void)
{
  uint32_t bdf;
  uint32_t rp_bdf;
  uint32_t tbl_index;
  pcie_device_bdf_table *bdf_tbl_ptr;

  bdf_tbl_ptr = val_pcie_bdf_table_ptr();
  tbl_index = 0;

  for (tbl_index = 0; tbl_index < bdf_tbl_ptr->num_entries; tbl_index++)
  {
      bdf = bdf_tbl_ptr->device[tbl_index].bdf;
      val_print(AVS_PRINT_DEBUG, "   Dev bdf 0x%06x", bdf);

      /* Checks if the BDF has RootPort */
      val_pcie_get_rootport(bdf, &rp_bdf);

      bdf_tbl_ptr->device[tbl_index].rp_bdf = rp_bdf;
      val_print(AVS_PRINT_DEBUG, "  RP bdf 0x%06x\n", rp_bdf);
  }
  return 0;
}

uint32_t
val_pcie_create_device_bdf_table()
{

  uint32_t num_ecam;
  uint32_t seg_num;
  uint32_t start_bus;
  uint32_t end_bus;
  uint32_t bus_index;
  uint32_t dev_index;
  uint32_t func_index;
  uint32_t ecam_index;
  uint32_t bdf;
  uint32_t reg_value;
  uint32_t cid_offset;
  uint32_t status;

  /* if table is already present, return success */
  if (g_pcie_bdf_table)
      return PCIE_SUCCESS;

  /* Allocate memory to store BDFs for the valid pcie device functions */
  g_pcie_bdf_table = (pcie_device_bdf_table *) pal_aligned_alloc(MEM_ALIGN_8K,
                                                                 PCIE_DEVICE_BDF_TABLE_SZ);
  if (!g_pcie_bdf_table)
  {
      val_print(AVS_PRINT_ERR, "\n       PCIe BDF table memory allocation failed          ", 0);
      return 1;
  }

  g_pcie_bdf_table->num_entries = 0;

  num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);
  if (num_ecam == 0)
  {
      val_print(AVS_PRINT_ERR, "\n       No ECAMs discovered              ", 0);
      return 1;
  }

  for (ecam_index = 0; ecam_index < num_ecam; ecam_index++)
  {
      /* Derive ecam specific information */
      seg_num = val_pcie_get_info(PCIE_INFO_SEGMENT, ecam_index);
      start_bus = val_pcie_get_info(PCIE_INFO_START_BUS, ecam_index);
      end_bus = val_pcie_get_info(PCIE_INFO_END_BUS, ecam_index);

      /* Iterate over all buses, devices and functions in this ecam */
      for (bus_index = start_bus; bus_index <= end_bus; bus_index++)
      {
          for (dev_index = 0; dev_index < PCIE_MAX_DEV; dev_index++)
          {
              for (func_index = 0; func_index < PCIE_MAX_FUNC; func_index++)
              {
                  /* Form bdf using seg, bus, device, function numbers */
                  bdf = PCIE_CREATE_BDF(seg_num, bus_index, dev_index, func_index);

                  /* Probe pcie device Function with this bdf */
                  if (val_pcie_read_cfg(bdf, TYPE01_VIDR, &reg_value) == PCIE_NO_MAPPING)
                  {
                      /* Return if there is a bdf mapping issue */
                      val_print(AVS_PRINT_ERR, "\n       BDF 0x%x mapping issue", bdf);
                      return 1;
                  }

                  /* Store the Function's BDF if there was a valid response */
                  if (reg_value != PCIE_UNKNOWN_RESPONSE)
                  {
                      /* Skip if the device is a host bridge */
                      if (val_pcie_is_host_bridge(bdf))
                          continue;

                      /* Skip if the device is a PCI legacy device */
                      if (val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS,  &cid_offset) != PCIE_SUCCESS)
                          continue;

                      status = pal_pcie_check_device_valid(bdf);
                      if (status)
                          continue;

                      g_pcie_bdf_table->device[g_pcie_bdf_table->num_entries++].bdf = bdf;
                  }
              }
          }
      }
  }

  /* Sanity Check : Confirm all EP (normal, integrated) have a rootport */
  val_pcie_populate_device_rootport();

  val_print(AVS_PRINT_TEST,
            " PCIE_INFO: Number of BDFs found      :    %d\n", g_pcie_bdf_table->num_entries);

  return 0;
}

/**
  @brief  Returns the ECAM address of the input PCIe function

  @param  bdf   - Segment/Bus/Dev/Func in PCIE_CREATE_BDF format
  @return ECAM address if success, else NULL address
**/
addr_t val_pcie_get_ecam_base(uint32_t bdf)
{

  uint8_t ecam_index;
  uint8_t sec_bus;
  uint8_t sub_bus;
  uint16_t seg_num;
  uint32_t reg_value;
  addr_t ecam_base;

  ecam_index = 0;
  ecam_base = 0;

  seg_num = PCIE_EXTRACT_BDF_SEG(bdf);

  while (ecam_index < val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0))
  {
      if (seg_num == val_pcie_get_info(PCIE_INFO_SEGMENT, ecam_index))
      {
          if (val_pcie_function_header_type(bdf) == TYPE0_HEADER)
          {
              /* Return ecam_base if Type0 Header */
              ecam_base = val_pcie_get_info(PCIE_INFO_ECAM, ecam_index);
              break;
          }
          else
          {
              /* Check for Secondary/Subordinate bus if Type1 Header */
              val_pcie_read_cfg(bdf, TYPE1_PBN, &reg_value);
              sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);
              sub_bus = ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK);

              if ((sec_bus >= val_pcie_get_info(PCIE_INFO_START_BUS, ecam_index)) &&
                  (sub_bus <= val_pcie_get_info(PCIE_INFO_END_BUS, ecam_index)))
              {
                  ecam_base = val_pcie_get_info(PCIE_INFO_ECAM, ecam_index);
                  break;
              }
          }
      }

      ecam_index++;
  }

  return ecam_base;
}

void *
val_pcie_bdf_table_ptr()
{
  return g_pcie_bdf_table;
}

/**
  @brief  Free the memory allocated for the pcie_info_table
**/
void
val_pcie_free_info_table()
{
  pal_mem_free((void *)g_pcie_info_table);
}


/**
  @brief   This API is the single entry point to return all PCIe related information
           1. Caller       -  Test Suite
           2. Prerequisite -  val_pcie_create_info_table
  @param   info_type  - Type of the information to be returned
  @param   index      - The index of the Ecam region whose information is returned

  @return  64-bit data pertaining to the requested input type
**/
uint64_t
val_pcie_get_info(PCIE_INFO_e type, uint32_t index)
{

  if (g_pcie_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "GET_PCIe_INFO: PCIE info table is not created \n", 0);
      return 0;
  }

  if (index >= g_pcie_info_table->num_entries) {
      if (g_pcie_info_table->num_entries != 0)
          val_print(AVS_PRINT_ERR, "Invalid index %d > num of entries \n", index);
      return 0;
  }

  switch (type) {
      case PCIE_INFO_NUM_ECAM:
          return g_pcie_info_table->num_entries;
      case PCIE_INFO_MCFG_ECAM:
          return pal_pcie_get_mcfg_ecam();
      case PCIE_INFO_ECAM:
          return g_pcie_info_table->block[index].ecam_base;
      case PCIE_INFO_START_BUS:
          return g_pcie_info_table->block[index].start_bus_num;
      case PCIE_INFO_END_BUS:
          return g_pcie_info_table->block[index].end_bus_num;
      case PCIE_INFO_SEGMENT:
          return g_pcie_info_table->block[index].segment_num;
      default:
          val_print(AVS_PRINT_ERR, "This PCIE info option not supported %d \n", type);
          break;
  }

  return 0;
}

/**
  @brief   This API returns list of MSI(X) vectors for a specified device
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @param   mvector  - Pointer to MSI vector's list head

  @return  number of MSI(X) vectors
**/
uint32_t
val_get_msi_vectors (uint32_t bdf, PERIPHERAL_VECTOR_LIST **mvector)
{
  return pal_get_msi_vectors (PCIE_EXTRACT_BDF_SEG (bdf),
                              PCIE_EXTRACT_BDF_BUS (bdf),
                              PCIE_EXTRACT_BDF_DEV (bdf),
                              PCIE_EXTRACT_BDF_FUNC (bdf),
                              mvector);
}

/**
  @brief   This API returns legacy interrupts routing map
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @param   irq_map  - Pointer to IRQ map structure

  @return  status code
**/
uint32_t
val_pci_get_legacy_irq_map (uint32_t bdf, PERIPHERAL_IRQ_MAP *irq_map)
{
  return pal_pcie_get_legacy_irq_map (PCIE_EXTRACT_BDF_SEG (bdf),
                                      PCIE_EXTRACT_BDF_BUS (bdf),
                                      PCIE_EXTRACT_BDF_DEV (bdf),
                                      PCIE_EXTRACT_BDF_FUNC (bdf),
                                      irq_map);
}

/**
  @brief   This API checks if device is behind SMMU
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  0 -> not present, nonzero -> present
**/
uint32_t
val_pcie_is_device_behind_smmu(uint32_t bdf)
{
  return pal_pcie_is_device_behind_smmu(PCIE_EXTRACT_BDF_SEG (bdf),
                                        PCIE_EXTRACT_BDF_BUS (bdf),
                                        PCIE_EXTRACT_BDF_DEV (bdf),
                                        PCIE_EXTRACT_BDF_FUNC (bdf));
}

/**
  @brief   This API checks if device is capable of 64-bit DMA
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  0 -> not 64-bit DMA capable, 1 -> 64-bit DMA capable
**/
uint32_t
val_pcie_is_devicedma_64bit(uint32_t bdf)
{

  uint32_t seg  = PCIE_EXTRACT_BDF_SEG (bdf);
  uint32_t bus  = PCIE_EXTRACT_BDF_BUS (bdf);
  uint32_t dev  = PCIE_EXTRACT_BDF_DEV (bdf);
  uint32_t func = PCIE_EXTRACT_BDF_FUNC (bdf);

  return (pal_pcie_is_devicedma_64bit(seg, bus, dev, func));
}

/**
  @brief   This API checks if device driver present for a pcie device
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  0 -> not Present, 1 -> Present
**/
uint32_t
val_pcie_device_driver_present(uint32_t bdf)
{

  uint32_t seg  = PCIE_EXTRACT_BDF_SEG (bdf);
  uint32_t bus  = PCIE_EXTRACT_BDF_BUS (bdf);
  uint32_t dev  = PCIE_EXTRACT_BDF_DEV (bdf);
  uint32_t func = PCIE_EXTRACT_BDF_FUNC (bdf);

  return pal_pcie_device_driver_present(seg, bus, dev, func);
}

/**
  @brief   This API scans bridge devices and checks memory type
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function

  @return  0 -> 32-bit mem type, 1 -> 64-bit mem type
**/
uint32_t val_pcie_scan_bridge_devices_and_check_memtype(uint32_t bdf)
{
  uint32_t Bus, Dev, Func;
  uint32_t sec_bus, sub_bus;
  uint32_t status = 0;
  uint32_t reg_value;
  uint32_t data;
  uint8_t  mem_type;

  val_pcie_read_cfg(bdf, TYPE1_PBN, &reg_value);
  sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);
  sub_bus = ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK);

  for (Bus = sec_bus; Bus <= sub_bus; Bus++)
  {
      for (Dev = 0; Dev < PCIE_MAX_DEV; Dev++)
      {
          for (Func = 0; Func < PCIE_MAX_FUNC; Func++)
          {
              if (val_pcie_function_header_type(bdf) == TYPE0_HEADER)
              {
                  val_pcie_read_cfg(bdf, TYPE01_BAR, &data);
                  if (data)
                  {
                      mem_type = ((data >> BAR_MDT_SHIFT) & BAR_MDT_MASK);
                      if (mem_type != 0) {
                          status = 1;
                          break;
                      }
                  }
              }
          }
      }
  }

  return status;
}

/**
  @brief   This API returns the bdf of root port
  @param   bdf             - PCIe BUS/Device/Function

  @return  status & BDF of root port
**/
uint32_t
val_pcie_get_root_port_bdf(uint32_t *bdf)
{
  uint32_t bus  = PCIE_EXTRACT_BDF_BUS (*bdf);
  uint32_t dev  = PCIE_EXTRACT_BDF_DEV (*bdf);
  uint32_t func = PCIE_EXTRACT_BDF_FUNC (*bdf);
  uint32_t seg  = PCIE_EXTRACT_BDF_SEG (*bdf);
  uint32_t status;
  status = pal_pcie_get_root_port_bdf(&seg, &bus, &dev, &func);
  if(status)
    return status;

  *bdf = PCIE_CREATE_BDF(seg, bus, dev, func);
  return 0;
}

/**
  @brief   This API returns the PCIe device type
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  0: Normal PCIe device, 1: PCIe bridge device,
           2: PCIe Host bridge, else: INVALID
**/
uint32_t
val_pcie_get_device_type(uint32_t bdf)
{

  uint32_t header_type, class_code;

  header_type = val_pcie_function_header_type(bdf);
  if (header_type != TYPE0_HEADER)
  {
      val_pcie_read_cfg(bdf, TYPE01_RIDR, &class_code);
      if ((((class_code >> CC_BASE_SHIFT) & CC_BASE_MASK) == HB_BASE_CLASS) &&
           (((class_code >> CC_SUB_SHIFT) & CC_SUB_MASK)) == HB_SUB_CLASS)
          return 2;
      else
          return 3;
  }
  else
      return 1;
}

/**
  @brief   This API checks the PCIe Hierarchy P2P support
           1. Caller       -  Test Suite
  @return  1 - P2P feature not supported 0 - P2P feature supported
**/
uint32_t
val_pcie_p2p_support()
{
  return pal_pcie_p2p_support();
}

/**
  @brief   This API checks the PCIe Root port supports P2P with other RP's
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  1 - P2P feature not supported 0 - P2P feature supported
**/
uint32_t
val_pcie_dev_p2p_support(uint32_t bdf)
{
  return pal_pcie_dev_p2p_support(PCIE_EXTRACT_BDF_SEG (bdf),
                                    PCIE_EXTRACT_BDF_BUS (bdf),
                                    PCIE_EXTRACT_BDF_DEV (bdf),
                                    PCIE_EXTRACT_BDF_FUNC (bdf));
}

/**
  @brief   This API checks the PCIe device multifunction support
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  1 - Multifunction not supported 0 - Multifunction supported
**/
uint32_t
val_pcie_multifunction_support(uint32_t bdf)
{
  uint32_t reg_data;
  val_pcie_read_cfg(bdf, TYPE01_CLSR, &reg_data);
  reg_data = ((reg_data >> TYPE01_HTR_SHIFT) & TYPE01_HTR_MASK);

  return !((reg_data >> HTR_MFD_SHIFT) & HTR_MFD_MASK);
}

/**
  @brief   This API returns the PCIe device/port type
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  Returns the PCIe device/port type
**/
uint32_t
val_pcie_get_pcie_type(uint32_t bdf)
{
  return pal_pcie_get_pcie_type(PCIE_EXTRACT_BDF_SEG (bdf),
                                PCIE_EXTRACT_BDF_BUS (bdf),
                                PCIE_EXTRACT_BDF_DEV (bdf),
                                PCIE_EXTRACT_BDF_FUNC (bdf));
}

/**
  @brief   This API returns PCIe device snoop bit transaction attribute
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  0 snoop
           1 no snoop
           2 device error
**/
uint32_t
val_pcie_get_snoop_bit(uint32_t bdf)
{
  return pal_pcie_get_snoop_bit(PCIE_EXTRACT_BDF_SEG (bdf),
                                PCIE_EXTRACT_BDF_BUS (bdf),
                                PCIE_EXTRACT_BDF_DEV (bdf),
                                PCIE_EXTRACT_BDF_FUNC (bdf));
}

/**
  @brief   This API returns PCIe device DMA support
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  0 no support
           1 support
           2 device error
**/
uint32_t
val_pcie_get_dma_support(uint32_t bdf)
{
  return pal_pcie_get_dma_support(PCIE_EXTRACT_BDF_SEG (bdf),
                                  PCIE_EXTRACT_BDF_BUS (bdf),
                                  PCIE_EXTRACT_BDF_DEV (bdf),
                                  PCIE_EXTRACT_BDF_FUNC (bdf));
}

/**
  @brief   This API returns PCIe device DMA coherency support
           1. Caller       -  Test Suite
  @param   bdf      - PCIe BUS/Device/Function
  @return  0 DMA is not coherent
           1 DMA is coherent
           2 device error
**/
uint32_t
val_pcie_get_dma_coherent(uint32_t bdf)
{
  return pal_pcie_get_dma_coherent(PCIE_EXTRACT_BDF_SEG (bdf),
                                   PCIE_EXTRACT_BDF_BUS (bdf),
                                   PCIE_EXTRACT_BDF_DEV (bdf),
                                   PCIE_EXTRACT_BDF_FUNC (bdf));
}

/**
  @brief  Increment the Dev/Bus number to the next valid value.
  @param  start_bdf - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return the new incremented BDF
**/
uint32_t
val_pcie_increment_busdev(uint32_t start_bdf)
{

  uint32_t seg = PCIE_EXTRACT_BDF_SEG(start_bdf);
  uint32_t bus = PCIE_EXTRACT_BDF_BUS(start_bdf);
  uint32_t dev = PCIE_EXTRACT_BDF_DEV(start_bdf);

  if (dev != PCIE_MAX_DEV) {
      dev++;
  } else {
      bus++;
      dev = 0;
  }

  return PCIE_CREATE_BDF(seg, bus, dev, 0);
}

/**
  @brief  Increment the segment/Bus/Dev/Bus number to the next valid value.
  @param  bdf - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return the new incremented BDF, else 0 to indicate incorrect input
**/
uint32_t
val_pcie_increment_bdf(uint32_t bdf)
{

  uint32_t seg;
  uint32_t bus;
  uint32_t dev;
  uint32_t func;
  int32_t ecam_cnt;
  uint32_t ecam_index = 0;

  seg = PCIE_EXTRACT_BDF_SEG(bdf);
  bus = PCIE_EXTRACT_BDF_BUS(bdf);
  dev = PCIE_EXTRACT_BDF_DEV(bdf);
  func = PCIE_EXTRACT_BDF_FUNC(bdf);

  /* Derive the ecam index to which sbdf belongs */
  ecam_cnt = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);
  while (ecam_cnt--) {
      if (seg == val_pcie_get_info(PCIE_INFO_SEGMENT, ecam_cnt)) {
          ecam_index = ecam_cnt;
          break;
      }
  }

  /* Return 0 to indicate incorrect input sbdf */
  if (ecam_cnt < 0)
    return 0;

  /* Find the next Segment/Bus/Dev/Func */
  if (func < (PCIE_MAX_FUNC-1)) {
      func++;
  } else {
      func = 0;
      if (dev < (PCIE_MAX_DEV-1)) {
          dev++;
      } else {
          dev = 0;
          if (bus < val_pcie_get_info(PCIE_INFO_END_BUS, ecam_index)) {
              bus++;
          } else {
              if ((ecam_index+1) < val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0)) {
                  bus = val_pcie_get_info(PCIE_INFO_START_BUS, ecam_index+1);
                  seg = val_pcie_get_info(PCIE_INFO_SEGMENT, ecam_index+1);
              }
              else {
                  return 0;
              }
          }
     }
  }

  return PCIE_CREATE_BDF(seg, bus, dev, func);
}

/**
    @brief   Returns the Bus, Dev, Function in the form (seg<<24 | bus<<16 | Dev <<8 | func)
    @param   class_code - is a 32bit value of format ClassCode << 16 | sub_class_code
    @param   start_bdf  - is 0     : start enumeration from Host bridge
                          is not 0 : start enumeration from the input segment, bus, dev
                          this is needed as multiple controllers with same class code are
                          potentially present in a system.
    @return  the BDF of the device matching the class code
**/
uint32_t
val_pcie_get_bdf(uint32_t class_code, uint32_t start_bdf)
{

  return pal_pcie_get_bdf_wrapper(class_code, start_bdf);
}

void *
val_pci_bdf_to_dev(uint32_t bdf)
{
  return pal_pci_bdf_to_dev(bdf);
}

void
val_pcie_read_ext_cap_word(uint32_t bdf, uint32_t ext_cap_id, uint8_t offset, uint16_t *val)
{
  pal_pcie_read_ext_cap_word(PCIE_EXTRACT_BDF_SEG (bdf),
                             PCIE_EXTRACT_BDF_BUS(bdf),
                             PCIE_EXTRACT_BDF_DEV(bdf),
                             PCIE_EXTRACT_BDF_FUNC(bdf),
                             ext_cap_id, offset, val);
}

/**
  @brief  Returns whether a PCIe Function is an on-chip peripheral or not

  @param  bdf        - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return Returns TRUE if the Function is on-chip peripheral, FALSE if it is
          not an on-chip peripheral
**/
uint32_t
val_pcie_is_onchip_peripheral(uint32_t bdf)
{
  return pal_pcie_is_onchip_peripheral(bdf);
}

/**
  @brief  Returns whether a PCIe Function is atomicop requester capable

  @param  bdf        - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return Returns 0 (if Function doesn't supports atomicop requester capable
                     else non-zero value)
**/
uint32_t
val_pcie_get_atomicop_requester_capable(uint32_t bdf)
{
  /* TO DO */
  //return pal_pcie_get_atomicop_requester_capable(bdf);
  return 0;
}

/**
  @brief  Returns the type of pcie device or port for the given bdf

  @param  bdf     - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return Returns (1 << 0b1001) for RCiEP,  (1 << 0b1010) for RCEC,
                  (1 << 0b0000) for EP,     (1 << 0b0100) for RP,
                  (1 << 0b1100) for iEP_EP, (1 << 0b1011) for iEP_RP,
                  (1 << PCIECR[7:4]) for any other device type.
**/
uint32_t
val_pcie_device_port_type(uint32_t bdf)
{

  uint32_t pciecs_base;
  uint32_t reg_value;
  uint32_t dp_type;

  /* Get the PCI Express Capability structure offset and
   * use that offset to read pci express capabilities register
   */
  val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
  val_pcie_read_cfg(bdf, pciecs_base + CIDR_OFFSET, &reg_value);

  /* Read Device/Port bits [7:4] in Function's PCIe Capabilities register */
  dp_type = (reg_value >> ((PCIECR_OFFSET - CIDR_OFFSET)*8 +
                            PCIECR_DPT_SHIFT)) & PCIECR_DPT_MASK;
  dp_type = (1 << dp_type);

  /* Check if the device/port is an on-chip peripheral */
  if (val_pcie_is_onchip_peripheral(bdf))
  {
      if (dp_type == EP)
          dp_type = iEP_EP;
      else if (dp_type == RP)
          dp_type = iEP_RP;
  }

  /* Return device/port type */
  return dp_type;
}

/**
  @brief  Find a Function's config capability offset matching it's input parameter
          cid. cid_offset set to the matching cpability offset w.r.t. zero.

  @param  bdf        - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @param  cid        - Capability ID
  @param  cid_offset - On return, points to cid offset in Function config space
  @return PCIE_CAP_NOT_FOUND, if there was a failure in finding required capability.
          PCIE_SUCCESS, if the search was successful.
**/
uint32_t
val_pcie_find_capability(uint32_t bdf, uint32_t cid_type, uint32_t cid, uint32_t *cid_offset)
{

  uint32_t reg_value;
  uint32_t next_cap_offset;
  uint32_t ret;

  if (cid_type == PCIE_CAP) {

      /* Serach in PCIe configuration space */
      ret = val_pcie_read_cfg(bdf, TYPE01_CPR, &reg_value);
      if (ret == PCIE_NO_MAPPING || reg_value == PCIE_UNKNOWN_RESPONSE)
          return ret;

      next_cap_offset = (reg_value & TYPE01_CPR_MASK);
      while (next_cap_offset)
      {
          val_pcie_read_cfg(bdf, next_cap_offset, &reg_value);
          if ((reg_value & PCIE_CIDR_MASK) == cid)
          {
              *cid_offset = next_cap_offset;
              return PCIE_SUCCESS;
          }
          next_cap_offset = ((reg_value >> PCIE_NCPR_SHIFT) & PCIE_NCPR_MASK);
      }
  } else if (cid_type == PCIE_ECAP)
  {

      /* Serach in PCIe extended configuration space */
      next_cap_offset = PCIE_ECAP_START;
      while (next_cap_offset)
      {
          val_pcie_read_cfg(bdf, next_cap_offset, &reg_value);
          if ((reg_value & PCIE_ECAP_CIDR_MASK) == cid)
          {
              *cid_offset = next_cap_offset;
              return PCIE_SUCCESS;
          }
          next_cap_offset = ((reg_value >> PCIE_ECAP_NCPR_SHIFT) & PCIE_ECAP_NCPR_MASK);
      }
  }

  /* The capability was not found */
  return PCIE_CAP_NOT_FOUND;
}

/**
  @brief  Disables bus master by clearing Bus Master Enable bit in the command register.
          When BME bit is clear, it disables the ability of a Function to issue Memory
          Read/Write Requests, and the ability of a Port to forward Memory Read/Write
          Requests in the Upstream direction.

  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return None
**/
void
val_pcie_disable_bme(uint32_t bdf)
{

  uint32_t reg_value;
  uint32_t dis_mask;

  /* Clear BME bit in Command Register to disable ability to issue Memory Requests */
  val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value);
  dis_mask = ~(1 << CR_BME_SHIFT);
  val_pcie_write_cfg(bdf, TYPE01_CR, reg_value & dis_mask);

}

/**
  @brief  Gets RP support of transaction forwarding.

  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return 0 if rp not involved in transaction forwarding
**/
uint32_t
val_pcie_get_rp_transaction_frwd_support(uint32_t bdf)
{
  return pal_pcie_get_rp_transaction_frwd_support(PCIE_EXTRACT_BDF_SEG (bdf),
                                                  PCIE_EXTRACT_BDF_BUS (bdf),
                                                  PCIE_EXTRACT_BDF_DEV (bdf),
                                                  PCIE_EXTRACT_BDF_FUNC(bdf));
}

/**
  @brief  Enables ability of a function to issue Memory Requests by setting Bus
          Master Enable bit in the command register.

  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return None
**/
void
val_pcie_enable_bme(uint32_t bdf)
{

  uint32_t reg_value;

  /* Set BME bit in Command Register to enable ability to issue Memory Requests */
  val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value);
  val_pcie_write_cfg(bdf, TYPE01_CR, reg_value | (1 << CR_BME_SHIFT));

}

/**
  @brief  Disables BAR memory space access by clearring memory space enable bit
          in the command register. When MSE bit is clear, all received memory
          space accesses are caused to be handled as Unsupported Requests.

  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return None
**/
void
val_pcie_disable_msa(uint32_t bdf)
{

  uint32_t reg_value;
  uint32_t dis_mask;

  /* Clear MSE bit in Command Register to disable BAR memory space accesses */
  val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value);
  dis_mask = ~(1 << CR_MSE_SHIFT);
  val_pcie_write_cfg(bdf, TYPE01_CR, reg_value & dis_mask);

}

/**
  @brief  Enables BAR memory space access by setting memory space enable bit
          in the command register. When MSE bit is Set, the Function is enabled
          to decode the address and further process Memory Space accesses.

  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return None
**/
void
val_pcie_enable_msa(uint32_t bdf)
{

  uint32_t reg_value;

  /* Enable MSE bit in Command Register to enable BAR memory space accesses */
  val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value);
  val_pcie_write_cfg(bdf, TYPE01_CR, reg_value | (1 << CR_MSE_SHIFT));

}

/**
  @brief  Reads the BAR memory space access in the command register.

  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return 0 - Success
          1 - Failure
**/
uint32_t
val_pcie_is_msa_enabled(uint32_t bdf)
{

  uint32_t reg_value;

  /* Enable MSE bit in Command Register to enable BAR memory space accesses */
  val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value);
  reg_value &= (1 << CR_MSE_SHIFT);
  if (reg_value)
      return 0;
  else
      return 1;
}

/**
  @brief  Clears unsupported request detected bit in Device Status Register

  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return None
**/
void
val_pcie_clear_urd(uint32_t bdf)
{

  uint32_t pciecs_base;
  uint32_t reg_value;

  /*
   * Get the PCI Express Capability structure offset and use that
   * offset to write 1b to clear URD bit in Device Status register
   */
  val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
  val_pcie_read_cfg(bdf, pciecs_base + DCTLR_OFFSET, &reg_value);
  reg_value &= DCTLR_MASK;
  reg_value |= (1 << (DCTLR_DSR_SHIFT + DSR_URD_SHIFT));
  val_pcie_write_cfg(bdf, pciecs_base + DCTLR_OFFSET, reg_value);

}

/**
  @brief  Returns whether a PCIe Function has detected an Unsupported Request

  @param  bdf        - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return Returns TRUE if the Function has received an Unsupported Request or
          FALSE if it does not receive Unsupported Request
**/
uint32_t
val_pcie_is_urd(uint32_t bdf)
{

  uint32_t pciecs_base;
  uint32_t reg_value;

  /* Get the PCI Express Capability structure offset and
   * use that offset to read the Device Status register
   */
  val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
  val_pcie_read_cfg(bdf, pciecs_base + DCTLR_OFFSET, &reg_value);

  /* Check if URD bit is set in Function's Device Control register */
  reg_value = (reg_value >> DCTLR_DSR_SHIFT) & DCTLR_DSR_MASK;
  if ((reg_value >> DSR_URD_SHIFT) & DSR_URD_MASK)
      return 1;

  /* Hasn't received UR */
  return 0;
}

/**
  @brief  Clears Error detected bit in Device Status Register

  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return None
**/
void
val_pcie_clear_device_status_error(uint32_t bdf)
{

  uint32_t pciecs_base;
  uint32_t reg_value;

  /*
   * Get the PCI Express Capability structure offset and use that
   * offset to write 1b to clear CED, NFED, FED, URD bit in Device Status register
   */
  val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
  val_pcie_read_cfg(bdf, pciecs_base + DCTLR_OFFSET, &reg_value);
  reg_value = reg_value | (0xF << DCTLR_DSR_SHIFT);
  val_pcie_write_cfg(bdf, pciecs_base + DCTLR_OFFSET, reg_value);

}

/**
  @brief  Check Error detected bit in Device Status Register

  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return 1 if Error is detected, 0 if No Error
**/
uint32_t
val_pcie_is_device_status_error(uint32_t bdf)
{

  uint32_t pciecs_base;
  uint32_t reg_value;

  /*
   * Get the PCI Express Capability structure offset and use that
   * offset to check CED, NFED, FED, URD bit in Device Status register
   */
  val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
  val_pcie_read_cfg(bdf, pciecs_base + DCTLR_OFFSET, &reg_value);

  if (reg_value & (0xF << DCTLR_DSR_SHIFT))
      return 1;

  return 0;
}

/**
  @brief  Clear Signaled Target Abort bit in Status/Secondary Status Register
          in Root Port
  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return None
**/
void
val_pcie_clear_sig_target_abort(uint32_t bdf)
{

  uint32_t status_val;
  uint32_t sec_status_val;

  /* Read Status Register at 0x4 Offset */
  val_pcie_read_cfg(bdf, TYPE01_CR, &status_val);
  val_pcie_write_cfg(bdf, TYPE01_CR, (status_val | (1 << SR_STA_SHIFT)));

  /* Read Secondary Status Register at 0x1C Offset */
  val_pcie_read_cfg(bdf, TYPE1_SEC_STA, &sec_status_val);
  val_pcie_write_cfg(bdf, TYPE1_SEC_STA, (sec_status_val | (1 << SSR_STA_SHIFT)));
}

/**
  @brief  Check Signaled Target Abort bit in Status/Secondary Status Register
          in Root Port
  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return 1 if STA Bit Set, 0 if Not Set
**/
uint32_t
val_pcie_is_sig_target_abort(uint32_t bdf)
{

  uint32_t status_val;
  uint32_t sec_status_val;

  /* Read Status Register at 0x4 Offset */
  val_pcie_read_cfg(bdf, TYPE01_CR, &status_val);
  /* Read Secondary Status Register at 0x1C Offset */
  val_pcie_read_cfg(bdf, TYPE1_SEC_STA, &sec_status_val);

  if (((status_val >> SR_STA_SHIFT) & SR_STA_MASK) ||
      ((sec_status_val >> SSR_STA_SHIFT) & SSR_STA_MASK))
      return 1;

  return 0;
}

/**
  @brief  Enable error reporting of the PCIe Function to the upstream
  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return None
**/
void
val_pcie_enable_eru(uint32_t bdf)
{

  uint32_t reg_value;
  uint32_t dis_mask;
  uint32_t pciecs_base;

  /* Set SERR# Enable bit in the Command Register to enable reporting
   * upstream of Non-fatal and Fatal errors detected by the Function.
   */
  val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value);
  dis_mask = (1 << CR_SERRE_SHIFT);
  val_pcie_write_cfg(bdf, TYPE01_CR, reg_value | dis_mask);

  /* Get the PCI Express Capability structure offset and
   * use that offset to read the Device Control register
   */
  val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
  val_pcie_read_cfg(bdf, pciecs_base + DCTLR_OFFSET, &reg_value);

  /* Set Correctable, Non-fatal, Fatal, UR Reporting Enable bits in the
   * Device Control Register to enable reporting upstream of these errors
   * detected by the Function.
   */
  dis_mask = (1 << DCTLR_CERE_SHIFT |
              1 << DCTLR_NFERE_SHIFT |
              1 << DCTLR_FERE_SHIFT |
              1 << DCTLR_URRE_SHIFT);
  val_pcie_write_cfg(bdf, pciecs_base + DCTLR_OFFSET, reg_value | dis_mask);
}

/**
  @brief  Disable error reporting of the PCIe Function to the upstream
  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return None
**/
void
val_pcie_disable_eru(uint32_t bdf)
{

  uint32_t reg_value;
  uint32_t dis_mask;
  uint32_t pciecs_base;

  /* Clear SERR# Enable bit in the Command Register to disable reporting
   * upstream of Non-fatal and Fatal errors detected by the Function.
   */
  val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value);
  dis_mask = ~(1 << CR_SERRE_SHIFT);
  val_pcie_write_cfg(bdf, TYPE01_CR, reg_value & dis_mask);

  /* Get the PCI Express Capability structure offset and
   * use that offset to read the Device Control register
   */
  val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
  val_pcie_read_cfg(bdf, pciecs_base + DCTLR_OFFSET, &reg_value);

  /* Clear Correctable, Non-fatal, Fatal, UR Reporting Enable bits in the
   * Device Control Register to disable reporting upstream of these errors
   * detected by the Function.
   */
  dis_mask = ~(1 << DCTLR_CERE_SHIFT |
               1 << DCTLR_NFERE_SHIFT |
               1 << DCTLR_FERE_SHIFT |
               1 << DCTLR_URRE_SHIFT);
  val_pcie_write_cfg(bdf, pciecs_base + DCTLR_OFFSET, reg_value & dis_mask);
}

/**
  @brief  Returns whether a device's bit-field passed the compliance check or not.
          The device under test is indicated by input bdf.

  @param  bdf           - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @param  bitfield_entry- Expected bit-field entry configuration for the comparison
  @return Return 0 for success, else 1 for failure.
**/
uint32_t val_pcie_bitfield_check(uint32_t bdf, uint64_t *bitfield_entry)
{

  uint32_t bf_value;
  uint32_t cap_base;
  uint32_t reg_value;
  uint32_t reg_offset;
  uint32_t temp_reg_value;
  uint32_t reg_overwrite_value;
  uint32_t alignment_byte_cnt;
  uint32_t status = PCIE_SUCCESS;

  pcie_cfgreg_bitfield_entry *bf_entry;

  bf_entry = (pcie_cfgreg_bitfield_entry *)bitfield_entry;

  /*
   * Calculate word alignment byte count and adjust
   * reg_offset to read word-aligned offsets always.
   */
  reg_offset = bf_entry->reg_offset;
  alignment_byte_cnt = (reg_offset & WORD_ALIGN_MASK);
  reg_offset = reg_offset - alignment_byte_cnt;

  switch (bf_entry->reg_type)
  {
      case HEADER:
          cap_base = 0;
          break;
      case PCIE_CAP:
          status = val_pcie_find_capability(bdf, PCIE_CAP, bf_entry->cap_id, &cap_base);
          break;
      case PCIE_ECAP:
          status = val_pcie_find_capability(bdf, PCIE_ECAP, bf_entry->ecap_id, &cap_base);
          break;
      default:
          val_print(AVS_PRINT_ERR, "\n       Invalid reg_type : 0x%x  ", bf_entry->reg_type);
          return 1;
  }

  if (status != PCIE_SUCCESS)
  {
      val_print(AVS_PRINT_ERR, "\n       PCIe Capability not found for BDF 0x%x", bdf);
      return status;
  }

  /* Derive bit-field of interest from the register value */
  val_pcie_read_cfg(bdf, cap_base + reg_offset, &reg_value);

  /* To prevent status bits are clear when write 1, just clear it firstly */
  val_pcie_write_cfg(bdf, cap_base + reg_offset, reg_value);
  val_pcie_read_cfg(bdf, cap_base + reg_offset, &reg_value);

  bf_value = (reg_value >> REG_SHIFT(alignment_byte_cnt, bf_entry->start)) &
                    REG_MASK(bf_entry->end, bf_entry->start);

  /* Check if bit-field value is proper */
  if (bf_value != bf_entry->cfg_value)
  {
      val_print(AVS_PRINT_ERR, "\n       BDF 0x%x : ", bdf);
      val_print(AVS_PRINT_ERR, bf_entry->err_str1, 0);
      val_print(AVS_PRINT_ERR, ": 0x%x", bf_value);
      val_print(AVS_PRINT_ERR, " instead of 0x%x", bf_entry->cfg_value);
      if (!val_strncmp(bf_entry->err_str1, "WARNING", WARN_STR_LEN))
          return 0;
      return 1;
  }

  /* Check if bit-field attribute is proper */
  switch (bf_entry->attr)
  {
      case HW_INIT:
      case READ_ONLY:
      case STICKY_RO:
          /* Software must not alter these bits */
          reg_overwrite_value = reg_value ^ (REG_MASK(bf_entry->end, bf_entry->start) <<
                                       REG_SHIFT(alignment_byte_cnt, bf_entry->start));
          val_pcie_write_cfg(bdf, cap_base + reg_offset, reg_overwrite_value);
          val_pcie_read_cfg(bdf, cap_base + reg_offset, &reg_overwrite_value);
          break;
      case RSVDP_RO:
          /* Software must preserve the value read to write to these bits */
          reg_overwrite_value = reg_value;
          val_pcie_write_cfg(bdf, cap_base + reg_offset, reg_overwrite_value);
          val_pcie_read_cfg(bdf, cap_base + reg_offset, &reg_overwrite_value);
          reg_overwrite_value = (reg_overwrite_value >> REG_SHIFT(alignment_byte_cnt, bf_entry->start)) &
                    REG_MASK(bf_entry->end, bf_entry->start);
          /* Software must return 0 when read */
          reg_value = 0;
          break;
      case RSVDZ_RO:
          /* Software must use 0b to write to these bits */
          reg_overwrite_value = reg_value & (~(REG_MASK(bf_entry->end, bf_entry->start) <<
                                       REG_SHIFT(alignment_byte_cnt, bf_entry->start)));
          val_pcie_write_cfg(bdf, cap_base + reg_offset, reg_overwrite_value);
          val_pcie_read_cfg(bdf, cap_base + reg_offset, &reg_overwrite_value);
          break;
      case READ_WRITE:
      case STICKY_RW:
          /* Software can alter these bits, toggle the required bits and write to register */
          temp_reg_value = reg_value;
          reg_overwrite_value = reg_value ^ (REG_MASK(bf_entry->end, bf_entry->start) <<
                                       REG_SHIFT(alignment_byte_cnt, bf_entry->start));
          val_pcie_write_cfg(bdf, cap_base + reg_offset, reg_overwrite_value);
          val_pcie_read_cfg(bdf, cap_base + reg_offset, &reg_value);
          /* Restore the original register value */
          val_pcie_write_cfg(bdf, cap_base + reg_offset, temp_reg_value);
          break;
      default:
          val_print(AVS_PRINT_ERR, "\n       Invalid Attribute : 0x%x  ", bf_entry->attr);
          return 1;
  }

  if (reg_overwrite_value != reg_value)
  {
      val_print(AVS_PRINT_ERR, "\n       BDF 0x%x : ", bdf);
      val_print(AVS_PRINT_ERR, bf_entry->err_str2, 0);
      val_print(AVS_PRINT_ERR, ": 0x%x", reg_overwrite_value);
      val_print(AVS_PRINT_ERR, " instead of 0x%x", reg_value);
      if (!val_strncmp(bf_entry->err_str2, "WARNING", WARN_STR_LEN))
          return 0;
      return 1;
  }

  /* Return pass status */
  val_print(AVS_PRINT_INFO, "\n       BDF 0x%x : PASS", bdf);
  return 0;
}

/**
  @brief  Returns if a PCIe config register bitfields are as per sbsa specification.

  @param  bf_info_table - table of registers and their bit-fields for checking
  @return Return  0                 for success
                  AVS_STATUS_SKIP   if no checks are executed
                  <value>           number of failures.
**/
uint32_t
val_pcie_register_bitfields_check(uint64_t *bf_info_table, uint32_t num_bitfield_entries)
{

  uint32_t bdf;
  uint16_t dp_type;
  uint32_t tbl_index;
  uint32_t num_fails;
  uint32_t num_pass;
  uint32_t index;
  pcie_cfgreg_bitfield_entry *bf_entry;

  num_fails = num_pass = tbl_index = 0;

  val_print(AVS_PRINT_INFO, "\n       Number of bit-field entries to check %d",
            num_bitfield_entries);

  while (tbl_index < g_pcie_bdf_table->num_entries)
  {
      bdf = g_pcie_bdf_table->device[tbl_index++].bdf;

      /* Disable error reporting of this Function to the Upstream */
      val_pcie_disable_eru(bdf);

      /* Get the Function's device/port type from bdf */
      dp_type = val_pcie_device_port_type(bdf);

      /* Set variables to iterate over all bit-field entries */
      bf_entry = (pcie_cfgreg_bitfield_entry *)&(bf_info_table[0]);

      for (index = 0; index < num_bitfield_entries; index++)
      {
          /*
           * Skip this entry checking, if the Function
           * is not part of it's device/port bit mask.
           */
          if (!(dp_type & bf_entry->dev_port_bitmask))
          {
              bf_entry++;
              continue;
          }

          /* Check for the compliance */
          if (val_pcie_bitfield_check(bdf, (void *)bf_entry))
              num_fails++;
          else
              num_pass++;

          /* Adjust the pointer to next bf */
          bf_entry++;
      }
  }

  /* Return register check status */
  if (num_pass > 0 || num_fails > 0)
      return num_fails;
  else
      return AVS_STATUS_SKIP;
}

/**
  @brief  Returns the header type of the input pcie device function

  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return TYPE0_HEADER for functions with Type 0 config space header,
          TYPE1_HEADER for functions with Type 1 config space header,
*/
uint32_t
val_pcie_function_header_type(uint32_t bdf)
{

  uint32_t reg_value;

  /* Read four bytes of config space starting from cache line size register */
  val_pcie_read_cfg(bdf, TYPE01_CLSR, &reg_value);

  /* Extract header type register value */
  reg_value = ((reg_value >> TYPE01_HTR_SHIFT) & TYPE01_HTR_MASK);

  /* Header layout bits within header type register indicate the header type */
  return ((reg_value >> HTR_HL_SHIFT) & HTR_HL_MASK);
}

/**
  @brief  Returns physical address of the first MMIO Base Address Register

  @param  bdf   - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @param  base  - Base Address Register address in 64-bit format
  @return Success BAR address in 64-bit format, if found. Else NULL pointer.
**/
void
val_pcie_get_mmio_bar(uint32_t bdf, void *base)
{

  uint32_t index;
  uint32_t *base_ptr;
  uint32_t bar_low32bits;
  uint32_t bar_high32bits;
  uint64_t ecam;
  uint32_t status;
  exerciser_data_t data;

  if (pal_is_bdf_exerciser(bdf))
  {
      ecam = val_pcie_get_ecam_base(bdf);
      status = pal_exerciser_get_data(EXERCISER_DATA_MMIO_SPACE, &data, bdf, ecam);
      if (status == NOT_IMPLEMENTED)
      {
          val_print(AVS_PRINT_ERR, "\n       pal_exerciser_get_data() not implemented", 0);
      }

      /* data.bar_space.base_addr will be zero if no MMIO bar are present for the function */
      *(uint64_t *)base = (uint64_t)data.bar_space.base_addr;
      return;
  }

  index = 0;
  base_ptr = (uint32_t *) base;
  while (index < TYPE0_MAX_BARS)
  {
      /* Read the base address register at loop index */
      val_pcie_read_cfg(bdf, TYPE01_BAR + index * 4, &bar_low32bits);

      /* Check if the BAR is Memory Mapped IO type */
      if (((bar_low32bits >> BAR_MIT_SHIFT) & BAR_MIT_MASK) == MMIO)
      {
          /* Check if the BAR is 64-bit decodable */
          if (((bar_low32bits >> BAR_MDT_SHIFT) & BAR_MDT_MASK) == BITS_64)
          {
              /* Read the second sequential BAR at next index */
              val_pcie_read_cfg(bdf, TYPE01_BAR + (index + 1) * 4, &bar_high32bits);

              /* Fill upper 32-bits of 64-bit address with second sequential BAR */
              base_ptr[1] = bar_high32bits;

              /* Adjust the index to skip next sequential BAR */
              index++;

          } else if (((bar_low32bits >> BAR_MDT_SHIFT) & BAR_MDT_MASK) == BITS_32)
          {
              /* Fill upper 32-bits of 64-bit address with zeros */
              base_ptr[1] = 0;
          }

          /* Fill lower 32-bits of 64-bit address with first sequential BAR */
          base_ptr[0] = ((bar_low32bits >> (BAR_BASE_SHIFT) & BAR_BASE_MASK)) << BAR_BASE_SHIFT;

          return;
      }

      /* Adjust index to point to next BAR */
      index++;

      /*
       * Functions that implement Type 1 configuration header are limited by two BARs.
       * Terminate the search after index reaches max Type 1 BARs. The header layout bits
       * in header type register provide the type of the configuration header.
       */
      if ((val_pcie_function_header_type(bdf) == TYPE1_HEADER) && (index == TYPE1_MAX_BARS))
          break;

  }

  /* Return NULL pointer to indicate unavailablity of mmio BAR */
  base_ptr[0] = 0;
  base_ptr[1] = 0;

}

/**
  @brief  Returns BDF of first found downstream Function of a pcie bridge device.
          The search is in the order of type 0 followed by type 1 functions.

  @param  bdf       - Bridge's Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @param  dsf_bdf   - Bridge's downstream function bdf in PCIE_CREATE_BDF format
  @return 0 for success, 1 for failure.
**/
uint32_t
val_pcie_get_downstream_function(uint32_t bdf, uint32_t *dsf_bdf)
{

  uint32_t index;
  uint32_t sec_bus;
  uint32_t sub_bus;
  uint32_t seg;
  uint32_t reg_value;
  uint32_t type1_bdf;
  uint32_t type1_flag;

  type1_bdf = 0;
  *dsf_bdf = 0;
  type1_flag = 0;

  /*
   * Read four bytes of config space starting from Primary Bus num
   * register and extract the Secondary and Subordinate Bus numbers
   * and the segment.
   */
  val_pcie_read_cfg(bdf, TYPE1_PBN, &reg_value);
  sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);
  sub_bus = ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK);
  seg = (PCIE_EXTRACT_BDF_SEG(bdf));

  /*
   * Search for a pcie Function whose bus number is within the range of
   * target bridge's bus numbers downstream, from the value of Secondary
   * Bus number to the Subordinate Bus number, inclusive.
   *
   */
  index = 0;
  while (index < g_pcie_bdf_table->num_entries)
  {
      if (((PCIE_EXTRACT_BDF_BUS(g_pcie_bdf_table->device[index].bdf)) >= sec_bus) &&
          ((PCIE_EXTRACT_BDF_BUS(g_pcie_bdf_table->device[index].bdf)) <= sub_bus) &&
          ((PCIE_EXTRACT_BDF_SEG(g_pcie_bdf_table->device[index].bdf)) == seg))
      {
          *dsf_bdf = g_pcie_bdf_table->device[index].bdf;

          /* Return the bdf of first found type 0 function */
          if (val_pcie_function_header_type(*dsf_bdf) == TYPE0_HEADER)
              return 0;
          else if (!type1_flag)
          {
              type1_flag++;
              type1_bdf = *dsf_bdf;
          }
      }

      index++;
  }

  /* Return the bdf of first found type 1 function */
  if (type1_flag)
  {
      *dsf_bdf = type1_bdf;
      return 0;
  }

  /* Return failure */
  return 1;

}

/**
  @brief  Returns BDF of the upstream Root Port of a pcie device function.

  @param  bdf       - Function's Segment/Bus/Dev/Func in PCIE_CREATE_BDF format
  @param  usrp_bdf  - Upstream Rootport bdf in PCIE_CREATE_BDF format
  @return 0 for success, 1 for failure.
**/
uint32_t
val_pcie_get_rootport(uint32_t bdf, uint32_t *rp_bdf)
{

  uint32_t index;
  uint32_t seg_num;
  uint32_t sec_bus;
  uint32_t sub_bus;
  uint32_t reg_value;
  uint32_t dp_type;

  index = 0;

  dp_type = val_pcie_device_port_type(bdf);

  val_print(AVS_PRINT_DEBUG, " DP type 0x%03x ", dp_type);

  /* If the device is RP or iEP_RP, set its rootport value to same */
  if ((dp_type == RP) || (dp_type == iEP_RP))
  {
      *rp_bdf = bdf;
      return 0;
  }

  /* If the device is RCiEP and RCEC, set RP as 0xff */
  if ((dp_type == RCiEP) || (dp_type == RCEC))
  {
      *rp_bdf = 0xffffffff;
      return 1;
  }

  while (index < g_pcie_bdf_table->num_entries)
  {
      *rp_bdf = g_pcie_bdf_table->device[index++].bdf;

      /*
       * Extract Secondary and Subordinate Bus numbers of the
       * upstream Root port and check if the input function's
       * bus number falls within that range.
       */
      val_pcie_read_cfg(*rp_bdf, TYPE1_PBN, &reg_value);
      seg_num = PCIE_EXTRACT_BDF_SEG(*rp_bdf);
      sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);
      sub_bus = ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK);
      dp_type = val_pcie_device_port_type(*rp_bdf);

      if (((dp_type == RP) || (dp_type == iEP_RP)) &&
          (sec_bus <= PCIE_EXTRACT_BDF_BUS(bdf)) &&
          (sub_bus >= PCIE_EXTRACT_BDF_BUS(bdf)) &&
          (seg_num == PCIE_EXTRACT_BDF_SEG(bdf)))
          return 0;
  }

  /* Return failure */
  val_print(AVS_PRINT_ERR, "\n       PCIe Hierarchy fail: RP of bdf 0x%x not found", bdf);
  *rp_bdf = 0;
  return 1;

}

uint8_t
val_pcie_parent_is_rootport(uint32_t dsf_bdf, uint32_t *rp_bdf)
{

  uint8_t dsf_bus;
  uint32_t bdf;
  uint32_t dp_type;
  uint32_t tbl_index;
  uint32_t reg_value;
  pcie_device_bdf_table *bdf_tbl_ptr;

  tbl_index = 0;
  dsf_bus = PCIE_EXTRACT_BDF_BUS(dsf_bdf);
  bdf_tbl_ptr = val_pcie_bdf_table_ptr();

  while (tbl_index < bdf_tbl_ptr->num_entries)
  {
      bdf = bdf_tbl_ptr->device[tbl_index++].bdf;
      dp_type = val_pcie_device_port_type(bdf);

      /* Check if this table entry is a Root Port */
      if ((dp_type == RP) || (dp_type == iEP_RP))
      {
         /* Check if device is a direct child of this root port */
          val_pcie_read_cfg(bdf, TYPE1_PBN, &reg_value);
          if ((dsf_bus == ((reg_value >> SECBN_SHIFT) & SECBN_MASK)) &&
              (dsf_bus <= ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK)))
          {
              *rp_bdf = bdf;
              return 0;
          }
      }
  }

  return 1;
}

/**
  @brief  Check if BDF is PCIe Host Bridge.

  @param  bdf - Function's Segment/Bus/Dev/Func in PCIE_CREATE_BDF format
  @return 0 If not a Host Bridge, 1 If it's a Host Bridge.
**/
uint8_t
val_pcie_is_host_bridge(uint32_t bdf)
{
  uint32_t  reg_value;

  val_pcie_read_cfg(bdf, TYPE01_RIDR, &reg_value);
  if ((HB_BASE_CLASS == ((reg_value >> CC_BASE_SHIFT) & CC_BASE_MASK)) &&
      (HB_SUB_CLASS == ((reg_value >> CC_SUB_SHIFT) & CC_SUB_MASK)))
    return 1;

  return 0;
}

/**
  @brief  Returns whether a PCIe Function has an Address Translation Cache

  @param  bdf        - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return Returns 0  - if Function doesn't have Addr Translation Cache else 1.
**/
uint32_t
val_pcie_is_cache_present(uint32_t bdf)
{
  return pal_pcie_is_cache_present(PCIE_EXTRACT_BDF_SEG (bdf),
                                   PCIE_EXTRACT_BDF_BUS (bdf),
                                   PCIE_EXTRACT_BDF_DEV (bdf),
                                   PCIE_EXTRACT_BDF_FUNC (bdf));
}

/**
  @brief  Returns data link layer link active status of the given PCIe function

  @param  bdf        - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return Returns PCIE_DLL_LINK_STATUS_NOT_ACTIVE    - If the Link status is not active.
                  PCIE_DLL_LINK_STATUS_ACTIVE        - If the Link status is active.
                  PCIE_DLL_LINK_ACTIVE_NOT_SUPPORTED - If the Link status is not supported
**/
uint32_t
val_pcie_data_link_layer_status(uint32_t bdf)
{
   uint32_t pciecs_base;
   uint32_t data_link_report;
   uint32_t dll_status;

   /* Obtain the Data Link Layer Link Active Reporting Capable to check if the
    * link status can be polled.
    */
   val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
   val_pcie_read_cfg(bdf, pciecs_base + LCAPR_OFFSET, &data_link_report);
   data_link_report = (data_link_report & LCAPR_DLLLARC_MASK) >> LCAPR_DLLLARC_SHIFT;

   /* If  Data Link Layer Link Active Reporting is supported, the check the
    * status of Data Link Layer Link Active.
    */
   if (data_link_report)
   {
       val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
       val_pcie_read_cfg(bdf, pciecs_base + LCTRLR_OFFSET, &dll_status);
       dll_status = (dll_status & LSTAT_DLLLA_MASK) >> LSTAT_DLLLA_SHIFT;
       return dll_status;
   }

   return PCIE_DLL_LINK_ACTIVE_NOT_SUPPORTED;
}

/**
  @brief  Returns whether a PCIe Function has detected an Interrupt request

  @param  bdf        - Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return Returns 1 - if the Function has received an Interrupt Request or
                  0 - if it does not detect Interrupt request
**/
uint32_t
val_pcie_check_interrupt_status(uint32_t bdf)
{

  uint32_t reg_value;

  val_pcie_read_cfg(bdf, TYPE01_CR, &reg_value);
  reg_value = (reg_value >> SR_IS_SHIFT) & SR_IS_MASK;

  return reg_value;
}

uint32_t
val_pcie_get_max_pasid_width(uint32_t bdf, uint32_t *max_pasid_width)
{
  uint32_t status;
  uint32_t pciecs_base;

  status = val_pcie_find_capability(bdf, PCIE_ECAP, ECID_PASID, &pciecs_base);
  if (status)
      return status;

  val_pcie_read_cfg(bdf, pciecs_base + PASID_CAPABILITY_OFFSET, max_pasid_width);
  *max_pasid_width = (*max_pasid_width & MAX_PASID_MASK) >> MAX_PASID_SHIFT;

  return 0;
}

/**
  @brief  Returns the ECAM address of the input PCIe function

  @param  bdf   - Segment/Bus/Dev/Func in PCIE_CREATE_BDF format
  @return 0 - success, 1 - failure
**/
uint32_t val_pcie_get_ecam_index(uint32_t bdf, uint32_t *ecam_index)
{

  uint8_t sec_bus;
  uint8_t sub_bus;
  uint16_t seg_num;
  uint32_t reg_value;
  uint32_t bus_num;
  uint32_t index = 0;

  seg_num = PCIE_EXTRACT_BDF_SEG(bdf);
  bus_num = PCIE_EXTRACT_BDF_BUS(bdf);

  while (index < val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0))
  {
      if (seg_num == val_pcie_get_info(PCIE_INFO_SEGMENT, index) &&
         (bus_num >= val_pcie_get_info(PCIE_INFO_START_BUS, index)) &&
         (bus_num <= val_pcie_get_info(PCIE_INFO_END_BUS, index)))
      {
          if (val_pcie_function_header_type(bdf) == TYPE0_HEADER)
          {
              /* Return ecam index if Type0 Header */
              *ecam_index = index;
              return 0;
          }
          else
          {
              /* Check for Secondary/Subordinate bus if Type1 Header */
              val_pcie_read_cfg(bdf, TYPE1_PBN, &reg_value);
              sec_bus = ((reg_value >> SECBN_SHIFT) & SECBN_MASK);
              sub_bus = ((reg_value >> SUBBN_SHIFT) & SUBBN_MASK);

              if ((sec_bus >= val_pcie_get_info(PCIE_INFO_START_BUS, index)) &&
                  (sub_bus <= val_pcie_get_info(PCIE_INFO_END_BUS, index)))
              {
                    *ecam_index = index;
                    return 0;
              }
          }
      }

      index++;
  }

  return 1;
}

/**
  @brief  Returns the memory offset that can be
          accessed from the BAR base and is within
          BAR limit value

  @param  type
  @return memory offset

**/
uint32_t val_pcie_mem_get_offset(uint32_t type)
{
  return pal_pcie_mem_get_offset(type);
}

/**
  @brief  Checks if link Capabilities is supported
  @param  bdf    -  Segment/Bus/Dev/Func in the format of PCIE_CREATE_BDF
  @return 0 if link capability is not supported else 1.
**/
uint32_t
val_pcie_link_cap_support(uint32_t bdf)
{
  uint32_t pciecs_base;
  uint32_t reg_value = 0xFFFFFFFF;

  val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
  val_pcie_read_cfg(bdf, pciecs_base + LCAPR_OFFSET, &reg_value);

  if (reg_value != 0) {
     val_print(AVS_PRINT_ERR, "\n       Link Capabilities reg check failed", 0);
     return 1;
  }

  reg_value = 0xFFFFFFFF;
  val_pcie_read_cfg(bdf, pciecs_base + LCTRLR_OFFSET, &reg_value);
  if (reg_value != 0) {
     val_print(AVS_PRINT_ERR, "\n       Link Capabilities control and status check failed", 0);
     return 1;
  }

  reg_value = 0xFFFFFFFF;
  val_pcie_read_cfg(bdf, pciecs_base + LCAP2R_OFFSET, &reg_value);
  if (reg_value != 0) {
     val_print(AVS_PRINT_ERR, "\n       Link Capabilities 2 reg check failed", 0);
     return 1;
  }

  reg_value = 0xFFFFFFFF;
  val_pcie_read_cfg(bdf, pciecs_base + LCTL2R_OFFSET, &reg_value);
  if (reg_value != 0) {
     val_print(AVS_PRINT_ERR, "\n       Link Capabilities 2 control and status check failed", 0);
     return 1;
  }

  return 0;
}
