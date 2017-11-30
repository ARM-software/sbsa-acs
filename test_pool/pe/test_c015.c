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

#include "val/include/sbsa_avs_val.h"
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM    (AVS_PE_TEST_NUM_BASE  +  15)
#define TEST_DESC  "Check Arch symmetry across PE     "

#define NUM_OF_REGISTERS  32

#define RAS               1
#define SPE               2
#define LOR               3
#define AA32              4

#define MASK_AA64MMFR0    0xF
#define MASK_MIDR         0x00F0FFFF
#define MASK_MPIDR        0xFF3FFFFFFF
#define MASK_CTR          0xC000
#define MASK_CCSIDR       0xFFFFFF8
#define MASK_PMCR         0xFFFF

#define MAX_CACHE_LEVEL   7

uint64_t rd_data_array[NUM_OF_REGISTERS];
uint64_t cache_list[MAX_CACHE_LEVEL];

typedef struct{
    uint32_t reg_name;
    uint64_t reg_mask;
    char     reg_desc[30];
    uint8_t  dependency;
}reg_details;

reg_details reg_list[] = {
    {CCSIDR_EL1,       MASK_CCSIDR,    "CCSIDR_EL1"      , 0x0 },
    {ID_AA64PFR0_EL1,  0x0,            "ID_AA64PFR0_EL1" , 0x0 },
    {ID_AA64PFR1_EL1,  0x0,            "ID_AA64PFR1_EL1" , 0x0 },
    {ID_AA64DFR0_EL1,  0x0,            "ID_AA64DFR0_EL1" , 0x0 },
    {ID_AA64DFR1_EL1,  0x0,            "ID_AA64DFR1_EL1" , 0x0 },
    {ID_AA64MMFR0_EL1, MASK_AA64MMFR0, "ID_AA64MMFR0_EL1", 0x0 },
    {ID_AA64MMFR1_EL1, 0x0,            "ID_AA64MMFR1_EL1", 0x0 },
    //{ID_AA64MMFR2_EL1, 0x0,            "ID_AA64MMFR2_EL1", 0x0 },
    {CTR_EL0,          MASK_CTR,       "CTR_EL0"         , 0x0 },
    {ID_AA64ISAR0_EL1, 0x0,            "ID_AA64ISAR0_EL1", 0x0 },
    {ID_AA64ISAR1_EL1, 0x0,            "ID_AA64ISAR1_EL1", 0x0 },
    {MPIDR_EL1,        MASK_MPIDR,     "MPIDR_EL1"       , 0x0 },
    {MIDR_EL1,         MASK_MIDR,      "MIDR_EL1"        , 0x0 },
    {ID_DFR0_EL1,      0x0,            "ID_DFR0_EL1"     , AA32},
    {ID_ISAR0_EL1,     0x0,            "ID_ISAR0_EL1"    , AA32},
    {ID_ISAR1_EL1,     0x0,            "ID_ISAR1_EL1"    , AA32},
    {ID_ISAR2_EL1,     0x0,            "ID_ISAR2_EL1"    , AA32},
    {ID_ISAR3_EL1,     0x0,            "ID_ISAR3_EL1"    , AA32},
    {ID_ISAR4_EL1,     0x0,            "ID_ISAR4_EL1"    , AA32},
    {ID_ISAR5_EL1,     0x0,            "ID_ISAR5_EL1"    , AA32},
    {ID_MMFR0_EL1,     0x0,            "ID_MMFR0_EL1"    , AA32},
    {ID_MMFR1_EL1,     0x0,            "ID_MMFR1_EL1"    , AA32},
    {ID_MMFR2_EL1,     0x0,            "ID_MMFR2_EL1"    , AA32},
    {ID_MMFR3_EL1,     0x0,            "ID_MMFR3_EL1"    , AA32},
    {ID_MMFR4_EL1,     0x0,            "ID_MMFR4_EL1"    , AA32},
    {ID_PFR0_EL1,      0x0,            "ID_PFR0_EL1"     , AA32},
    {ID_PFR1_EL1,      0x0,            "ID_PFR1_EL1"     , AA32},
    {MVFR0_EL1,        0x0,            "MVFR0_EL1"       , AA32},
    {MVFR1_EL1,        0x0,            "MVFR1_EL1"       , AA32},
    {MVFR2_EL1,        0x0,            "MVFR2_EL1"       , AA32},
    {PMCEID0_EL0,      0x0,            "PMCEID0_EL0"     , 0x0 },
    {PMCEID1_EL0,      0x0,            "PMCEID1_EL0"     , 0x0 },
    {PMCR_EL0,         MASK_PMCR,      "PMCR_EL0"        , 0x0 },
    {PMBIDR_EL1,       0x0,            "PMBIDR_EL1"      , SPE },
    {PMSIDR_EL1,       0x0,            "PMSIDR_EL1"      , SPE },
    {ERRIDR_EL1,       0x0,            "ERRIDR_EL1"      , RAS },
    {ERR0FR_EL1,       0x0,            "ERR0FR_EL1"      , RAS },
    {ERR1FR_EL1,       0x0,            "ERR1FR_EL1"      , RAS },
    {ERR2FR_EL1,       0x0,            "ERR2FR_EL1"      , RAS },
    {ERR3FR_EL1,       0x0,            "ERR3FR_EL1"      , RAS },
    {LORID_EL1,        0x0,            "LORID_EL1"       , LOR }
};

uint64_t
return_reg_value(uint32_t reg, uint8_t dependency)
{
  uint64_t temp=0;

  if(dependency == 0)
      return val_pe_reg_read(reg);

  switch(dependency)
  {
    case RAS: // If RAS is not supported, then skip register check
        temp = val_pe_reg_read(ID_AA64PFR0_EL1);
        temp = (temp >> 28) & 0xf;
        if(temp == 1)
            return val_pe_reg_read(reg);
        else
            return 0;
        break;

    case SPE: // If Statistical Profiling Extension is not supported, then skip register check
        temp = val_pe_reg_read(ID_AA64DFR0_EL1);
        temp = (temp >> 32) & 0xf;
        if(temp == 1)
            return val_pe_reg_read(reg);
        else
            return 0;
        break;

    case LOR: // If Limited Ordering Region is not supported, then skip register check
        temp = val_pe_reg_read(ID_AA64MMFR1_EL1);
        temp = (temp >> 16) & 0xf;
        if(temp == 1)
            return val_pe_reg_read(reg);
        else
            return 0;
        break;

    case AA32: // If the register is UNK in pure AArch64 implementation, then skip register check
        temp = val_pe_reg_read(ID_AA64PFR0_EL1);
        temp = (temp & 1);
        if(temp == 0)
            return val_pe_reg_read(reg);
        else
            return 0;
        break;

    default:
        val_print(AVS_PRINT_ERR, "\n Unknown dependency = %d ", dependency);
        return 0x0;
  }

}

void
id_regs_check(void)
{
  uint64_t reg_read_data;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t i = 0;

  /* Loop CLIDR to check if a cache level is implemented before comparing */
  while (i < MAX_CACHE_LEVEL) {
      reg_read_data = val_pe_reg_read(CLIDR_EL1);
      if (reg_read_data & ((0x7) << (i * 3))) {
          /* Select the correct cache in csselr register */
          val_pe_reg_write(CSSELR_EL1, i << 1);
          reg_read_data = return_reg_value(reg_list[0].reg_name, reg_list[i].dependency);

          if ((reg_read_data & (~reg_list[0].reg_mask)) != (cache_list[i] & (~reg_list[0].reg_mask))) {
              val_set_test_data(index, (reg_read_data & (~reg_list[i].reg_mask)), 0);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
              return;
          }
      }
      i++;
  }

  for (i = 1; i < NUM_OF_REGISTERS; i++) {
      reg_read_data = return_reg_value(reg_list[i].reg_name, reg_list[i].dependency);

      if((reg_read_data & (~reg_list[i].reg_mask)) != (rd_data_array[i] & (~reg_list[i].reg_mask)))
      {
          val_set_test_data(index, (reg_read_data & (~reg_list[i].reg_mask)), i);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          return;
      }
      reg_read_data = 0;
  }
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));

  return;
}

static
void
payload(uint32_t num_pe)
{
  uint32_t my_index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t i;
  uint32_t timeout;
  uint64_t reg_read_data, debug_data=0, array_index=0;

  if (num_pe == 1) {
      val_print(AVS_PRINT_WARN, "\n       Skipping as num of PE is 1        ", 0);
      val_set_status(my_index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  /* Loop CLIDR to check if a cache level is implemented */
  i = 0;
  while (i < MAX_CACHE_LEVEL) {
      reg_read_data = val_pe_reg_read(CLIDR_EL1);
      if (reg_read_data & ((0x7) << (i * 3))) {
         /* Select the correct cache level in csselr register */
         val_pe_reg_write(CSSELR_EL1, i << 1);
         cache_list[i] = return_reg_value(reg_list[0].reg_name, reg_list[i].dependency);
         val_print(AVS_PRINT_INFO, "\n      cache size read is %x ", cache_list[i]);
      }
      i++;
  }

  for (i = 1; i < NUM_OF_REGISTERS; i++) {
      rd_data_array[i] = return_reg_value(reg_list[i].reg_name, reg_list[i].dependency);
      val_data_cache_ops_by_va((addr_t)(rd_data_array + i), CLEAN_AND_INVALIDATE);
  }

  for (i = 0; i < num_pe; i++) {
      if (i != my_index) {
          timeout=TIMEOUT_LARGE;
          val_execute_on_pe(i, id_regs_check, 0);
          while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

          if(timeout == 0) {
              val_print(AVS_PRINT_ERR, "\n       **Timed out** for PE index = %d", i);
              val_set_status(i, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
              return;
          }

          if(IS_TEST_FAIL(val_get_status(i))) {
              val_get_test_data(i, &debug_data, &array_index);
              val_print(AVS_PRINT_ERR, "\n       Reg compare failed for PE index=%d for Register: ", i);
              val_print(AVS_PRINT_ERR, reg_list[array_index].reg_desc, 0);
              val_print(AVS_PRINT_ERR, "\n       Current PE value = 0x%llx", rd_data_array[array_index] & (~reg_list[array_index].reg_mask));
              val_print(AVS_PRINT_ERR, "         Other PE value = 0x%llx", debug_data);
              return;
          }
      }
  }

  return;

}

uint32_t
c015_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, val_pe_get_num(), g_sbsa_level);

  if (status != AVS_STATUS_SKIP)
  /* execute payload, which will execute relevant functions on current and other PEs */
      payload(num_pe);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}

