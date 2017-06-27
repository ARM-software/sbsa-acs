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

#include "val/include/sbsa_avs_smmu.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 10)
#define TEST_DESC  "PASID support atleast 16 bits     "

#define MIN_PASID_SUPPORT (1 << 16)

static void
payload(void)
{
  int num_per = 0, num_smmu = 0, skip = 1;
  uint32_t max_pasids = 0;
  uint32_t index = val_pe_get_index_mpid (val_pe_get_mpid());

  num_per = val_peripheral_get_info(NUM_ALL, 0);
  /* For each peripheral check for PASID support */
  /* If PASID is supported, test the max number of PASIDs supported */
  for(num_per--; num_per >= 0; num_per--)
  {
     if((max_pasids = val_peripheral_get_info(MAX_PASIDS, num_per)) > 0)
     {
        skip = 0;
        if(max_pasids < MIN_PASID_SUPPORT)
        {
           val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 1));
           break;
        }
     }
  }
  if(num_per < 0)
  {
     /* For each SMMUv3 check for PASID support */
     /* If PASID is supported, test the max number of PASIDs supported */
     num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);
     for(num_smmu--; num_smmu >= 0; num_smmu--)
     {
         if(val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) == 3)
         {
             if((max_pasids = val_smmu_max_pasids(val_smmu_get_info(SMMU_CTRL_BASE, num_smmu))) > 0)
             {
                 skip = 0;
                 if(max_pasids < MIN_PASID_SUPPORT)
                 {
                    val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 2));
                    break;
                 }
             }
         }
     }
  }
  if(num_smmu < 0) {
      if(skip)
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 3));
      else
          val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 0));
  }
}

uint32_t
p010_entry(uint32_t num_pe)
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
