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


#define TEST_NUM   (AVS_PE_TEST_NUM_BASE  +  11)
#define TEST_DESC  "Check PMU Overflow signal         "

static uint32_t int_id;

void
set_pmu_overflow()
{
  uint64_t pmcr;

  //Initializing the state of overflow status and interrupt request registers
  val_pe_reg_write(PMINTENCLR_EL1, 0xFFFFFFFF);
  val_pe_reg_write(PMOVSCLR_EL0, 0xFFFFFFFF);

  //Sequence to generate PMUIRQ
  pmcr = val_pe_reg_read(PMCR_EL0);
  val_pe_reg_write(PMCR_EL0, pmcr|0x1);

  val_pe_reg_write(PMINTENSET_EL1, 0x1);
  val_pe_reg_write(PMOVSSET_EL0, 0x1);

}


static
void
isr()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  /* We received our interrupt, so disable PMUIRQ from generating further interrupts */
  val_pe_reg_write(PMOVSCLR_EL0, 0x1);
  val_print(AVS_PRINT_INFO, "\n Received PMUIRQ ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  val_gic_end_of_interrupt(int_id);

  return;
}


static
void
payload()
{
  uint32_t timeout = 0x100000;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  int_id = val_pe_get_pmu_gsiv(index);

  /* For SBSA level 2 and above, the PPI has to be a specific value. */
  if (g_sbsa_level > 1) {
      if (int_id != 23) {
          timeout = 0;
          val_print(AVS_PRINT_ERR, "\n       Incorrect PPI value      %d       ", int_id);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
          return;
      }
  }

  val_gic_install_isr(int_id, isr);

  set_pmu_overflow();

  while ((--timeout > 0) && (IS_RESULT_PENDING(val_get_status(index))));

  if(timeout == 0)
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
}

/**
  @brief  Install ISR and verify PMU Overflow Interrupt by programming System registers
**/
uint32_t
c011_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;
  num_pe = 1;

  status = val_initialize_test(TEST_NUM, TEST_DESC, val_pe_get_num(), g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      /* execute payload on present PE and then execute on other PE */
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}

