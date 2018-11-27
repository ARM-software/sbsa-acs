/** @file
 * Copyright (c) 2016-2018, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/sbsa_avs_memory.h"
#include "val/include/sbsa_avs_exerciser.h"

#include "val/include/sbsa_avs_pcie.h"
#include "val/include/sbsa_avs_pcie_enumeration.h"

#define TEST_NUM   (AVS_PCIE_TEST_NUM_BASE + 15)
#define TEST_DESC  "PCIe No Snoop transaction attr    "
#define TEST_DATA_BLK_SIZE  512
#define TEST_DATA 0xDE

#define MEM_ATTR_CACHEABLE_SHAREABLE 0
#define MEM_ATTR_NON_CACHEABLE 1

void init_source_buf_data(void *buf, uint32_t size)
{

  uint32_t index;

  for (index = 0; index < size; index++) {
    *((char8_t *)buf + index) = TEST_DATA;
  }

}

#define VALUE1 0xDEADDEAD
#define VALUE2 0xDEADDEAF

static
void
payload (void)
{

  uint32_t index;
  uint32_t count;
  uint32_t no_snoop_set;
  uint32_t status;
  uint64_t dev_bdf;
  uint32_t dev_type;
  uint32_t instance;
  uint32_t dma_len;
  void *source_buf;
  void *dest_buf;
  exerciser_data_t e_data;

  status = 0;
  no_snoop_set = 0;
  index = val_pe_get_index_mpid (val_pe_get_mpid());
  count = val_peripheral_get_info (NUM_ALL, 0);

  if(!count) {
     val_set_status (index, RESULT_SKIP (g_sbsa_level, TEST_NUM, 3));
     return;
  }

  /* Read No Snoop bit from the device control register */
  while (count > 0) {
    count--;
    dev_bdf = val_peripheral_get_info (ANY_BDF, count);
    dev_type = val_pcie_get_device_type(dev_bdf);
    /* Skipping snoop bit check for type-1 and type-2 headers */
    if ((!dev_type) || (dev_type > 1))
        continue;

    if (val_pcie_get_dma_support (dev_bdf) == 1) {
      val_print (AVS_PRINT_INFO, "    have DMA support on %X", dev_bdf);
      if (val_pcie_get_dma_coherent(dev_bdf) == 1) {
        val_print (AVS_PRINT_INFO, "    DMA is coherent on %X", dev_bdf);
        status = val_pcie_get_snoop_bit (dev_bdf);
        if (status != 2) {
          no_snoop_set |= status;
          val_print (AVS_PRINT_INFO, "    no snoop bit is %d", status);
        }
      } else {
        val_print (AVS_PRINT_INFO, "    DMA is not coherent on %X", dev_bdf);
      }
    }
  }

  if(no_snoop_set) {
    val_print (AVS_PRINT_ERR, "\n       PCIe no snoop bit set to %d for a device with coherent DMA", no_snoop_set);
    val_set_status (index, RESULT_FAIL (g_sbsa_level, TEST_NUM, 1));
  } else {
    val_set_status (index, RESULT_PASS (g_sbsa_level, TEST_NUM, status));
  }

    /* Read the number of excerciser cards */
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  if (instance == 0) {
      val_print(AVS_PRINT_INFO, "    No exerciser cards in the system %x", 0);
      return;
  }

  while (instance-- != 0) {

    /*
     * Set the buf size required in a non-cacheable DDR memory region.
     * The test expects PAL to allocate a buffer of TEST_DATA_BLK_SIZE.
     */
    e_data.nc_ddr.size = TEST_DATA_BLK_SIZE;
    e_data.nc_ddr.phy_addr = NULL;
    e_data.nc_ddr.virt_addr = NULL;

    /* Get a non-caheable DDR Buffer of size indicated above */
    if (val_exerciser_get_data(EXERCISER_DATA_NC_DDR_SPACE, &e_data, instance)) {
      val_print(AVS_PRINT_ERR, "\n      DDR memory allocation failure for inst %4x", instance);
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
      return;
    }

    /* Check if the buffer parameters are proper */
    if (e_data.nc_ddr.virt_addr == NULL) {
      val_print(AVS_PRINT_ERR, "\n      Unexpected DDR region for exerciser %4x", instance);
      val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
      return;
    }

    /* Program exerciser to start sending TLPs  with No Snoop attribute header.
     * This includes setting Enable No snoop bit in exerciser Control register.
     */
    if(val_exerciser_ops(NO_SNOOP_TLP_START, 0, instance)) {
      val_print(AVS_PRINT_ERR, "\n       Exerciser %x No Snoop enable error", instance);
      goto coherence_test_failure;
    }

    /* Set virtual addres for the test buffer */
    source_buf = e_data.nc_ddr.virt_addr;
    dest_buf = source_buf + (TEST_DATA_BLK_SIZE / 2);
    dma_len = TEST_DATA_BLK_SIZE / 2;

    /* Initialize source buffer with test specific data */
    init_source_buf_data(source_buf, dma_len);

    /* Program Exerciser DMA controller with the source buffer information */
    val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)source_buf, dma_len, instance);
    if (val_exerciser_ops(START_DMA, EDMA_TO_DEVICE, instance)) {
      val_print(AVS_PRINT_ERR, "\n      DMA write failure to exerciser %4x", instance);
      goto coherence_test_failure;
    }

    /* READ Back from Exerciser to validate above DMA write */
    val_exerciser_set_param(DMA_ATTRIBUTES, (uint64_t)dest_buf, dma_len, instance);
    if (val_exerciser_ops(START_DMA, EDMA_FROM_DEVICE, instance)) {
      val_print(AVS_PRINT_ERR, "\n      DMA read failure from exerciser %4x", instance);
      goto coherence_test_failure;
    }

    if (memcmp(source_buf, dest_buf, dma_len)) {
      val_print(AVS_PRINT_ERR, "\n        SW corency failure with no snoop for Exerciser %4x", instance);
      goto coherence_test_failure;
    }

    /* Stop exerciser sending TLPs with No Snoop attribute header */
    if (val_exerciser_ops(NO_SNOOP_TLP_STOP, 0, instance)) {
        val_print(AVS_PRINT_ERR, "\n       Exerciser %x No snoop TLP disable error", instance);
        goto coherence_test_failure;
    }

}

  val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 0));
  return;

coherence_test_failure:
  val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
  return;
}

uint32_t
p015_entry (uint32_t num_pe)
{
  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test (TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);
  if (status != AVS_STATUS_SKIP) {
      val_run_test_payload (TEST_NUM, num_pe, payload, 0);
  }

  /* get the result from all PE and check for failure */
  status = val_check_for_error (TEST_NUM, num_pe);

  val_report_status (0, SBSA_AVS_END (g_sbsa_level, TEST_NUM));

  return status;
}
