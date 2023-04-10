/** @file
 * Copyright (c) 2023 Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_iovirt.h"

#include "val/include/sbsa_avs_pe.h"
#include "val/include/sbsa_avs_smmu.h"
#include "val/include/sbsa_avs_memory.h"

#define TEST_NUM   (AVS_SMMU_TEST_NUM_BASE + 16)
#define TEST_RULE  "S_L7SM_02"
#define TEST_DESC  "Check for SMMU/CATU in ETR Path       "

#define MAX_NUM_ETR 6

static
void
payload(void)
{
  uint32_t index = 0;
  uint32_t status = 0;
  uint32_t etr_count = 0;
  uint32_t smmu_found = 0;
  uint32_t num_named_comp = 0;
  char etr_path[MAX_NUM_ETR][MAX_NAMED_COMP_LENGTH];
  uint32_t i = 0;
  uint32_t j = 0;

  index = val_pe_get_index_mpid(val_pe_get_mpid());
  val_memory_set(etr_path, sizeof(etr_path), 0);

  /*Check for ETR devices using ETR using unique HID (ARMHC97C)*/
  status = val_get_device_path("ARMHC97C", etr_path);
  if (status != 0) {
    val_print(AVS_PRINT_ERR, "\n       Unable to get ETR device info from ACPI namespace", 0);
    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 1));
    return;
  }

  if ((char)etr_path[0][0] == '\0') {
    val_print(AVS_PRINT_ERR, "\n       No ETR devices are discovered                 ", 0);
    val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 1));
    return;
  } else {
    /*Counting num of ETR Devices*/
    for (i = 0; i < MAX_NUM_ETR; i++) {
      if ((char)etr_path[i][0] != '\0')
        etr_count++;
    }
  }
  val_print(AVS_PRINT_DEBUG, "\n       Num of ETR devices found etr_count: %d ", etr_count);

  num_named_comp = val_iovirt_get_named_comp_info(NUM_NAMED_COMP, 0);
  val_print(AVS_PRINT_DEBUG, "\n       NUM Named component  : %d", num_named_comp);

  /*ETR device must be behind SMMU or CATU*/
  for (i = 0; i < etr_count; i++) {
    for (j = 0; j < num_named_comp; j++) {
      smmu_found = 0;
      /* print info fields */
      val_print(AVS_PRINT_DEBUG, "\n       Named component  :", 0);
      val_print(AVS_PRINT_DEBUG,
                    (char8_t *)val_iovirt_get_named_comp_info(NAMED_COMP_DEV_OBJ_NAME, j), 0);

      /*Check the ETR and Named Componnet paths are matching*/
      if (!val_strncmp((char8_t *)val_iovirt_get_named_comp_info(NAMED_COMP_DEV_OBJ_NAME, j),
                      (char8_t *)etr_path[i], 9)) {

        /*Check Named Component with ETR path have SMMU*/
        if (val_iovirt_get_named_comp_info(NAMED_COMP_SMMU_BASE, j)) {
          smmu_found++;
          break;
        }
      }
    }

    /*SMMU not found in ETR path, check for CATU*/
    if (!smmu_found) {
      /*Check for CATU in the path of ETR device*/
      val_print(AVS_PRINT_DEBUG, "\n       SMMU not found in ETR Path at index %d", i);

      /*Check the CATU in ETR path*/
      status = val_smmu_is_etr_behind_catu((char8_t *)etr_path[i]);
      if (status == NOT_IMPLEMENTED) {
        val_print(AVS_PRINT_DEBUG,
                    "\n       val_smmu_is_etr_behind_catu API not implemented", 0);
        val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 2));
        return;
      } else if (status) {
        val_print(AVS_PRINT_DEBUG, "\n       No CATU found in ETR path at index %d", i);
        val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 2));
        return;
      }
    }

    if (!smmu_found) {
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 3));
      return;
    }
  }
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 1));
}

uint32_t
i016_entry(uint32_t num_pe)
{

  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level, TEST_RULE);
  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM), TEST_RULE);

  return status;
}
