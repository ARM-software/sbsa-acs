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
#include "include/sbsa_avs_ras.h"
#include "include/sbsa_avs_pe.h"

static RAS_INFO_TABLE  *g_ras_info_table;
static RAS2_INFO_TABLE *g_ras2_info_table;

/**
  @brief   This API executes all the RAS tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_ras_create_info_table
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_ras_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status, i;
  uint32_t skip_module;
  uint64_t num_ras_nodes = 0;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == AVS_RAS_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "\n      USER Override - Skipping all RAS tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  skip_module = val_check_skip_module(AVS_RAS_TEST_NUM_BASE);
  if (skip_module) {
      val_print(AVS_PRINT_TEST, "\n USER Override - Skipping all RAS tests \n", 0);
      return AVS_STATUS_SKIP;
  }

  /* check if PE supports RAS extension, else skip all RAS tests */
  if (val_pe_feat_check(PE_FEAT_RAS)) {
      val_print(AVS_PRINT_TEST,
                "\n       PE RAS extension unimplemented. Skipping all RAS tests\n", 0);
      return AVS_STATUS_SKIP;
  }

  g_curr_module = 1 << RAS_MODULE;

  status = val_ras_get_info(RAS_INFO_NUM_NODES, 0, &num_ras_nodes);
  if (status || (num_ras_nodes == 0)) {
    val_print(AVS_PRINT_TEST, "\n       RAS nodes not found. Skipping all RAS tests\n", 0);
    return AVS_STATUS_SKIP;
  }

  /* set default status to AVS_STATUS_FAIL */
  status = AVS_STATUS_FAIL;

  val_print(AVS_PRINT_TEST, "\n      *** Starting RAS tests ***  \n", 0);

  if (g_sbsa_level > 6) {
      status = ras001_entry(num_pe);
      status |= ras002_entry(num_pe);
      status |= ras003_entry(num_pe);
      status |= ras004_entry(num_pe);
      status |= ras005_entry(num_pe);
      status |= ras006_entry(num_pe);
      status |= ras007_entry(num_pe);
      status |= ras008_entry(num_pe);
      status |= ras009_entry(num_pe);
      status |= ras010_entry(num_pe);
      status |= ras011_entry(num_pe);
      status |= ras012_entry(num_pe);
  }
  val_print_test_end(status, "RAS");

  return status;
}

/**
  @brief   This API will call PAL layer to fill in the RAS information
           into the address pointed by g_ras_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   ras_info_table  pre-allocated memory pointer for RAS info
  @return  Error if Input param is NULL
**/
uint32_t
val_ras_create_info_table(uint64_t *ras_info_table)
{

  if (ras_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "Input for Create Info table cannot be NULL \n", 0);
      return AVS_STATUS_ERR;
  }

  g_ras_info_table = (RAS_INFO_TABLE *)ras_info_table;

  pal_ras_create_info_table(g_ras_info_table);

  val_print(AVS_PRINT_TEST, " RAS_INFO: Number of RAS nodes        : %4d \n",
                           g_ras_info_table->num_nodes);

  return AVS_STATUS_PASS;
}

/**
  @brief  Free the memory allocated for the RAS information table
**/
void
val_ras_free_info_table()
{
  pal_mem_free((void *)g_ras_info_table);
}

/**
  @brief   This API will call PAL layer to fill in the RAS2 feature
           information into the address pointed by g_ras2_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   ras_info_table  pre-allocated memory pointer for RAS info
  @return  Error if Input param is NULL
**/
void
val_ras2_create_info_table(uint64_t *ras2_info_table)
{

  if (ras2_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "\nInput for RAS2 feat create info table cannot be NULL \n", 0);
      return;
  }

#ifndef TARGET_LINUX
  g_ras2_info_table = (RAS2_INFO_TABLE *)ras2_info_table;

  pal_ras2_create_info_table(g_ras2_info_table);

  val_print(AVS_PRINT_TEST, " RAS2_INFO: Number of RAS2 entries    : %4d \n",
                           g_ras2_info_table->num_all_block);
  val_print(AVS_PRINT_TEST, " RAS2_INFO: Num of RAS2 memory entries: %4d \n",
                           g_ras2_info_table->num_of_mem_block);
#endif
return;
}

/**
  @brief  Free the memory allocated for the RAS2 feature information table
**/
void
val_ras2_free_info_table()
{
  pal_mem_free((void *)g_ras2_info_table);
}

/**
  @brief   This API will use the info table to return the RAS information
           for info_type and param1 into the address pointed by ret_data.
           1. Caller       -  Test layer.
           2. Prerequisite -  val_ras_create_info_table.
  @param   info_type  Required RAS information type enum
  @param   param1     Variable passed from test for which info needs to be returned.
  @param   *ret_data  Return Data Pointer
  @return  Status
**/
uint32_t
val_ras_get_info(uint32_t info_type, uint32_t param1, uint64_t *ret_data)
{
  uint32_t status = AVS_STATUS_FAIL;
  uint32_t j = 0;
  uint64_t value = 0;
  uint64_t pe_affinity = 0;
  uint32_t pe_uid = 0;

  switch (info_type) {

  case RAS_INFO_NUM_NODES:
      *ret_data = g_ras_info_table->num_nodes;
      return AVS_STATUS_PASS;
  case RAS_INFO_NODE_TYPE:
      *ret_data = g_ras_info_table->node[param1].type;
      return AVS_STATUS_PASS;
  case RAS_INFO_INTF_TYPE:
      *ret_data = g_ras_info_table->node[param1].intf_info.intf_type;
      return AVS_STATUS_PASS;
  case RAS_INFO_PE_RES_TYPE:
      if (g_ras_info_table->node[param1].type == NODE_TYPE_PE) {
        *ret_data = g_ras_info_table->node[param1].node_data.pe.resource_type;
        return AVS_STATUS_PASS;
      } else
        return AVS_STATUS_FAIL;
      break;
  case RAS_INFO_MC_RES_PROX_DOMAIN:
      if (g_ras_info_table->node[param1].type == NODE_TYPE_MC) {
        *ret_data = g_ras_info_table->node[param1].node_data.mc.proximity_domain;
        return AVS_STATUS_PASS;
      } else
        return AVS_STATUS_FAIL;
      break;
  case RAS_INFO_NUM_PE:
      /* Returns the number of PE RAS Nodes */
      *ret_data = g_ras_info_table->num_pe_node;
      return AVS_STATUS_PASS;
      break;
  case RAS_INFO_NUM_MC:
      /* Returns the number of MC RAS Nodes */
      *ret_data = g_ras_info_table->num_mc_node;
      return AVS_STATUS_PASS;
      break;
  case RAS_INFO_BASE_ADDR:
      /* Returns the Base address of Error Group. Valid only in MMIO */
      if (g_ras_info_table->node[param1].intf_info.intf_type == RAS_INTERFACE_MMIO) {
        *ret_data = g_ras_info_table->node[param1].intf_info.base_addr;
        return AVS_STATUS_PASS;
      } else
        return AVS_STATUS_FAIL;
      break;
  case RAS_INFO_START_INDEX:
      /* Returns the Start Error Record Index Number */
      *ret_data = g_ras_info_table->node[param1].intf_info.start_rec_index;
      return AVS_STATUS_PASS;
      break;
  case RAS_INFO_NUM_ERR_REC:
      /* Returns the Number of Error Records */
      *ret_data = g_ras_info_table->node[param1].intf_info.num_err_rec;
      return AVS_STATUS_PASS;
      break;
  case RAS_INFO_ERR_REC_IMP:
      /* Returns the Error Record Implemented */
      *ret_data = g_ras_info_table->node[param1].intf_info.err_rec_implement;
      return AVS_STATUS_PASS;
      break;
  case RAS_INFO_ADDR_MODE:
      /* Returns the addressing mode bitmap for RAS address syndrome */
      *ret_data = g_ras_info_table->node[param1].intf_info.addressing_mode;
      return AVS_STATUS_PASS;
      break;
  case RAS_INFO_STATUS_REPORT:
      /* Returns the Error Reporting Status Field */
      *ret_data = g_ras_info_table->node[param1].intf_info.err_status_reporting;
      return AVS_STATUS_PASS;
      break;
  case RAS_INFO_PFG_SUPPORT:
      /* Returns the PFG Support */
      value = val_ras_reg_read(param1, RAS_ERR_FR, 0);
      if (value == INVALID_RAS_REG_VAL) {
          val_print(AVS_PRINT_ERR,
                "\n       Couldn't read ERR<0>FR register for RAS node index: 0x%lx",
                param1);
          return AVS_STATUS_FAIL;
      }
      if (value & ERR_FR_INJ_MASK)
        *ret_data = 0x1;
      else
        *ret_data = 0x0;
      return AVS_STATUS_PASS;
  case RAS_INFO_ERI_ID:
      /* Returns the ERI ID For the Node */
      for (j = 0; j < g_ras_info_table->node[param1].num_intr_entries; j++) {
        if (g_ras_info_table->node[param1].intr_info[j].type) {
          *ret_data = g_ras_info_table->node[param1].intr_info[j].gsiv;
          return AVS_STATUS_PASS;
        }
      }
      return AVS_STATUS_FAIL;
      break;
  case RAS_INFO_FHI_ID:
      /* Returns the FHI ID For the Node */
      for (j = 0; j < g_ras_info_table->node[param1].num_intr_entries; j++) {
        if (!(g_ras_info_table->node[param1].intr_info[j].type)) {
          *ret_data = g_ras_info_table->node[param1].intr_info[j].gsiv;
          return AVS_STATUS_PASS;
        }
      }
      return AVS_STATUS_FAIL;
      break;
  case RAS_INFO_NODE_INDEX_FOR_AFF:
      /* Returns the Node Index For the PE node for MPIDR = param1 */
      val_print(AVS_PRINT_DEBUG,
                "\n       RAS_GET_INFO : Param1 = 0x%x ",
                param1);
      for (j = 0; j < g_ras_info_table->num_nodes; j++) {
        if (g_ras_info_table->node[j].type == NODE_TYPE_PE) {
          if (g_ras_info_table->node[j].node_data.pe.flags & 0x1) {
            /* This is a global node return the index*/
            *ret_data = j;
            return AVS_STATUS_PASS;
          } else if (g_ras_info_table->node[j].node_data.pe.flags & 0x2) {
            /* This is a shared resource */
            /* Get RAS node interface type and read for processor affinity */
            if (g_ras_info_table->node[j].intf_info.intf_type == RAS_INTF_TYPE_SYS_REG)
              /* affinity field read from ACPI */
              pe_affinity = g_ras_info_table->node[j].node_data.pe.affinity;
            else {
              pe_affinity = val_ras_reg_read(RAS_ERR_ERRDEVAFF, j, 0);
              if (pe_affinity == INVALID_RAS_REG_VAL) {
                val_print(AVS_PRINT_ERR,
                "\n       RAS_GET_INFO : Invalid pe_affinity (ERR_ERRDEVAFF) for RAS node = %d ",
                j);
                return AVS_STATUS_FAIL;
              }
            }
            /* check if PE belongs to any of the higher affinity level i.e, 1, 2, or 3 */
            if (((param1 & PE_AFFINITY_LVL_1) == (pe_affinity & PE_AFFINITY_LVL_1))
                 || ((param1 & PE_AFFINITY_LVL_2) == (pe_affinity & PE_AFFINITY_LVL_2))
                 || ((param1 & PE_AFFINITY_LVL_3) == (pe_affinity & PE_AFFINITY_LVL_3))) {
              *ret_data = j;
              return AVS_STATUS_PASS;
            }
          } else {
            /* if global or shared error flag not set for node,
               Use g_ras_info_table->node[j].node_data.pe.processor_id */
            /* get processor UID */
            pe_uid = val_pe_get_uid(param1);
            if (pe_uid == INVALID_PE_INFO) {
                val_print(AVS_PRINT_ERR,
                "\n       RAS_GET_INFO : Invalid PE UID for MPIDR = %lx",
                param1);
                return AVS_STATUS_FAIL;
            }
            if (pe_uid == g_ras_info_table->node[j].node_data.pe.processor_id) {
              *ret_data = j;
              return AVS_STATUS_PASS;
            }
          }
        }
      }
      val_print(AVS_PRINT_ERR,
                "\n       RAS_GET_INFO : No PE RAS node matches with MPIDR = %lx",
                param1);
      return AVS_STATUS_FAIL;
      break;
  default:
      break;

  }

  return status;
}

/**
  @brief   This API is a single point of entry to retrieve
           RAS2 memory feature info from RAS2 info table.
           1. Caller       -  Test suite
           2. Prerequisite -  val_ras2_create_info_table
  @param   type   the type of information being requested.
  @param   index  the index of RAS2 memory feat info instance.
  @return  64-bit data
**/
uint64_t
val_ras2_get_mem_info(RAS2_MEM_INFO_e type, uint32_t index)
{
  uint32_t i, j = 0;
  RAS2_BLOCK *block;

  if (g_ras2_info_table == NULL) {
      val_print(AVS_PRINT_ERR, "\nRAS2_GET_MEM_INFO : ras2 info table is not created \n", 0);
      return 0; /* imply no ras2_info entries */
  }

  if (type == RAS2_NUM_MEM_BLOCK)
      return g_ras2_info_table->num_of_mem_block;

  /* check if index in range */
  if (index > g_ras2_info_table->num_of_mem_block - 1) {
      val_print(AVS_PRINT_ERR,
                "\nRAS2_GET_MEM_INFO: Index (%d) is greater than num of RAS2 mem blocks\n",
                 index);
      return INVALID_RAS2_INFO;
  }

  /* Go through the table return the relevant field value for RAS2 memory info block */
  /* at the index position */
  block = &g_ras2_info_table->blocks[0];
  for (i = 0; i < g_ras2_info_table->num_all_block; i++, block++) {
      if (block->type == RAS2_FEATURE_TYPE_MEMORY) {
          if (j == index) {
              switch (type) {
              case RAS2_PROX_DOMAIN:
                  return block->block_info.mem_feat_info.proximity_domain;
              case RAS2_SCRUB_SUPPORT:
                  return block->block_info.mem_feat_info.patrol_scrub_support;
              default:
                  val_print(AVS_PRINT_ERR,
                            "\nThis RAS2 memory info option not supported: %d \n", type);
                  return INVALID_RAS2_INFO;
              }
          }
          j++;
      }
  }
  return INVALID_RAS2_INFO;
}

/**
  @brief   This API will be used to Read RAS Registers based on interface
           1. Caller       -  Test layer.
           2. Prerequisite -  val_ras_create_info_table.
  @param   node_index  RAS Node Index
  @param   reg         Register to read.
  @param   err_rec_idx Error record index (required for record specific registers).
  @return  register read value or INVALID_RAS_REG_VAL if failure
**/
uint64_t
val_ras_reg_read(uint32_t node_index, uint32_t reg, uint32_t err_rec_idx)
{
  uint64_t base, value = INVALID_RAS_REG_VAL;
  uint32_t start_rec_index, offset;
  uint64_t num_err_recs, err_rec_impl_bitmap;

  start_rec_index = g_ras_info_table->node[node_index].intf_info.start_rec_index;

  /* err_rec_idx = 0 means the first error record of the node */
  if (err_rec_idx == 0)
      err_rec_idx = start_rec_index;

  /* Check if err record index is valid */
  val_ras_get_info(RAS_INFO_NUM_ERR_REC, node_index, &num_err_recs);
  if ((err_rec_idx - start_rec_index) >= num_err_recs) {
      val_print(AVS_PRINT_ERR,
                "\n       RAS_REG_READ : Invalid Input error record index(%d)\n", err_rec_idx);
      return INVALID_RAS_REG_VAL;
  }

  /* check if err record is implemented for given node index*/
  val_ras_get_info(RAS_INFO_ERR_REC_IMP, node_index, &err_rec_impl_bitmap);

  if ((err_rec_impl_bitmap >> err_rec_idx) & 0x1) {
      val_print(AVS_PRINT_ERR,
                "\n       RAS_REG_READ : Error record index(%d) is unimplemented ", err_rec_idx);
      val_print(AVS_PRINT_ERR,
                "for node with index: %d \n", node_index);
      return INVALID_RAS_REG_VAL;
  }

  if (g_ras_info_table->node[node_index].intf_info.intf_type == RAS_INTF_TYPE_MMIO) {
      /* MMIO based RAS register read */

      /* Get the Base address for this node */
      base = g_ras_info_table->node[node_index].intf_info.base_addr;

      switch (reg) {
      case RAS_ERR_FR:
          /* ERR<n>FR RAS register of first standard error record is
            shared across multiple error records if exists */
          offset = ERR_FR_OFFSET + (64 * start_rec_index);
          break;
      case RAS_ERR_CTLR:
          /* ERR<n>CTLR RAS register of first standard error record is
            shared across multiple error records if exists */
          offset = ERR_CTLR_OFFSET + (64 * start_rec_index);
          break;
      case RAS_ERR_STATUS:
          offset = ERR_STATUS_OFFSET + (64 * err_rec_idx);
          break;
      case RAS_ERR_ADDR:
          offset = ERR_ADDR_OFFSET + (64 * err_rec_idx);
          break;
      case RAS_ERR_PFGCDN:
          /* ERR<n>PFGCDN RAS register is valid only for first error record */
          if (err_rec_idx == start_rec_index)
              offset = ERR_PFGCDN_OFFSET + (64 * start_rec_index);
          else {
              val_print(AVS_PRINT_ERR,
                "\n       RAS_REG_READ : ERR<%d>PFGCDN is RES0 for node index :", err_rec_idx);
              val_print(AVS_PRINT_ERR, " %d", node_index);
              return INVALID_RAS_REG_VAL;
          }
          break;
      case RAS_ERR_PFGCTL:
          /* ERR<n>PFGCTL RAS register is valid only for first error record */
          if (err_rec_idx == start_rec_index)
              offset = ERR_PFGCTL_OFFSET + (64 * start_rec_index);
          else {
              val_print(AVS_PRINT_ERR,
                "\n       RAS_REG_READ : ERR<%d>PFGCTL is RES0 for node index :", err_rec_idx);
              val_print(AVS_PRINT_ERR, " %d", node_index);
              return INVALID_RAS_REG_VAL;
          }
          break;
      case RAS_ERR_ERRDEVAFF:
          /* only valid for MMIO interface */
          offset = ERR_ERRDEVAFF_OFFSET;
      default:
          break;
      }
      value = val_mmio_read(base + offset);
  } else {
      /* System register based read */

      /* RAS registers reads with ERRSELR_EL1.SEL set to start error record index */
      AA64WriteErrSelr1(start_rec_index);

      switch (reg) {
      case RAS_ERR_FR:
          value = AA64ReadErrFr1();
          break;
      case RAS_ERR_CTLR:
          value = AA64ReadErrCtlr1();
          break;
      case RAS_ERR_PFGCDN:
          /* ERR<n>PFGCDN RAS register is valid only for first error record */
          if (err_rec_idx == start_rec_index)
              value = AA64ReadErrPfgcdn1();
          else {
              val_print(AVS_PRINT_ERR,
                "\n       RAS_REG_READ : ERR<%d>PFGCDN is RES0 for the node index :", err_rec_idx);
              val_print(AVS_PRINT_ERR, " %d", node_index);
              return INVALID_RAS_REG_VAL;
          }
          break;
      case RAS_ERR_PFGCTL:
          /* ERR<n>PFGCTL RAS register is valid only for first error record */
          if (err_rec_idx == start_rec_index)
              value = AA64ReadErrPfgctl1();
          else {
              val_print(AVS_PRINT_ERR,
                "\n       RAS_REG_READ : ERR<%d>PFGCTL is RES0 for the node index :", err_rec_idx);
              val_print(AVS_PRINT_ERR, " %d", node_index);
              return INVALID_RAS_REG_VAL;
          }
          break;
      default:
          break;
      }

      /* RAS registers reads with ERRSELR_EL1.SEL set to current error record index
        These registers are unique to given error record */
      AA64WriteErrSelr1(err_rec_idx);

      switch (reg) {
      case RAS_ERR_STATUS:
          value = AA64ReadErrStatus1();
          break;
      case RAS_ERR_ADDR:
          value = AA64ReadErrAddr1();
          break;
      default:
          break;
      }
  }

  return value;
}

/**
  @brief   This API will be used to Write RAS Registers based on interface
           1. Caller       -  Test layer.
           2. Prerequisite -  val_ras_create_info_table.
  @param   node_index  RAS Node Index
  @param   reg         Register to write.
  @param   write_data  Value to write in reg
  @return  None
**/
void
val_ras_reg_write(uint32_t node_index, uint32_t reg, uint64_t write_data)
{
  uint64_t base;
  uint32_t rec_index, offset;

  rec_index = g_ras_info_table->node[node_index].intf_info.start_rec_index;

  if (g_ras_info_table->node[node_index].intf_info.intf_type == RAS_INTF_TYPE_MMIO) {
    /* MMIO Based Write */

    /* Get the Base address for this node */
    base = g_ras_info_table->node[node_index].intf_info.base_addr;

    switch (reg) {
    case RAS_ERR_FR:
      offset = ERR_FR_OFFSET + (64 * rec_index);
      break;
    case RAS_ERR_CTLR:
      offset = ERR_CTLR_OFFSET + (64 * rec_index);
      break;
    case RAS_ERR_STATUS:
      offset = ERR_STATUS_OFFSET + (64 * rec_index);
      break;
    case RAS_ERR_PFGCDN:
      offset = ERR_PFGCDN_OFFSET + (64 * rec_index);
      break;
    case RAS_ERR_PFGCTL:
      offset = ERR_PFGCTL_OFFSET + (64 * rec_index);
      break;
    default:
      break;
    }

    val_mmio_write(base + offset, write_data);
  } else {
    /* System register based Write */

    /* Update ERRSELR_EL1.SEL to choose which record index to use */
    AA64WriteErrSelr1(rec_index);

    switch (reg) {
    case RAS_ERR_CTLR:
      AA64WriteErrCtlr1(write_data);
      break;
    case RAS_ERR_STATUS:
      AA64WriteErrStatus1(write_data);
      break;
    case RAS_ERR_PFGCDN:
      AA64WriteErrPfgcdn1(write_data);
      break;
    case RAS_ERR_PFGCTL:
      AA64WriteErrPfgctl1(write_data);
      break;
    default:
      break;
    }
  }
}

/**
  @brief  Function for setting up the Error Environment

  @param  in_param  - Error Input Parameters.
  @param  *out_param  - Parameters returned from platform to be used in the test.

  @return  Status
**/
uint32_t
val_ras_setup_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param)
{
  uint32_t status = AVS_STATUS_FAIL;
  uint32_t pfgctl_value = 0;

  /* Clear the ERR_STATUS for any previous error */
  val_ras_reg_write(in_param.node_index, RAS_ERR_STATUS, ERR_STATUS_CLEAR);

  /* Make Sure ERI/FHI is not enabled */
  val_ras_reg_write(in_param.node_index, RAS_ERR_CTLR, 0);

  /* Enable fault injection: ERR<n>CTLR.ED=1 */
  val_ras_reg_write(in_param.node_index, RAS_ERR_CTLR, ERR_CTLR_ED_ENABLE);

  /* Check if Pseudo Fault needs to test */
  if (in_param.is_pfg_check)
  {
    /* Write Counter in ERR<n>PFGCDN */
    val_ras_reg_write(in_param.node_index, RAS_ERR_PFGCDN, 0x5);

    /* Write to ERR<n>PFGCTL.* To Enable Error */
    switch (in_param.ras_error_type) {
    case ERR_UC:
                pfgctl_value = ERR_PFGCTL_UC_ENABLE;
                break;
    case ERR_DE:
                pfgctl_value = ERR_PFGCTL_DE_ENABLE;
                break;
    case ERR_CE:
                pfgctl_value = ERR_PFGCTL_CE_NON_ENABLE;
                break;
    case ERR_CRITICAL:
                pfgctl_value = ERR_PFGCTL_CI_ENABLE;
                break;
    default:
                break;
    }
    val_ras_reg_write(in_param.node_index, RAS_ERR_PFGCTL, pfgctl_value);

    return AVS_STATUS_PASS;
  }

  /* Platform defined way of error setup */
  status = pal_ras_setup_error(in_param, out_param);
  return status;
}

/**
  @brief  Platform Defined way of Timeout/Wait loop

  @param  count  - Timeout/Wait Multiplier.

  @return None
**/
void
val_ras_wait_timeout(uint32_t count)
{
  pal_ras_wait_timeout(count);
}

void
ras_pfg_access_node(uint32_t node_index)
{
  uint64_t reg_value;

  /* Loop for Wait */
  val_ras_wait_timeout(1);

  /* Access to the Node register, Might need an imp def way here */
  reg_value = val_ras_reg_read(node_index, RAS_ERR_CTLR, 0);
  if (reg_value == INVALID_RAS_REG_VAL) {
      val_print(AVS_PRINT_ERR,
                "\n       Couldn't read ERR<0>CTLR register for RAS node index: 0x%lx",
                node_index);
  }

  val_print(AVS_PRINT_INFO, "      Access RAS Node, CTLR : 0x%llx \n", reg_value);
}

/**
  @brief  Function for injecting the Error

  @param  in_param  - Error Input Parameters.
  @param  *out_param  - Parameters returned from platform to be used in the test.

  @return  Status
**/
uint32_t
val_ras_inject_error(RAS_ERR_IN_t in_param, RAS_ERR_OUT_t *out_param)
{
  uint32_t status = AVS_STATUS_FAIL;
  uint64_t reg_value;

  /* Check if Pseudo Fault needs to test */
  if (in_param.is_pfg_check)
  {
    /* Write to ERR<n>PFGCTL.CDNEN */
    reg_value = val_ras_reg_read(in_param.node_index, RAS_ERR_PFGCTL, in_param.rec_index);

    if (reg_value == INVALID_RAS_REG_VAL) {
        val_print(AVS_PRINT_ERR,
                    "\n       Couldn't read ERR<%d>PFGCTL register for ",
                    in_param.rec_index);
        val_print(AVS_PRINT_ERR,
                    "RAS node index: 0x%lx",
                    in_param.node_index);
        return AVS_STATUS_FAIL;
    }

    val_ras_reg_write(in_param.node_index, RAS_ERR_PFGCTL, reg_value | ERR_PFGCTL_CDNEN_ENABLE);

    /* Wait and Access to Node */
    ras_pfg_access_node(in_param.node_index);

    return AVS_STATUS_PASS;
  }

  /* Platform Defined Way of Error Injection */
  status = pal_ras_inject_error(in_param, out_param);

  return status;
}

/**
  @brief  Function to check the Error Record Status

  @param  node_index  - RAS Node index in the info table.
  @param  error_type  - RAS Error Type.

  @return  Status
**/
uint32_t val_ras_check_err_record(uint32_t node_index, uint32_t error_type)
{
  uint32_t status = AVS_STATUS_PASS;
  uint64_t err_status;
  uint32_t err_type_mask;

  /* Loop for Wait */
  val_ras_wait_timeout(1);

  err_status = val_ras_reg_read(node_index, RAS_ERR_STATUS, 0);
  if (err_status == INVALID_RAS_REG_VAL) {
      val_print(AVS_PRINT_ERR,
                "\n       Couldn't read ERR<0>STATUS register for RAS node index: 0x%lx",
                node_index);
      return AVS_STATUS_FAIL;
  }

  /* Check Status Register Validity in Ras Node */
  if (!(err_status & ERR_STATUS_V_MASK)) {
    val_print(AVS_PRINT_DEBUG, "\n       Status Reg Not Valid, for node %d", node_index);
    status = AVS_STATUS_FAIL;
  }

  switch (error_type) {
  case ERR_UC:
              err_type_mask = ERR_STATUS_UE_MASK;
              break;
  case ERR_DE:
              err_type_mask = ERR_STATUS_DE_MASK;
              break;
  case ERR_CE:
              err_type_mask = ERR_STATUS_CE_MASK;
              break;
  case ERR_CRITICAL:
              err_type_mask = ERR_STATUS_CI_MASK;
              break;
  default:
              break;
  }

  /* Check Error Bit in Ras Node */
  if (!(err_status & err_type_mask)) {
    val_print(AVS_PRINT_DEBUG, "\n       ERR Status Type Fail, for node %d", node_index);
    status = AVS_STATUS_FAIL;
  }

  return status;
}

/**
  @brief  API to check support for Poison

  @param  None

  @return  0 - Poison storage & forwarding not supported
           1 - Poison storage & forwarding supported
**/
uint32_t val_ras_check_plat_poison_support(void)
{
  return pal_ras_check_plat_poison_support();
}
