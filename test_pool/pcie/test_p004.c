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
#include "val/include/val_interface.h"

#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_memory.h"

/* SBSA-checklist 63 & 64 */
#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 4)
#define TEST_DESC  "Check PCIe Unaligned access, Norm mem"

static
void
payload(void)
{
  uint32_t count = 0;
  uint64_t base;
  uint32_t data;
  char *baseptr;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* Map SATA Controller BARs to a NORMAL memory attribute. check unaligned access */
  count = val_peripheral_get_info(NUM_SATA, 0);

  while (count--) {
      base = val_peripheral_get_info(SATA_BASE1, count);
      baseptr = (char *)val_memory_ioremap((void *)base, 1024, 0);

      data = *(uint32_t *)(baseptr+3);

      val_memory_unmap(baseptr);
  }
   val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 0));

}

uint32_t
p004_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
