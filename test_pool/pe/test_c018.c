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

#include "val/include/sbsa_avs_val.h"
#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_secure.h"

#define TEST_NUM   (AVS_PE_TEST_NUM_BASE  +  18)
#define TEST_DESC  "Check for PMBIRQ signal           "

#define PMSCR_VALUE   0x1
#define PMBLIMITR_VALUE(val) (val + 0x10000 + 1)

uint64_t mem_array[2048];
/* Interrupt ID for PMBIRQ for compliance level > 1 is 22,
   the test will be skipped for compliance level <= 1 */
static uint32_t int_id = 22;

uint64_t
get_interval_for_pmsirr(void)
{
  uint64_t temp, pmsidr;
  pmsidr = val_pe_reg_read(PMSIDR_EL1);
  pmsidr = ((pmsidr >> 8) & 0xf);
  temp = (1 << ((pmsidr/2) + 8));
  if(pmsidr % 2)
    temp += ((1 << ((pmsidr-3)/2))*212);

  return temp;
}

void
generate_pmbirq(uint64_t fault_addr)
{
  uint64_t read_data=0, interval;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  SBSA_SMC_t  smc;

  //SMC call to set MDCR_EL3.NSPB=3
  smc.test_index = SBSA_SECURE_PMBIRQ;
  smc.test_arg01 = 0x3;   // Value to be written to MDCR_EL3.NSPB
  val_secure_call_smc(&smc);

  val_secure_get_result(&smc, 2);
  if(smc.test_arg02 != SBSA_SMC_INIT_SIGN){
      val_print(AVS_PRINT_WARN, "\n   ARM-TF firmware not ported, skipping this test", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
      return;
  }

  read_data = val_pe_reg_read(MDCR_EL2);
  val_pe_reg_write(MDCR_EL2, read_data & (~0x300));

  interval = get_interval_for_pmsirr();
  val_pe_reg_write(PMSIRR_EL1, interval << 8);

  val_pe_reg_write(PMSCR_EL2, PMSCR_VALUE);
  val_pe_reg_write(PMSFCR_EL1, 0x0);
  val_pe_reg_write(PMBPTR_EL1, fault_addr);
  val_pe_reg_write(PMBLIMITR_EL1, PMBLIMITR_VALUE(fault_addr));

  val_pe_spe_program_under_profiling(interval,(uint64_t)mem_array);

}


static
void
pmbirq_isr()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Disable profiling */
  val_pe_spe_disable();

  val_print(AVS_PRINT_INFO, "\n Received PMBIRQ ", 0);
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  val_gic_end_of_interrupt(int_id);

  return;
}


static
void
payload()
{

  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  addr_t addr = 0x3200000;  //50 MB - randomly chosen.
  uint64_t attr,data;
  uint32_t loop_var = 2048;   //loop for 2048 times to see if we can get an unpopulated region

  /* This test is run only when Statistical Profiling Extension is supported */
  data = val_pe_reg_read(ID_AA64DFR0_EL1);

  if (((data >> 32) & 0xF) == 0) {
    val_print(AVS_PRINT_WARN, "\n       SPE not supported on this PE      ", 0);
    val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
    return;
  }

  val_gic_install_isr(int_id, pmbirq_isr);

  /* Skip the test if no un-populated memory is found */
  val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
  while (loop_var > 0) {
      if (val_memory_get_info(addr, &attr) == MEM_TYPE_NOT_POPULATED) {
          generate_pmbirq(addr);
          break;
      } else {
          addr += 0x1000000;   //check at 16MB hops
          loop_var --;
      }
  }

}

uint32_t
c018_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);

  if (status != AVS_STATUS_SKIP)
      /* execute payload on present PE and then execute on other PE */
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}

