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


#include "sbsa_gic_its.h"
#include "include/sbsa_avs_gic_support.h"

uint64_t ArmReadMpidr(void);

extern GIC_ITS_INFO    *g_gic_its_info;
static uint32_t        *g_cwriter_ptr;
static uint32_t        g_its_setup_done;

uint64_t val_its_get_curr_rdbase(uint64_t rd_base, uint32_t length)
{
  uint64_t     Mpidr;
  uint32_t     Affinity, CpuAffinity;
  uint32_t     rd_granularity;
  uint64_t     curr_rd_base; /* RD Base for Current CPU */
  uint32_t     typer;

  Mpidr = ArmReadMpidr();

  CpuAffinity = (Mpidr & (ARM_CORE_AFF0 | ARM_CORE_AFF1 | ARM_CORE_AFF2)) |
                ((Mpidr & ARM_CORE_AFF3) >> 8);

  rd_granularity = ARM_GICR_CTLR_FRAME_SIZE
                   + ARM_GICR_SGI_PPI_FRAME_SIZE;

  typer = val_mmio_read(rd_base + ARM_GICR_TYPER);

  /* Skip VLPI_base + reserved page */
  if (typer & ARM_GICR_TYPER_VLPIS)
      rd_granularity += ARM_GICR_VLPI_FRAME_SIZE + ARM_GICR_RESERVED_PAGE_SIZE;

  curr_rd_base = rd_base;

  /* If information is present in GICC Structure */
  if (length == 0)
  {
      Affinity = val_mmio_read(curr_rd_base + ARM_GICR_TYPER + NEXT_DW_OFFSET);
      if (Affinity == CpuAffinity) {
        return curr_rd_base;
      }
      return 0;
  }

  /* If information is present in GICR Structure */
  while (curr_rd_base < (rd_base + length))
  {
    Affinity = val_mmio_read(curr_rd_base + ARM_GICR_TYPER + NEXT_DW_OFFSET);

    if (Affinity == CpuAffinity) {
      return curr_rd_base;
    }

    /* Move to the next GIC Redistributor frame */
    curr_rd_base += rd_granularity;
  }

  return 0;
}

uint32_t val_its_gicd_lpi_support(uint64_t gicd_base)
{
  return (val_mmio_read(gicd_base + ARM_GICD_TYPER) & ARM_GICD_TYPER_LPIS);
}

uint32_t val_its_gicr_lpi_support(uint64_t rd_base)
{
  return (val_mmio_read(rd_base + ARM_GICR_TYPER) & ARM_GICR_TYPER_PLPIS);
}

uint32_t
ArmGicSetItsCommandQueueBase(
    uint32_t     its_index
  )
{
  /* Allocate Memory for Command queue. Set command queue base in GITS_CBASER. */
  uint64_t                    Address;
  uint64_t                    write_value;
  uint64_t                    ItsBase;

  ItsBase = g_gic_its_info->GicIts[its_index].Base;

  Address = (uint64_t)val_aligned_alloc(SIZE_64KB, (NUM_PAGES_8 * SIZE_4KB));

  if (!Address) {
    val_print(AVS_PRINT_ERR,  "ITS : Could Not Allocate Memory CmdQ. Test may not pass.\n", 0);
    return 1;
  }

  val_memory_set((void *)Address,  (NUM_PAGES_8*SIZE_4KB), 0);

  g_gic_its_info->GicIts[its_index].CommandQBase = Address;

  write_value = val_mmio_read64(ItsBase + ARM_GITS_CBASER) & (~ARM_GITS_CBASER_PA_MASK);
  write_value = write_value | (Address & ARM_GITS_CBASER_PA_MASK);
  write_value = write_value | ARM_GITS_CBASER_VALID | (NUM_PAGES_8 - 1);

  val_mmio_write64(ItsBase + ARM_GITS_CBASER, write_value);

  return 0;
}


uint32_t ArmGicSetItsTables(uint32_t its_index)
{
  uint32_t                Pages;
  uint32_t                TableSize, entry_size;
  uint64_t                its_baser, its_typer;
  uint8_t                 it, table_type;
  uint64_t                write_value;
  uint32_t                DevBits, CIDBits;
  uint64_t                Address;
  uint64_t                ItsBase;

  ItsBase = g_gic_its_info->GicIts[its_index].Base;

  /* Allocate Memory for Table Depending on the Type of the table in GITS_BASER<n>. */
  for (it = 0; it < ARM_NUM_GITS_BASER; it++) {

    its_baser = val_mmio_read64(ItsBase + ARM_GITS_BASER(it));
    table_type = ARM_GITS_BASER_GET_TYPE(its_baser);
    entry_size = ARM_GITS_BASER_GET_ENTRY_SIZE(its_baser);

    its_typer = val_mmio_read64(ItsBase + ARM_GITS_TYPER);
    DevBits = ARM_GITS_TYPER_DevBits(its_typer);
    CIDBits = ARM_GITS_TYPER_CIDBits(its_typer);

    if (table_type == ARM_GITS_TBL_TYPE_DEVICE) {
      TableSize = (1 << (DevBits+1))*(entry_size+1); // Assuming Single Level Table

    } else if (table_type == ARM_GITS_TBL_TYPE_CLCN) {
      TableSize = (1 << (CIDBits+1))*(entry_size+1); // Assuming Single Level Table

    } else {
      continue;
    }

    Pages = SIZE_TO_PAGES (TableSize);

  Address = (uint64_t)val_aligned_alloc(SIZE_64KB, PAGES_TO_SIZE(Pages));

  if (!Address) {
      val_print(AVS_PRINT_ERR,  "ITS : Could Not Allocate Memory DT/CT. Test may not pass.\n", 0);
      return 1;
  }

  val_memory_set((void *)Address,  PAGES_TO_SIZE(Pages), 0);

  write_value = val_mmio_read64(ItsBase + ARM_GITS_BASER(it));
  write_value = write_value & (~ARM_GITS_BASER_PA_MASK);
  write_value = write_value | (Address & ARM_GITS_BASER_PA_MASK);
  write_value = write_value | ARM_GITS_BASER_VALID;
  write_value = write_value | (Pages-1);
  val_mmio_write64(ItsBase + ARM_GITS_BASER(it), write_value);

  }

  /* Allocate Memory for Interrupt Translation Table */
  Address = (uint64_t)val_aligned_alloc(SIZE_64KB, (NUM_PAGES_8 * SIZE_4KB));

  if (!Address) {
    val_print(AVS_PRINT_ERR,  "ITS : Could Not Allocate Memory For ITT. Test may not pass.\n", 0);
    return 1;
  }

  val_memory_set((void *)Address, (NUM_PAGES_8*SIZE_4KB), 0);

  g_gic_its_info->GicIts[its_index].ITTBase = Address;

  return 0;
}


void EnableITS(uint64_t GicItsBase)
{
  /* Set GITS_CTLR.Enable as 1 to enable the ITS */
  uint32_t    value;

  value = val_mmio_read(GicItsBase + ARM_GITS_CTLR);
  val_mmio_write(GicItsBase + ARM_GITS_CTLR, (value | ARM_GITS_CTLR_ENABLE));
}

void
WriteCmdQMAPD(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint64_t     device_id,
   uint64_t     ITT_BASE,
   uint32_t     Size,
   uint64_t     Valid
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)((device_id << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_MAPD));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1), (uint64_t)(Size));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2),
                     (uint64_t)((Valid << ITS_CMD_SHIFT_VALID) | (ITT_BASE & ITT_PAR_MASK)));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0x0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}

void
WriteCmdQMAPC(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint32_t     device_id,
   uint32_t     Clctn_ID,
   uint32_t     RDBase,
   uint64_t     Valid
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)(ARM_ITS_CMD_MAPC));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1), (uint64_t)(0x0));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2),
                     (uint64_t)((Valid << ITS_CMD_SHIFT_VALID) | RDBase | Clctn_ID));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0x0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}

void
WriteCmdQMAPI(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint64_t     device_id,
   uint32_t     int_id,
   uint32_t     Clctn_ID
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)((device_id << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_MAPI));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1), (uint64_t)(int_id));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2), (uint64_t)(Clctn_ID));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0x0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}

void
WriteCmdQINV(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint64_t     device_id,
   uint32_t     int_id
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)((device_id << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_INV));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1), (uint64_t)(int_id));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2), (uint64_t)(0x0));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0x0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}

void
WriteCmdQDISCARD(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint64_t     device_id,
   uint32_t     int_id
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)((device_id << ITS_CMD_SHIFT_DEVID) | ARM_ITS_CMD_DISCARD));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1), (uint64_t)(int_id));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2), (uint64_t)(0x0));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0x0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}


void
WriteCmdQSYNC(
   uint32_t     its_index,
   uint64_t     *CMDQ_BASE,
   uint32_t     RDBase
  )
{
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index]),
                     (uint64_t)(ARM_ITS_CMD_SYNC));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 1), (uint64_t)(0x0));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 2), (uint64_t)(RDBase));
    val_mmio_write64((uint64_t)(CMDQ_BASE + g_cwriter_ptr[its_index] + 3), (uint64_t)(0x0));
    g_cwriter_ptr[its_index] = g_cwriter_ptr[its_index] + ITS_NEXT_CMD_PTR;
}

void PollTillCommandQueueDone(uint32_t its_index)
{
  uint32_t    count;
  uint64_t    creadr_value;
  uint64_t    stall_value;
  uint64_t    cwriter_value;
  uint64_t    ItsBase;

  count = 0;
  ItsBase = g_gic_its_info->GicIts[its_index].Base;
  cwriter_value = val_mmio_read64(ItsBase + ARM_GITS_CWRITER);
  creadr_value = val_mmio_read64(ItsBase + ARM_GITS_CREADR);

  while (creadr_value != cwriter_value) {
    /* Check Stall Value */
    stall_value = creadr_value & ARM_GITS_CREADR_STALL;

    if (stall_value) {
      /* Retry */
      val_mmio_write64((ItsBase + ARM_GITS_CWRITER),
                  (cwriter_value | ARM_GITS_CWRITER_RETRY)
                 );
    }

    count++;
    if (count > WAIT_ITS_COMMAND_DONE) {
      val_print(AVS_PRINT_ERR,
                "\n       ITS : Command Queue READR not moving, Test may not pass", 0);
      break;
    }

    creadr_value = val_mmio_read64(ItsBase + ARM_GITS_CREADR);
  }

}

uint64_t GetRDBaseFormat(uint32_t its_index)
{
  uint32_t    value;
  uint64_t    pe_num;
  uint64_t    ItsBase;

  ItsBase = g_gic_its_info->GicIts[its_index].Base;

  /* Check GITS_TYPER.PTA.
     If PTA = 1 then RDBase = Physical Address,
     Else RDBase = GICR_TYPER.Processor_Number
  */
  value = val_mmio_read64(ItsBase + ARM_GITS_TYPER);
  if (value & ARM_GITS_TYPER_PTA) {
    return g_gic_its_info->GicRdBase;
  } else {
    value = val_mmio_read64(g_gic_its_info->GicRdBase + ARM_GICR_TYPER);
    pe_num = (value & ARM_GICR_TYPER_PN_MASK) >> ARM_GICR_TYPER_PN_SHIFT;

    /* RDBase is made 64KB aligned */
    return (pe_num << RD_BASE_SHIFT);
  }
}


void val_its_clear_lpi_map(uint32_t its_index, uint32_t device_id, uint32_t int_id)
{
  uint64_t    value;
  uint64_t    RDBase;
  uint64_t    ItsBase;
  uint64_t    ItsCommandBase;

  if (!g_its_setup_done)
    return;

  ItsBase        = g_gic_its_info->GicIts[its_index].Base;
  ItsCommandBase = g_gic_its_info->GicIts[its_index].CommandQBase;

  /* Clear Config table for LPI=int_id */
  ClearConfigTable(int_id);

  /* Get RDBase Depending on GITS_TYPER.PTA */
  RDBase = GetRDBaseFormat(its_index);

  /* Discard Mappings */
  WriteCmdQDISCARD(its_index, (uint64_t *)(ItsCommandBase), device_id, int_id);
  /* Un Map Device using MAPD */
  WriteCmdQMAPD(its_index, (uint64_t *)(ItsCommandBase), device_id,
                g_gic_its_info->GicIts[its_index].ITTBase,
                0, 0 /*InValid*/);
  /* ITS SYNC Command */
  WriteCmdQSYNC(its_index, (uint64_t *)(ItsCommandBase), RDBase);

  TestExecuteBarrier();
  /* Update the CWRITER Register so that all the commands from Command queue gets executed.*/
  value = ((g_cwriter_ptr[its_index] * NUM_BYTES_IN_DW));
  val_mmio_write64((ItsBase + ARM_GITS_CWRITER), value);

  /* Check CREADR value which ensures Command Queue is processed */
  PollTillCommandQueueDone(its_index);
  TestExecuteBarrier();

}

void val_its_create_lpi_map(uint32_t its_index, uint32_t device_id,
                            uint32_t int_id, uint32_t Priority)
{
  uint64_t    value;
  uint64_t    RDBase;
  uint64_t    ItsBase;
  uint64_t    ItsCommandBase;

  if (!g_its_setup_done)
    return;

  ItsBase        = g_gic_its_info->GicIts[its_index].Base;
  ItsCommandBase = g_gic_its_info->GicIts[its_index].CommandQBase;

  /* Set Config table with enable the LPI = int_id, Priority. */
  SetConfigTable(int_id, Priority);

  /* Enable Redistributor */
  EnableLPIsRD(g_gic_its_info->GicRdBase);

  /* Enable ITS */
  EnableITS(ItsBase);

  /* Get RDBase Depending on GITS_TYPER.PTA */
  RDBase = GetRDBaseFormat(its_index);

  /* Map Device using MAPD */
  WriteCmdQMAPD(its_index, (uint64_t *)(ItsCommandBase), device_id,
                g_gic_its_info->GicIts[its_index].ITTBase,
                g_gic_its_info->GicIts[its_index].IDBits, 0x1 /*Valid*/);
  /* Map Collection using MAPC */
  WriteCmdQMAPC(its_index, (uint64_t *)(ItsCommandBase), device_id,
                0x1 /*Clctn_ID*/, RDBase, 0x1 /*Valid*/);
  /* Map Interrupt using MAPI */
  WriteCmdQMAPI(its_index, (uint64_t *)(ItsCommandBase), device_id, int_id, 0x1 /*Clctn_ID*/);
  /* Invalid Entry */
  WriteCmdQINV(its_index, (uint64_t *)(ItsCommandBase), device_id, int_id);
  /* ITS SYNC Command */
  WriteCmdQSYNC(its_index, (uint64_t *)(ItsCommandBase), RDBase);

  TestExecuteBarrier();

  /* Update the CWRITER Register so that all the commands from Command queue gets executed.*/
  value = ((g_cwriter_ptr[its_index] * NUM_BYTES_IN_DW));
  val_mmio_write64((ItsBase + ARM_GITS_CWRITER), value);

  /* Check CREADR value which ensures Command Queue is processed */
  PollTillCommandQueueDone(its_index);
  TestExecuteBarrier();

}


uint32_t val_its_get_max_lpi(void)
{
  uint32_t    index;
  uint32_t    min_idbits = ARM_LPI_MAX_IDBITS;

  if ((g_gic_its_info == NULL) || (g_gic_its_info->GicNumIts == 0))
    return 0;

  if (!g_its_setup_done)
    return 0;

  /* Return The Minimum IDBits supported in ITS */
  for (index = 0; index < g_gic_its_info->GicNumIts; index++)
  {
    min_idbits = (min_idbits < g_gic_its_info->GicIts[index].IDBits) ?
                 (min_idbits) :
                 (g_gic_its_info->GicIts[index].IDBits);
  }
  return ((1 << (min_idbits+1)) - 1);
}


uint64_t val_its_get_translater_addr(uint32_t its_index)
{
  return (g_gic_its_info->GicIts[its_index].Base + ARM_GITS_TRANSLATER);
}


uint32_t
SetInitialConfiguration(
  uint32_t     its_index
  )
{
  /* Program GIC Redistributor with the Min ID bits supported. */
  uint32_t    gicd_typer_idbits, gits_typer_bits;
  uint64_t    write_value;
  uint64_t    ItsBase;

  ItsBase = g_gic_its_info->GicIts[its_index].Base;

  gicd_typer_idbits = ARM_GICD_TYPER_IDbits(val_mmio_read(
                                            g_gic_its_info->GicDBase + ARM_GICD_TYPER));
  gits_typer_bits = ARM_GITS_TYPER_IDbits(val_mmio_read64(ItsBase + ARM_GITS_TYPER));

  /* Check least bits implemented is 14 if LPIs are supported. */
  if (GET_MIN(gicd_typer_idbits, gits_typer_bits) < ARM_LPI_MIN_IDBITS) {
    return 1;
  }

  write_value = val_mmio_read64(g_gic_its_info->GicRdBase + ARM_GICR_PROPBASER);
  write_value |= GET_MIN(gicd_typer_idbits, gits_typer_bits);
  g_gic_its_info->GicIts[its_index].IDBits = GET_MIN(gicd_typer_idbits, gits_typer_bits);

  val_mmio_write64((g_gic_its_info->GicRdBase + ARM_GICR_PROPBASER), write_value);

  return 0;
}


uint32_t val_its_init(void)
{
  uint32_t    Status;
  uint32_t    index;

  g_cwriter_ptr = (uint32_t *)pal_mem_alloc(sizeof(uint32_t) * (g_gic_its_info->GicNumIts));

  if (g_cwriter_ptr == NULL) {
    val_print(AVS_PRINT_ERR, "ITS : Could Not Allocate Memory CWriteR. Test may not pass.\n", 0);
    return 0;
  }

  for (index = 0; index < g_gic_its_info->GicNumIts; index++)
    g_cwriter_ptr[index] = 0;

  for (index = 0; index < g_gic_its_info->GicNumIts; index++)
  {
    /* Set Initial configuration */   // DONE
    Status = SetInitialConfiguration(index);
    if (Status)
      return Status;
  }

  /* Configure Redistributor For LPIs */
  Status = ArmGicRedistributorConfigurationForLPI(g_gic_its_info->GicDBase,
                                                  g_gic_its_info->GicRdBase);
  if (Status)
    return Status;

  for (index = 0; index < g_gic_its_info->GicNumIts; index++)
  {
    /* Set Command Queue Base */
    Status = ArmGicSetItsCommandQueueBase(index);
    if (Status)
      return Status;

    /* Set Up the ITS tables */
    Status = ArmGicSetItsTables(index);
    if (Status)
      return Status;
  }

  g_its_setup_done = 1;

  val_print(AVS_PRINT_INFO, "ITS : Info Block \n", 0);
  for (index = 0; index < g_gic_its_info->GicNumIts; index++)
  {
      val_print(AVS_PRINT_INFO, "GIC ITS Index : %x\n", index);
      val_print(AVS_PRINT_INFO, "GIC ITS ID : %x\n", g_gic_its_info->GicIts[index].ID);
      val_print(AVS_PRINT_INFO, "GIC ITS Base : %llx\n\n", g_gic_its_info->GicIts[index].Base);
  }

  return 0;
}
