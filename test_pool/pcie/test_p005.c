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

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 5)
#define TEST_DESC  "DMA Address translations (SATA)   "

#define TEST_DATA_BLK_SIZE  512

/* For all DMA masters populated in the Info table, Verify functional DMA,
   before we proceed with other tests */
static
void
payload(void)
{

  void *orig_buffer = NULL, *new_buffer;
  void *backup;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t target_dev_index;
  uint64_t dma_addr;
  uint32_t dma_len;

  target_dev_index = val_dma_get_info(DMA_NUM_CTRL, 0);

  if (!target_dev_index) {
      val_print(AVS_PRINT_WARN, "\n       No DMA controllers detected...    ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  while (target_dev_index)
  {
      target_dev_index--; //index is zero based

      if (val_dma_get_info(DMA_HOST_COHERENT, target_dev_index) != DMA_COHERENT) {
          if (val_dma_get_info(DMA_HOST_PCI, target_dev_index) == PCI_EP) {
              val_print(AVS_PRINT_ERR, "\n       All PCIe end points must be IO-Coherent. .. ", 0);
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
              return;
          } else {
              val_print(AVS_PRINT_WARN, "\n      Controller Index = %x is not IO-Coherent. Skipping.. ",
                target_dev_index);
              if (target_dev_index)
                  continue;
              else {
                  val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
                  return;
              }
          }
      }
      val_smmu_ops(SMMU_START_MONITOR_DEV, 0, &target_dev_index, NULL);

      /* Allocate DMA-able memory region in DDR */
      orig_buffer = kzalloc(TEST_DATA_BLK_SIZE, GFP_KERNEL);
      new_buffer  = kzalloc(TEST_DATA_BLK_SIZE, GFP_KERNEL);
      backup      = kzalloc(TEST_DATA_BLK_SIZE, GFP_KERNEL);

      /* Backup the data on the disk before we override it with test data */
      val_dma_start_from_device(backup, 512, target_dev_index);


      *((uint32_t *)orig_buffer)     = 0x12345678;
      *((uint32_t *)orig_buffer + 1) = 0x1234569A;
      *((uint32_t *)orig_buffer + 2) = 0x12ABCDEF;
      *((uint32_t *)orig_buffer + 9) = 0x12ABCDEF;
      *((uint32_t *)orig_buffer + 10) = 0x12ABCDEF;
      /* Program Device DMA controller with the source buffer */
      val_dma_start_to_device(orig_buffer, TEST_DATA_BLK_SIZE, target_dev_index);

      /* READ Back from device and Verify the DDR memory has the original data */
      val_dma_start_from_device(new_buffer, TEST_DATA_BLK_SIZE, target_dev_index);

      val_print(AVS_PRINT_DEBUG, "\n new buffer = %x ", *(uint32_t *)new_buffer);
      val_print(AVS_PRINT_DEBUG, " %x ", *((uint32_t *)new_buffer + 1));
      val_print(AVS_PRINT_DEBUG, " %x \n", *((uint32_t *)new_buffer + 2));


      if (memcmp(orig_buffer, new_buffer, TEST_DATA_BLK_SIZE)) {
          val_print(AVS_PRINT_ERR, "\n        Data Compare of DMA TO and FROM Device %d - failed.",
            target_dev_index);

          kfree(orig_buffer);
          kfree(new_buffer);

          /* Restore back the original data */
          val_dma_start_to_device(backup, TEST_DATA_BLK_SIZE, target_dev_index);

          val_smmu_ops(SMMU_STOP_MONITOR_DEV, 0, &target_dev_index, NULL);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, target_dev_index));
          return;
      }

      if (val_dma_get_info(DMA_HOST_IOMMU_ATTACHED, target_dev_index) == 0) {
          /* Make sure that the DMA address used by the device is the same as what we were allocated */
          /* This is to make sure there are no additional address translations in between */
          val_dma_device_get_dma_addr(target_dev_index, &dma_addr, &dma_len);

          if (dma_addr != virt_to_phys(new_buffer))
          {
              /* Restore back the original data */
              val_dma_start_to_device(backup, TEST_DATA_BLK_SIZE, target_dev_index);

              val_print(AVS_PRINT_ERR, "\n      Device DMA addr does not match allocated address %lx ", dma_addr);
              val_print(AVS_PRINT_ERR, "\n      !=  %lx ", virt_to_phys(new_buffer));
              val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
              return;
          }

      }

      val_smmu_ops(SMMU_STOP_MONITOR_DEV, 0, &target_dev_index, NULL);

      /* Restore back the original data */
      val_dma_start_to_device(backup, TEST_DATA_BLK_SIZE, target_dev_index);

      kfree(orig_buffer);
      kfree(new_buffer);
      kfree(backup);
  }
  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 02));
}


uint32_t
p005_entry(uint32_t num_pe)
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
