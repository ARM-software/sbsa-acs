/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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
#include "include/sbsa_avs_mpam.h"
#include "include/sbsa_avs_mpam_reg.h"

static MPAM_INFO_TABLE *g_mpam_info_table;
static SRAT_INFO_TABLE *g_srat_info_table;
static HMAT_INFO_TABLE *g_hmat_info_table;

/**
  @brief   This API executes all the MPAM tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_mpam_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_mpam_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status = AVS_STATUS_FAIL, i;
  uint32_t skip_module;
  uint32_t msc_node_cnt;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == AVS_MPAM_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "      USER Override - Skipping all MPAM tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in the current module with user override*/
  skip_module = val_check_skip_module(AVS_MPAM_TEST_NUM_BASE);
  if (skip_module) {
      val_print(AVS_PRINT_TEST, "\n USER Override - Skipping all MPAM tests \n", 0);
      return AVS_STATUS_SKIP;
  }

  /* check if PE supports MPAM extension, else skip all MPAM tests */
  if (val_pe_feat_check(PE_FEAT_MPAM)) {
      val_print(AVS_PRINT_TEST,
                "\n       PE MPAM extension unimplemented. Skipping all MPAM tests\n", 0);
      return AVS_STATUS_SKIP;
  }

  val_print_test_start("MPAM");
  g_curr_module = 1 << MPAM_MODULE;

  /* run tests which don't check MPAM MSCs */
  status = mpam001_entry(num_pe);

  msc_node_cnt = val_mpam_get_msc_count();
  if (msc_node_cnt == 0) {
      val_print(AVS_PRINT_TEST,
                "\n       MPAM MSCs not found. Skipping remaining MPAM tests\n", 0);
      return AVS_STATUS_SKIP;
  }

  status |= mpam002_entry(num_pe);
  status |= mpam003_entry(num_pe);
  status |= mpam004_entry(num_pe);
  status |= mpam005_entry(num_pe);
  status |= mpam006_entry(num_pe);
  val_print_test_end(status, "MPAM");

  return status;
}

/**
  @brief   This API provides a 'C' interface to call MPAM system register reads
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   reg_id  - the system register index for which data is returned.
  @return  the value read from the system register.
**/
uint64_t
val_mpam_reg_read(MPAM_SYS_REGS reg_id)
{
  switch (reg_id) {
  case MPAMIDR_EL1:
      return AA64ReadMpamidr();
  case MPAM2_EL2:
      return AA64ReadMpam2();
  case MPAM1_EL1:
      return AA64ReadMpam1();
  default:
      val_report_status(val_pe_get_index_mpid(val_pe_get_mpid()),
                        RESULT_FAIL(g_sbsa_level, 0, STATUS_SYS_REG_ACCESS_FAIL), NULL);
  }

  return 0;
}

/**
  @brief   This API provides a 'C' interface to call MPAM system register writes
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   reg_id  - the system register index for which data is written
  @param   write_data - the 64-bit data to write to the system register
  @return  None
**/
void
val_mpam_reg_write(MPAM_SYS_REGS reg_id, uint64_t write_data)
{
  switch (reg_id) {
  case MPAM2_EL2:
      AA64WriteMpam2(write_data);
      break;
  case MPAM1_EL1:
      AA64WriteMpam1(write_data);
      break;
  default:
      val_report_status(val_pe_get_index_mpid(val_pe_get_mpid()),
                        RESULT_FAIL(g_sbsa_level, 0, STATUS_SYS_REG_ACCESS_FAIL), NULL);
  }

  return;
}

/**
  @brief   This API returns requested MSC or resource info.

  @param   type       - the type of information being requested.
  @param   msc_index  - index of the MSC node in the MPAM info table.
  @param   rsrc_index - index of the resource node in the MPAM MSC node if resource
                        related info is requested, set this parameter 0 otherwise.

  @return  requested data if found, otherwise MPAM_INVALID_INFO.
**/
uint64_t
val_mpam_get_info(MPAM_INFO_e type, uint32_t msc_index, uint32_t rsrc_index)
{
  uint32_t i = 0;
  MPAM_MSC_NODE *msc_entry;

  if (g_mpam_info_table == NULL) {
      val_print(AVS_PRINT_WARN, "\n   MPAM info table not found", 0);
      return MPAM_INVALID_INFO;
  }

  if (msc_index > g_mpam_info_table->msc_count) {
      val_print(AVS_PRINT_ERR, "Invalid MSC index = 0x%lx ", msc_index);
      return 0;
  }

  /* Walk the MPAM info table and return requested info */
  msc_entry = &g_mpam_info_table->msc_node[0];
  for (i = 0; i < g_mpam_info_table->msc_count; i++, msc_entry = MPAM_NEXT_MSC(msc_entry)) {
      if (msc_index == i) {
          if (rsrc_index > msc_entry->rsrc_count - 1) {
              val_print(AVS_PRINT_ERR,
                      "\n   Invalid MSC resource index = 0x%lx for", rsrc_index);
              val_print(AVS_PRINT_ERR, "MSC index = 0x%lx ", msc_index);
              return MPAM_INVALID_INFO;
          }
          switch (type) {
          case MPAM_MSC_RSRC_COUNT:
              return msc_entry->rsrc_count;
          case MPAM_MSC_RSRC_RIS:
              return msc_entry->rsrc_node[rsrc_index].ris_index;
          case MPAM_MSC_RSRC_TYPE:
              return msc_entry->rsrc_node[rsrc_index].locator_type;
          case MPAM_MSC_RSRC_DESC1:
              return msc_entry->rsrc_node[rsrc_index].descriptor1;
          case MPAM_MSC_BASE_ADDR:
              return msc_entry->msc_base_addr;
          case MPAM_MSC_ADDR_LEN:
              return msc_entry->msc_addr_len;
          case MPAM_MSC_NRDY:
              return msc_entry->max_nrdy;
          default:
              val_print(AVS_PRINT_ERR,
                       "\n   This MPAM info option for type %d is not supported", type);
              return MPAM_INVALID_INFO;
          }
      }
  }
  return MPAM_INVALID_INFO;
}

/**
  @brief   This API returns requested Base address or Address length or num of mem ranges info.
           1. Caller       - Test Suite
           2. Prerequisite - val_srat_create_info_table
  @param   type - the type of information being requested.
  @param   data - proximity domain or uid.

  @return  requested data if found, otherwise SRAT_INVALID_INFO.
**/
uint64_t
val_srat_get_info(SRAT_INFO_e type, uint64_t data)
{
  uint32_t i = 0;

  if (g_srat_info_table == NULL) {
      val_print(AVS_PRINT_WARN, "\n   SRAT info table not found", 0);
      return SRAT_INVALID_INFO;
  }

  switch (type) {
  case SRAT_MEM_NUM_MEM_RANGE:
      return g_srat_info_table->num_of_mem_ranges;
  case SRAT_MEM_BASE_ADDR:
      for (i = 0; i < g_srat_info_table->num_of_srat_entries; i++) {
          if (g_srat_info_table->srat_info[i].node_type == SRAT_NODE_MEM_AFF) {
              if (data == g_srat_info_table->srat_info[i].node_data.mem_aff.prox_domain) {
                  return g_srat_info_table->srat_info[i].node_data.mem_aff.addr_base;
              }
          }
      }
      break;
  case SRAT_MEM_ADDR_LEN:
      for (i = 0; i < g_srat_info_table->num_of_srat_entries; i++) {
          if (g_srat_info_table->srat_info[i].node_type == SRAT_NODE_MEM_AFF) {
              if (data == g_srat_info_table->srat_info[i].node_data.mem_aff.prox_domain) {
                  return g_srat_info_table->srat_info[i].node_data.mem_aff.addr_len;
              }
          }
      }
      break;
  case SRAT_GICC_PROX_DOMAIN:
      for (i = 0; i < g_srat_info_table->num_of_srat_entries; i++) {
          if (g_srat_info_table->srat_info[i].node_type == SRAT_NODE_GICC_AFF) {
              if (data == g_srat_info_table->srat_info[i].node_data.gicc_aff.proc_uid) {
                  return g_srat_info_table->srat_info[i].node_data.gicc_aff.prox_domain;
              }
          }
      }
      break;
  case SRAT_GICC_PROC_UID:
      for (i = 0; i < g_srat_info_table->num_of_srat_entries; i++) {
          if (g_srat_info_table->srat_info[i].node_type == SRAT_NODE_GICC_AFF) {
              if (data == g_srat_info_table->srat_info[i].node_data.gicc_aff.prox_domain) {
                  return g_srat_info_table->srat_info[i].node_data.gicc_aff.proc_uid;
              }
          }
      }
      return SRAT_INVALID_INFO;
      break;
  case SRAT_GICC_REMOTE_PROX_DOMAIN:
      for (i = 0; i < g_srat_info_table->num_of_srat_entries; i++) {
          if (g_srat_info_table->srat_info[i].node_type == SRAT_NODE_GICC_AFF) {
              if (g_srat_info_table->srat_info[i].node_data.gicc_aff.prox_domain != data) {
                  return g_srat_info_table->srat_info[i].node_data.gicc_aff.prox_domain;
              }
          }
      }
      return SRAT_INVALID_INFO;
      break;
  default:
      val_print(AVS_PRINT_ERR,
                    "\n    This SRAT info option for type %d is not supported", type);
      break;
  }
  return SRAT_INVALID_INFO;
}

/**
  @brief   This API returns proximity domain mapped to the memory range.

  @param   none
  @return  proximity domain
**/
uint64_t
val_srat_get_prox_domain(uint64_t mem_range_index)
{
  uint32_t  i = 0;

  if (g_srat_info_table == NULL) {
      val_print(AVS_PRINT_WARN, "\n   SRAT info table not found", 0);
      return SRAT_INVALID_INFO;
  }

  if (mem_range_index > g_srat_info_table->num_of_mem_ranges) {
      val_print(AVS_PRINT_WARN, "\n   Invalid index", 0);
      return SRAT_INVALID_INFO;
  }

  for (i = 0; i < g_srat_info_table->num_of_mem_ranges; i++) {
      if (g_srat_info_table->srat_info[i].node_type == SRAT_NODE_MEM_AFF) {
          if (mem_range_index == 0)
              return g_srat_info_table->srat_info[i].node_data.mem_aff.prox_domain;
          else
              mem_range_index--;
      }
  }
  return SRAT_INVALID_INFO;
}

/**
 * @brief   This API returns numbers of MPAM MSC nodes in the system.
 *
 * @param   None
 * @return  number of MPAM MSC nodes
 */
uint32_t
val_mpam_get_msc_count(void)
{
    if (g_mpam_info_table == NULL) {
        val_print(AVS_PRINT_WARN, "\n   MPAM info table not found", 0);
        return 0;
    }
    else
        return g_mpam_info_table->msc_count;
}

/**
  @brief   This API returns MSC MAPAM version
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  msc mpam version.
**/
uint32_t
val_mpam_msc_get_version(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    return BITFIELD_READ(AIDR_VERSION, val_mmio_read(base + REG_MPAMF_AIDR));
}

/**
  @brief   This API checks whether resource monitoring is supported by the MPAM MSC.
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  1 if supported 0 otherwise.
**/
uint32_t
val_mpam_msc_supports_mon(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    return BITFIELD_READ(IDR_HAS_MSMON, val_mmio_read64(base + REG_MPAMF_IDR));
}

/**
  @brief   This API checks whether MSC has cache portion partitioning.
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  1 if supported 0 otherwise.
**/
uint32_t
val_mpam_supports_cpor(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    return BITFIELD_READ(IDR_HAS_CPOR_PART, val_mmio_read64(base + REG_MPAMF_IDR));
}

/**
  @brief   This API checks whether resource instance selection (RIS) implemented
            for the MSC.
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  1 if supported 0 otherwise.
**/
uint32_t
val_mpam_msc_supports_ris(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    return BITFIELD_READ(IDR_HAS_RIS, val_mmio_read64(base + REG_MPAMF_IDR));
}

/**
  @brief   This API checks if the MSC supports Memory Bandwidth Usage Monitor (MBWU)
           Prerequisite - If MSC supports RIS, Resource instance should be
                          selected using val_mpam_memory_configure_ris_sel
                          prior calling this API.
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  1 if supported 0 otherwise.
**/
uint32_t
val_mpam_msc_supports_mbwumon(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    if (val_mpam_msc_supports_mon(msc_index))
        return BITFIELD_READ(MSMON_IDR_MSMON_MBWU, val_mmio_read(base + REG_MPAMF_MSMON_IDR));
    else
        return 0;
}

/**
  @brief   This API returns max bandwidth supported by a memory interface with mbwu attached
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  1 if supported 0 otherwise.
**/
uint64_t
val_mpam_msc_get_mscbw(uint32_t msc_index, uint32_t rsrc_index)
{
    uint64_t prox_domain;
    uint32_t i = 0;

    prox_domain = val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index);

    if (g_hmat_info_table == NULL) {
        val_print(AVS_PRINT_WARN, "\n   HMAT info table not found", 0);
        return HMAT_INVALID_INFO;
    }

    for (i = 0; i < g_hmat_info_table->num_of_mem_prox_domain ; i++) {
        if (g_hmat_info_table->bw_info[i].mem_prox_domain == prox_domain) {
            return (g_hmat_info_table->bw_info[i].write_bw +
                                   g_hmat_info_table->bw_info[i].read_bw);
        }
    }
    val_print(AVS_PRINT_WARN, "\n       Invalid Proximity domain 0x%lx", prox_domain);
    return HMAT_INVALID_INFO;
}

/**
  @brief   This API checks if the MBWU supports 44-bit ot 64-bit counter
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  1 if supported 0 otherwise.
**/
uint32_t
val_mpam_mbwu_supports_long(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    return BITFIELD_READ(MBWUMON_IDR_HAS_LONG, val_mmio_read(base + REG_MPAMF_MBWUMON_IDR));
}

/**
  @brief   This API checks if the MBWU supports 64 bit counter
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  1 if supported 0 otherwise.
**/
uint32_t
val_mpam_mbwu_supports_lwd(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    return BITFIELD_READ(MBWUMON_IDR_LWD, val_mmio_read(base + REG_MPAMF_MBWUMON_IDR));
}

/**
  @brief   This API checks if the MSC supports Cache Usage Monitor (CSU)
           Prerequisite - If MSC supports RIS, Resource instance should be
                          selected using val_mpam_memory_configure_ris_sel
                          prior calling this API.
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  1 if supported 0 otherwise.
**/
uint32_t
val_mpam_supports_csumon(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    if (val_mpam_msc_supports_mon(msc_index))
        return BITFIELD_READ(MSMON_IDR_MSMON_CSU, val_mmio_read(base + REG_MPAMF_MSMON_IDR));
    else
        return 0;
}

/**
  @brief   This API checks if the MSC supports Cache Usage Monitor (CSU)
           Prerequisite - If MSC supports RIS, Resource instance should be
                          selected using val_mpam_memory_configure_ris_sel
                          prior calling this API.
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  1 if supported 0 otherwise.
**/
uint32_t
val_mpam_get_csumon_count(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    return BITFIELD_READ(CSUMON_IDR_NUM_MON, val_mmio_read(base + REG_MPAMF_CSUMON_IDR));
}

/**
  @brief   This API configures MPAM MBWU selection registers.
           Prerequisite - val_mpam_msc_supports_ris
           This API should only be used for MSC supporting RIS.

  @param   msc_index  - index of the MSC node in the MPAM info table.
  @param   rsrc_index - index of the resource node in the MPAM MSC node.

  @return  null
**/
void
val_mpam_memory_configure_ris_sel(uint32_t msc_index, uint32_t rsrc_index)
{
    addr_t base;
    uint32_t data;
    uint8_t ris_index;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    ris_index = val_mpam_get_info(MPAM_MSC_RSRC_RIS, msc_index, rsrc_index);

    /*configure MSMON_CFG_MON_SEL.RIS field and write MSMON_CFG_MON_SEL.MON_SEL
       field to be 0 */
    data = BITFIELD_SET(MON_SEL_RIS, ris_index);
    val_mmio_write(base + REG_MSMON_CFG_MON_SEL, data);

    /* configure MPAMCFG_PART_SEL.RIS field and write MPAMCFG_PART_SEL.
       PARTID_SEL field to DEFAULT PARTID*/
    data = BITFIELD_SET(PART_SEL_RIS, ris_index)
                           | BITFIELD_SET(PART_SEL_PARTID_SEL, DEFAULT_PARTID);
    val_mmio_write(base + REG_MPAMCFG_PART_SEL, data);
}

/**
  @brief   This API configures the bandwidth usage monitor.
           Prerequisite - If MSC supports RIS, Resource instance should be
                          selected using val_mpam_memory_configure_ris_sel
                          prior calling this API.
                        - MSC should support MBWU monitoring, can be checked
                          using val_mpam_msc_supports_mbwumon API.

  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  None
**/
void
val_mpam_memory_configure_mbwumon(uint32_t msc_index)
{
    uint32_t data = 0;
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);

    /* select monitor instance zero by writing zero to MSMON_CFG_MON_SEL.MON_SEL */
    data = val_mmio_read(base + REG_MSMON_CFG_MON_SEL);
    /* retaining other configured fields e.g, RIS index if supported */
    data = BITFIELD_WRITE(data, MON_SEL_MON_SEL, 0);
    val_mmio_write(base + REG_MSMON_CFG_MON_SEL, data);

    /* disable monitor instance before configuration */
    val_mpam_memory_mbwumon_disable(msc_index);

    /* configure monitor ctrl reg for default partid and default pmg */
    data = BITFIELD_SET(MBWU_CTL_MATCH_PARTID, 1) | BITFIELD_SET(MBWU_CTL_MATCH_PMG, 1);
    val_mmio_write(base + REG_MSMON_CFG_MBWU_CTL, data);

    /* configure monitor filter reg for default partid and default pmg */
    data = BITFIELD_SET(MBWU_FLT_PARTID, DEFAULT_PARTID) | BITFIELD_SET(MBWU_FLT_PMG, DEFAULT_PMG);
    val_mmio_write(base + REG_MSMON_CFG_MBWU_FLT, data);

    /* reset the MBWU monitor count */
    val_mpam_memory_mbwumon_reset(msc_index);
}

/**
  @brief   This API enables the bandwidth usage monitor.
           Prerequisite - If MSC supports RIS, Resource instance should be
                          selected using val_mpam_memory_configure_ris_sel
                          prior calling this API.
                        - MSC should support MBWU monitoring, can be checked
                          using val_mpam_msc_supports_mbwumon API..

  @param   msc_index - index of the MSC node in the MPAM info table.
**/
void
val_mpam_memory_mbwumon_enable(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    /* enable the monitor instance to collect information according to the configuration */
    val_mmio_write(base + REG_MSMON_CFG_MBWU_CTL, BITFIELD_SET(MBWU_CTL_EN, 1));
}

/**
  @brief   This API disables the bandwidth usage monitor.
           Prerequisite - If MSC supports RIS, Resource instance should be
                          selected using val_mpam_memory_configure_ris_sel
                          prior calling this API.
                        - MSC should support MBWU monitoring, can be checked
                          using val_mpam_msc_supports_mbwumon API.

  @param   msc_index - index of the MSC node in the MPAM info table.
**/
void
val_mpam_memory_mbwumon_disable(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    /* disable the monitor */
    val_mmio_write(base + REG_MSMON_CFG_MBWU_CTL, BITFIELD_SET(MBWU_CTL_EN, 0));
}

/**
  @brief   This API reads the MBWU montior counter value.
           Prerequisite - val_mpam_memory_configure_mbwumon,
           This API can be called only after configuring MBWU monitor.

  @param   msc_index  - MPAM feature page index for this MSC.
  @return  MPAM_MON_NOT_READY if monitor has Not Ready status,
           else counter value.
**/
uint64_t
val_mpam_memory_mbwumon_read_count(uint32_t msc_index)
{
    addr_t base;
    uint64_t count = MPAM_MON_NOT_READY;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);

    /*if MSMON_MBWU_L is implemented*/
    if (BITFIELD_READ(MBWUMON_IDR_LWD, val_mmio_read64(base + REG_MPAMF_MBWUMON_IDR))) {
        if (BITFIELD_READ(MBWUMON_IDR_HAS_LONG, val_mmio_read64(base + REG_MPAMF_MBWUMON_IDR))) {
            // (63 bits)
            if (BITFIELD_READ(MSMON_MBWU_L_NRDY, val_mmio_read64(base + REG_MSMON_MBWU_L)) == 0)
                count = BITFIELD_READ(MSMON_MBWU_L_63BIT_VALUE,
                                      val_mmio_read64(base + REG_MSMON_MBWU_L));
        }
        else {
            // (44 bits)
            if (BITFIELD_READ(MSMON_MBWU_L_NRDY, val_mmio_read64(base + REG_MSMON_MBWU_L)) == 0)
                count = BITFIELD_READ(MSMON_MBWU_L_44BIT_VALUE,
                                      val_mmio_read64(base + REG_MSMON_MBWU_L));
        }
    }
    else {
        // (31 bits)
        if (BITFIELD_READ(MSMON_MBWU_NRDY, val_mmio_read(base + REG_MSMON_MBWU)) == 0) {
            count = BITFIELD_READ(MSMON_MBWU_VALUE,
                                  val_mmio_read(base + REG_MSMON_MBWU));
            /* shift the count if scaling is enabled */
            count = count << BITFIELD_READ(MBWUMON_IDR_SCALE,
                                  val_mmio_read(base + REG_MPAMF_MBWUMON_IDR));
        }
    }
    return(count);
}

/**
  @brief   This API resets the MBWU montior counter value.
           Prerequisite - val_mpam_memory_configure_mbwumon,
           This API can be called only after configuring MBWU monitor.

  @param   msc_index  - MPAM feature page index for this MSC.
  @return  None
**/
void
val_mpam_memory_mbwumon_reset(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);

    /*if MSMON_MBWU_L is implemented*/
    if (BITFIELD_READ(MBWUMON_IDR_LWD, val_mmio_read64(base + REG_MPAMF_MBWUMON_IDR)))
        val_mmio_write64(base + REG_MSMON_MBWU_L, 0);
    else
       val_mmio_write(base + REG_MSMON_MBWU, 0);
}


/**
  @brief   Creates a buffer with length equal to size within the
           address range (mem_base, mem_base + mem_size)

  @param   mem_base    - Base address of the memory range
  @param   size        - Buffer size to be created

  @return  Buffer address if SUCCESSFUL, else NULL
**/
void *
val_mem_alloc_at_address(uint64_t mem_base, uint64_t size)
{
  return pal_mem_alloc_at_address(mem_base, size);
}

/**
  @brief   Frees the allocated buffer with length equal to size within the
           address range (mem_base, mem_base + mem_size)

  @param   mem_base    - Base address of the memory range
  @param   size        - Buffer size to be created

  @return  Buffer address if SUCCESSFUL, else NULL
**/
void
val_mem_free_at_address(uint64_t mem_base, uint64_t size)
{
  return pal_mem_free_at_address(mem_base, size);
}
/**
  @brief   Creates a buffer with length equal to size within the
           address range (mem_base, mem_base + mem_size)

  @param   mem_base    - Base address of the memory range
  @param   mem_size    - Size of the memory range of interest
  @param   size        - Buffer size to be created

  @return  Buffer address if SUCCESSFUL, else NULL
**/
uint64_t val_mpam_memory_get_size(uint32_t msc_index, uint32_t rsrc_index)
{
    uint64_t prox_domain;

    prox_domain = val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index);
    return val_srat_get_info(SRAT_MEM_ADDR_LEN, prox_domain);
}

/**
  @brief   Creates a buffer with length equal to size within the
           address range (mem_base, mem_base + mem_size)

  @param   mem_base    - Base address of the memory range
  @param   mem_size    - Size of the memory range of interest
  @param   size        - Buffer size to be created

  @return  Buffer address if SUCCESSFUL, else NULL
**/
uint64_t val_mpam_memory_get_base(uint32_t msc_index, uint32_t rsrc_index)
{
    uint64_t prox_domain;

    prox_domain = val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index);
    return val_srat_get_info(SRAT_MEM_BASE_ADDR, prox_domain);
}

/**
  @brief   This API will call PAL layer to fill in the MPAM table information
           into the g_mpam_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   mpam_info_table  pre-allocated memory pointer for cache info.
  @return  Error if Input parameter is NULL
**/
void
val_mpam_create_info_table(uint64_t *mpam_info_table)
{
  if (mpam_info_table == NULL) {
    val_print(AVS_PRINT_ERR, "\n Pre-allocated memory pointer is NULL \n", 0);
    return;
  }

  g_mpam_info_table = (MPAM_INFO_TABLE *)mpam_info_table;
#ifndef TARGET_LINUX
  pal_mpam_create_info_table(g_mpam_info_table);

  val_print(AVS_PRINT_TEST,
                " MPAM INFO: Number of MSC nodes       :    %d \n", g_mpam_info_table->msc_count);
#endif
}

/**
  @brief   This API frees the memory allocated for MPAM info table.
  @param   None
  @return  None
**/
void
val_mpam_free_info_table(void)
{
  pal_mem_free((void *)g_mpam_info_table);
}

/**
  @brief   This API will call PAL layer to fill in the HMAT table information
           into the g_hmat_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   hmat_info_table  pre-allocated memory pointer for cache info.
  @return  Error if Input parameter is NULL
**/
void
val_hmat_create_info_table(uint64_t *hmat_info_table)
{
  if (hmat_info_table == NULL) {
    val_print(AVS_PRINT_ERR, "\n Pre-allocated memory pointer is NULL \n", 0);
    return;
  }
#ifndef TARGET_LINUX
  g_hmat_info_table = (HMAT_INFO_TABLE *)hmat_info_table;

  pal_hmat_create_info_table(g_hmat_info_table);

  if (g_hmat_info_table->num_of_mem_prox_domain != 0)
      val_print(AVS_PRINT_TEST,
                " HMAT INFO: Number of Prox domains    :    %d \n",
                                    g_hmat_info_table->num_of_mem_prox_domain);
#endif
}

/**
  @brief   This API frees the memory allocated for HMAT info table.
  @param   None
  @return  None
**/
void
val_hmat_free_info_table(void)
{
    pal_mem_free((void *)g_hmat_info_table);
}

/**
  @brief   This API will call PAL layer to fill in the SRAT table information
           into the g_hmat_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   srat_info_table  pre-allocated memory pointer for cache info.
  @return  Error if Input parameter is NULL
**/
void
val_srat_create_info_table(uint64_t *srat_info_table)
{
  if (srat_info_table == NULL) {
    val_print(AVS_PRINT_ERR, "\n Pre-allocated memory pointer is NULL \n", 0);
    return;
  }
#ifndef TARGET_LINUX
  g_srat_info_table = (SRAT_INFO_TABLE *)srat_info_table;

  pal_srat_create_info_table(g_srat_info_table);

  if (g_srat_info_table->num_of_mem_ranges != 0)
      val_print(AVS_PRINT_TEST,
                " SRAT INFO: Number of Memory Ranges   :    %d \n",
                                    g_srat_info_table->num_of_mem_ranges);
#endif
}

/**
  @brief   This API frees the memory allocated for SRAT info table.
  @param   None
  @return  None
**/
void
val_srat_free_info_table(void)
{
    pal_mem_free((void *)g_srat_info_table);
}

/**
  @brief   This API gets maximum supported value of PMG
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  PMG value.
**/
uint32_t
val_mpam_get_max_pmg(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    return BITFIELD_READ(IDR_PMG_MAX, val_mmio_read64(base + REG_MPAMF_IDR));
}

/**
  @brief   This API gets Maximum supported value of PARTID
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  Partion ID value.
**/
uint32_t
val_mpam_get_max_partid(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    return BITFIELD_READ(IDR_PARTID_MAX, val_mmio_read64(base + REG_MPAMF_IDR));
}

/**
  @brief   This API Configures CPOR settings for given MSC
           Prerequisite - If MSC supports RIS, Resource instance should be
                          selected using val_mpam_memory_configure_ris_sel
                          prior calling this API.
                        - MSC should support CSU monitoring, can be checked
                          using val_mpam_supports_csumon API.
  @param   msc_index - index of the MSC node in the MPAM info table.
  @param   partid - PATRTID for CPOR configuration
  @param   cpbm_percentage - Percentage of cache to be partitioned
  @return  void.
**/
void
val_mpam_configure_cpor(uint32_t msc_index, uint16_t partid, uint32_t cpbm_percentage)
{
    addr_t base;
    uint16_t index;
    uint32_t unset_bitmask;
    uint32_t num_unset_bits;
    uint16_t num_cpbm_bits;
    uint32_t data;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);

    /* Get CPBM width */
    num_cpbm_bits = val_mpam_get_cpbm_width(msc_index);

    /* retaining other configured fields e.g, RIS index if supported */
    data = val_mmio_read(base + REG_MPAMCFG_PART_SEL);

    /* Select PARTID */
    data = BITFIELD_WRITE(data, PART_SEL_PARTID_SEL, partid);
    val_mmio_write(base + REG_MPAMCFG_PART_SEL, partid);

    /*
     * Configure CPBM register to have a 1 in cpbm_percentage
     * bits in the overall CPBM_WD bit positions
     */
    num_cpbm_bits = (num_cpbm_bits * cpbm_percentage) / 100 ;
    for (index = 0; index < num_cpbm_bits - 31; index += 32)
        val_mmio_write(base + REG_MPAMCFG_CPBM + index, CPOR_BITMAP_DEF_VAL);

    /* Unset bits from above step are set */
    num_unset_bits = num_cpbm_bits - index;
    unset_bitmask = (1 << num_unset_bits) - 1;
    if (unset_bitmask)
        val_mmio_write(base + REG_MPAMCFG_CPBM + index, unset_bitmask);

    /* Issue a DSB instruction */
    val_mem_issue_dsb();

    return;
}

/**
  @brief   This API gets CPBM width
  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  Number of bits in the cache portion partitioning bit map.
**/
uint32_t
val_mpam_get_cpbm_width(uint32_t msc_index)
{
    addr_t base;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    if (val_mpam_supports_cpor(msc_index))
        return BITFIELD_READ(CPOR_IDR_CPBM_WD, val_mmio_read(base + REG_MPAMF_CPOR_IDR));
    else
        return 0;
}

/**
  @brief   This API issues a DSB Memory barrier instruction
  @param   None
  @return  None
**/
void
val_mem_issue_dsb(void)
{
    AA64IssueDSB();
}

/**
  @brief   This API configures the cache storage usage monitor.
           Prerequisite - If MSC supports RIS, Resource instance should be
                          selected using val_mpam_memory_configure_ris_sel
                          prior calling this API.
                        - MSC should support CSU monitoring, can be checked
                          using val_mpam_supports_csumon API.

  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  None
**/
void
val_mpam_configure_csu_mon(uint32_t msc_index, uint16_t partid, uint8_t pmg, uint16_t mon_sel)
{
    addr_t base;
    uint32_t data;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);

    /* retaining other configured fields e.g, RIS index if supported */
    data = val_mmio_read(base + REG_MSMON_CFG_MON_SEL);
    /* Select the monitor instance */
    data = BITFIELD_WRITE(data, MON_SEL_MON_SEL, mon_sel);
    val_mmio_write(base + REG_MSMON_CFG_MON_SEL, data);

    /* Configure the CSU monitor filter register for input PARTID & PMG */
    data = BITFIELD_SET(CSU_FLT_PARTID, partid) | BITFIELD_SET(CSU_FLT_PMG, pmg);
    val_mmio_write(base + REG_MSMON_CFG_CSU_FLT, data);

    /*Disable the monitor */
    val_mpam_csumon_disable(msc_index);

    /* Configure the CSU monitor control register to match input PARTID & PMG */
    data = BITFIELD_SET(CSU_CTL_MATCH_PARTID, 1) | BITFIELD_SET(CSU_CTL_MATCH_PMG, 1);
    val_mmio_write(base + REG_MSMON_CFG_CSU_CTL, data);

    /* Issue a DSB instruction */
    val_mem_issue_dsb();

    return;
}

/**
  @brief   This API enables the Cache storage usage monitor.
           Prerequisite - If MSC supports RIS, Resource instance should be
                          selected using val_mpam_memory_configure_ris_sel
                          prior calling this API.
                        - MSC should support CSU monitoring, can be checked
                          using val_mpam_supports_csumon API..

  @param   msc_index - index of the MSC node in the MPAM info table.
**/
void
val_mpam_csumon_enable(uint32_t msc_index)
{
    addr_t base;
    uint32_t data;
    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);

    /* enable the monitor instance to collect information according to the configuration */
    data = BITFIELD_WRITE(val_mmio_read(base + REG_MSMON_CFG_CSU_CTL), CSU_CTL_EN, 1);
    val_mmio_write(base + REG_MSMON_CFG_CSU_CTL, data);
}

/**
  @brief   This API disables the Cache strorage usage monitor.
           Prerequisite - If MSC supports RIS, Resource instance should be
                          selected using val_mpam_memory_configure_ris_sel
                          prior calling this API.
                        - MSC should support CSU monitoring, can be checked
                          using val_mpam_supports_csumon API.

  @param   msc_index - index of the MSC node in the MPAM info table.
  @return  None
**/
void
val_mpam_csumon_disable(uint32_t msc_index)
{
    addr_t base;
    uint32_t data;
    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);

    /* enable the monitor instance to collect information according to the configuration */
    data = BITFIELD_WRITE(val_mmio_read(base + REG_MSMON_CFG_CSU_CTL), CSU_CTL_EN, 0);
    val_mmio_write(base + REG_MSMON_CFG_CSU_CTL, data);
}

/**
  @brief   This API reads the CSU montior counter value.
           Prerequisite - val_mpam_configure_csu_mon,
           This API can be called only after configuring CSU monitor.

  @param   msc_index  - MPAM feature page index for this MSC.
  @return  0 if monitor has Not Ready status,
           else counter value.
**/
uint32_t
val_mpam_read_csumon(uint32_t msc_index)
{
    addr_t base;
    uint32_t count;

    base = val_mpam_get_info(MPAM_MSC_BASE_ADDR, msc_index, 0);
    if (BITFIELD_READ(MSMON_CSU_NRDY, val_mmio_read(base + REG_MSMON_CSU)) == 0) {
        count = BITFIELD_READ(MSMON_CSU_VALUE,
                                      val_mmio_read(base + REG_MSMON_CSU));
        return count;
    }
    return 0;
}
