/** @file
 * Copyright (c) 2018, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/sbsa_avs_val.h"
#include "val/include/val_interface.h"

#include "val/include/sbsa_avs_pcie_enumeration.h"
#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_exerciser.h"

#define TEST_NUM   (AVS_EXERCISER_TEST_NUM_BASE + 1)
#define TEST_DESC  "Enhanced ECAM Memory access check "


static
void
payload(void)
{

  uint32_t pe_index;
  uint32_t data;
  uint32_t bdf;
  uint32_t start_bus;
  uint32_t start_segment;
  uint32_t start_bdf;
  uint32_t instance;
  uint32_t reg_index;
  exerciser_data_t e_data;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  /* Set start_bdf segment and bus numbers to 1st ecam region values */
  start_segment = val_pcie_get_info(PCIE_INFO_SEGMENT, 0);
  start_bus = val_pcie_get_info(PCIE_INFO_START_BUS, 0);
  start_bdf = PCIE_CREATE_BDF(start_segment, start_bus, 0, 0);

  while (instance-- != 0) {

    if (val_exerciser_get_data(EXERCISER_DATA_CFG_SPACE, &e_data, instance)) {
        val_print(AVS_PRINT_ERR, "\n      Exerciser %d data read error     ", instance);
        goto check_fail;
    }

    bdf = val_pcie_get_bdf(EXERCISER_CLASSCODE, start_bdf);
    start_bdf = val_pcie_increment_bdf(bdf);

    /* Check ECAM config register read/write */
    for (reg_index = 0; reg_index < TEST_REG_COUNT; reg_index++) {

        if (e_data.cfg_space.reg[reg_index].attribute == ACCESS_TYPE_RW) {
            val_pcie_write_cfg(bdf, e_data.cfg_space.reg[reg_index].offset, e_data.cfg_space.reg[reg_index].value);
        }

        if (val_pcie_read_cfg(bdf, e_data.cfg_space.reg[reg_index].offset, &data) == PCIE_READ_ERR) {
            val_print(AVS_PRINT_ERR, "\n      Exerciser %d cfg reg read error  ", instance);
            goto check_fail;
        }

        if (data != e_data.cfg_space.reg[reg_index].value) {
            val_print(AVS_PRINT_ERR, "\n      Exerciser cfg reg read write mismatch %d   ", data);
            goto check_fail;
        }

    }

  }

  val_set_status(pe_index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  return;

check_fail:
  val_set_status(pe_index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
  return;

}

uint32_t
e001_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = AVS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
