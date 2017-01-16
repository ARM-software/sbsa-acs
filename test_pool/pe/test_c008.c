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

#define TEST_NUM   (AVS_PE_TEST_NUM_BASE  +  8)
#define TEST_DESC  "Check Little Endian support       "

#define MMFR0_BIGEND 0xF00
#define TEST_DATA    0x11223344

static
void
payload()
{
  uint64_t data = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  data = val_pe_reg_read(ID_AA64MMFR0_EL1);
  if(data & MMFR0_BIGEND) {
  // mixed endian support present, check whether both endianness is present
  // based on functional check
      data = TEST_DATA;
      if(1 == (val_pe_bigend_check(&data)))
          val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
      else
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
  } else {
  // single endian only, the current endian should be little endian,
  // check SCTLR.EE
      data = val_pe_reg_read(SCTLR_EL2);
      if (((data >> 25) & 1) == 0) //Bit 25 must be 0
          val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 02));
      else
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
  }

  return;
}

uint32_t
c008_entry(uint32_t num_pe)
{
  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
