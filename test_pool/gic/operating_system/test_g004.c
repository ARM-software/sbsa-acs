/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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

#include "val/common/include/acs_val.h"
#include "val/sbsa/include/sbsa_val_interface.h"

#include "val/sbsa/include/sbsa_acs_gic.h"

#define TEST_NUM   (ACS_GIC_TEST_NUM_BASE + 4)
#define TEST_RULE  "S_L8GI_01"
#define TEST_DESC  "Check GICv4.1 or higher compliant     "

static
void
payload(void)
{

  uint32_t gic_version;
  uint32_t num_gicr_rd = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t gicrd_base, gicrd_rvpeid;
  uint32_t gicrd_length, i;

  if (g_sbsa_level < 8) {
      val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
      return;
  }

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  val_print(ACS_PRINT_INFO, "\n       Received GIC Major version = %4d      ", gic_version);


  /* Check the Major Version of GIC */
  if (gic_version < 4) {
      val_print(ACS_PRINT_ERR,
                "\n       Expected GICv4 or higher major version", gic_version);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
      return;
  }

  num_gicr_rd = val_gic_get_info(GIC_INFO_NUM_GICR_GICRD);
  val_print(ACS_PRINT_INFO, "\n       Redistributor count: %llx", num_gicr_rd);

  for (i = 0; i < num_gicr_rd; i++)
  {
      gicrd_base = val_get_gicr_base(&gicrd_length, i);
      if (gicrd_base == 0) {
          val_print(ACS_PRINT_INFO, "\n       Invalid gicrd Base Address: %llx", gicrd_base);
          continue;
      }

      /* GICR_TYPER.RVPEID[7] == 0x1 indicates gic is v4.1 complaint */
      gicrd_rvpeid = (val_mmio_read64(gicrd_base + GICR_TYPER) >> 7) & 0x1;

      if (gicrd_rvpeid == 0x1)
      {
        val_print(ACS_PRINT_INFO,
                "\n       Interrupt controller is compliant with Gicv4.1 or higher", 0);
        val_set_status(index, RESULT_PASS(TEST_NUM, 01));
        return;
      }
  }

  val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
  return;
}

uint32_t
g004_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This GIC test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
