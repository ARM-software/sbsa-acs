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

#include "val/include/sbsa_avs_dma.h"
#include "val/include/sbsa_avs_smmu.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 14)
#define TEST_DESC  "Memory attributes of DMA traffic  "


/* For all DMA masters populated in the Info table, which are behind an SMMU,
   verify there are no additional translations before address is given to SMMU */
static
void
payload(void)
{

  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t target_dev_index;
  uint32_t status = 0;
  addr_t   dma_addr = 0;
  void *buffer;
  uint32_t attr, sh;
  int ret;

  target_dev_index = val_dma_get_info(DMA_NUM_CTRL, 0);

  if (!target_dev_index)
  {
      val_print(AVS_PRINT_TEST, "\n       No DMA controllers detected...    ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 03));
      return;
  }

  while (target_dev_index)
  {
      target_dev_index--; //index is zero based
      if(val_dma_get_info(DMA_HOST_COHERENT, target_dev_index))
      {
          dma_addr = val_dma_mem_alloc(&buffer, 512, target_dev_index, DMA_COHERENT);
          ret = val_dma_mem_get_attrs(buffer, &attr, &sh);
          if(ret)
          {
              val_print(AVS_PRINT_ERR, "\n     DMA controler %d: Failed to get memory attributes\n", target_dev_index);
              status = 1;
              continue;
          }
          if(!(MEM_NORMAL_WB_IN_OUT(attr) && MEM_SH_INNER(sh)))
          {
              val_print(AVS_PRINT_INFO, "\n    DMA controler %d: IO Coherent DMA memory should be inner/outer writeback, inner shareable\n", target_dev_index);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
              status = 1;
          }
      }
      else {
          dma_addr = val_dma_mem_alloc(&buffer, 512, target_dev_index, DMA_NOT_COHERENT);
          ret = val_dma_mem_get_attrs(buffer, &attr, &sh);
          if(ret)
          {
              val_print(AVS_PRINT_ERR, "\n     DMA controler %d: Failed to get memory attributes\n", target_dev_index);
              status = 1;
              continue;
          }
          if(!((MEM_NORMAL_WB_IN_OUT(attr) && MEM_SH_INNER(sh)) ||
                MEM_NORMAL_NC_IN_OUT(attr) ||
                MEM_DEVICE(attr)))
          {
              val_print(AVS_PRINT_INFO,
              "\n     DMA controler %d: DMA memory should be inner/outer writeback inner shareable, inner/outer non-cacheable, or device type\n",
              target_dev_index);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
              status = 1;
          }
      }
  }
  if(!status)
      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 0));
}


uint32_t
p014_entry(uint32_t num_pe)
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
