/** @file
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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

#include "val/common/include/acs_pcie_enumeration.h"
#include "val/sbsa/include/sbsa_acs_pcie.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_acs_gic.h"
#include "val/sbsa/include/sbsa_acs_iovirt.h"
#include "val/common/include/acs_iovirt.h"
#include "val/sbsa/include/sbsa_acs_smmu.h"
#include "val/sbsa/include/sbsa_acs_memory.h"
#include "val/sbsa/include/sbsa_acs_exerciser.h"

#define TEST_NUM   (ACS_EXERCISER_TEST_NUM_BASE + 6)
#define TEST_DESC  "RP's must support AER feature         "
#define TEST_RULE  "PCI_ER_01, PCI_ER_04"

#define ERR_CORR     0x2
#define ERR_UNCORR   0x3
#define CLEAR_STATUS 0xFFFFFFFF

static uint32_t irq_pending;
static uint32_t lpi_int_id = 0x204C;
static uint32_t mask_value;

static
void
intr_handler(void)
{
  /* Clear the interrupt pending state */
  irq_pending = 0;

  val_print(ACS_PRINT_INFO, "\n       Received MSI interrupt %x       ", lpi_int_id);
  val_gic_end_of_interrupt(lpi_int_id);
  return;
}

/* Clear all the status bits and set the mask and severity
 * @param e_bdf   - Exerciser bdf
 *        aer_offset - AER capability offset of bdf
 *        mask       - Whether error to be masked or not
 *                     0 - no mask 0xFFFFFFFF - mask all errors
 *        severity   - Whether error is fatal or non-fatal
 *                     0 - non-fatal 0xFFFFFFFF - fatal
 **/
static void
clear_status_bits(uint32_t e_bdf, uint32_t aer_offset, uint32_t mask, uint32_t severity)
{

    /* Clear all status bits of Correctable and Unconrrectable errors */
     val_pcie_write_cfg(e_bdf, aer_offset + AER_UNCORR_STATUS_OFFSET, CLEAR_STATUS);
     val_pcie_write_cfg(e_bdf, aer_offset + AER_CORR_STATUS_OFFSET, CLEAR_STATUS);

     /* Mask or UnMask all errors */
     val_pcie_write_cfg(e_bdf, aer_offset + AER_UNCORR_MASK_OFFSET, mask);
     val_pcie_write_cfg(e_bdf, aer_offset + AER_CORR_MASK_OFFSET, mask);
     val_pcie_write_cfg(e_bdf, aer_offset + AER_UNCORR_SEVR_OFFSET, severity);
}

static uint32_t
correctable_err_status_chk(uint32_t e_bdf, uint32_t aer_offset, uint32_t err_code)
{
    uint32_t erp_bdf, reg_bdf, rp_aer_offset;
    uint32_t value, err_bit;
    uint32_t pciecs_base, reg_value;
    uint32_t fail_cnt = 0;

    val_pcie_get_rootport(e_bdf, &erp_bdf);
    val_pcie_find_capability(erp_bdf, PCIE_ECAP, ECID_AER, &rp_aer_offset);
    err_bit = val_get_exerciser_err_info(err_code);

    /* Check if corresponding error bit is set */
    val_pcie_read_cfg(e_bdf, aer_offset + AER_CORR_STATUS_OFFSET, &value);
    if (!((value >> err_bit) & 0x1))
    {
        val_print(ACS_PRINT_ERR, "\n       Err bit for error not set", 0);
        fail_cnt++;
    }

    /* Check if the RP has received the corresponding error type if error is not masked */
    val_pcie_read_cfg(erp_bdf, rp_aer_offset + AER_ROOT_ERR_OFFSET, &value);
    if ((mask_value == 0) && ((value & 0x1) == 0))
    {
        val_print(ACS_PRINT_ERR, "\n       Root error status not set", 0);
        fail_cnt++;
    }

    if ((mask_value == 1) && ((value & 0x1) == 1))
    {
        val_print(ACS_PRINT_ERR, "\n       Root error status set when error is masked", 0);
        fail_cnt++;
    }

    /* Check if the Reg ID matches with the error source ID */
    val_pcie_read_cfg(erp_bdf, rp_aer_offset + AER_ROOT_ERR_SOURCE_ID, &value);
    reg_bdf = PCIE_CREATE_BDF_PACKED(e_bdf);
    if ((mask_value == 0) && ((value & AER_SOURCE_ID_MASK) != reg_bdf))
    {
        val_print(ACS_PRINT_ERR, "\n       Error source Identification failed", 0);
        fail_cnt++;
    }

    /* Check if the appropriate status bit is set in Device status register */
    val_pcie_find_capability(e_bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
    val_pcie_read_cfg(e_bdf, pciecs_base + DCTLR_OFFSET, &reg_value);
    if (!((reg_value >> DSTS_SHIFT) & 0x1))
    {
        val_print(ACS_PRINT_ERR, "\n       Device reg of EP not set %x ", reg_value);
        fail_cnt++;
    }

    /* Clear the Error status bit in the RP */
    val_pcie_write_cfg(erp_bdf, rp_aer_offset + AER_ROOT_ERR_OFFSET, 0x1);
    val_pcie_read_cfg(erp_bdf, rp_aer_offset + AER_ROOT_ERR_OFFSET, &value);
    if ((value & 0x1))
    {
        val_print(ACS_PRINT_ERR, "\n       Err bit is not cleared %x ", value);
        fail_cnt++;
    }

    if (fail_cnt)
        return 1;

    return 0;
}

static uint32_t
uncorrectable_error_chk(uint32_t e_bdf, uint32_t aer_offset, uint32_t err_code)
{
    uint32_t erp_bdf, reg_bdf, rp_aer_offset;
    uint32_t value, err_bit;
    uint32_t pciecs_base, reg_value;
    uint32_t fail_cnt = 0;

    val_pcie_get_rootport(e_bdf, &erp_bdf);
    val_pcie_find_capability(erp_bdf, PCIE_ECAP, ECID_AER, &rp_aer_offset);
    err_bit = val_get_exerciser_err_info(err_code);

    /* Check if corresponding error bit is set */
    val_pcie_read_cfg(e_bdf, aer_offset + AER_UNCORR_STATUS_OFFSET, &value);
    if (!((value >> err_bit) & 0x1))
    {
        val_print(ACS_PRINT_ERR, "\n       Err bit not set %x", value);
        fail_cnt++;
    }

    /* Check if the RP has received the corresponding error type if error is not masked */
    val_pcie_read_cfg(erp_bdf, rp_aer_offset + AER_ROOT_ERR_OFFSET, &value);
    if ((mask_value == 0) && ((value & 0x4) == 0))
    {
        val_print(ACS_PRINT_ERR, "\n       Root Error status not set", 0);
        fail_cnt++;
    }

    if ((mask_value == 1) && ((value & 0x4) == 0x4))
    {
        val_print(ACS_PRINT_ERR, "\n       Root error status set when error is masked", 0);
        fail_cnt++;
    }

    /* Check if the Reg ID matches with the error source ID */
    val_pcie_read_cfg(erp_bdf, rp_aer_offset + AER_ROOT_ERR_SOURCE_ID, &value);
    reg_bdf = PCIE_CREATE_BDF_PACKED(e_bdf);
    if ((mask_value == 0) && (((value >> AER_SOURCE_ID_SHIFT) & AER_SOURCE_ID_MASK) != reg_bdf))
    {
        val_print(ACS_PRINT_ERR, "\n       Error source Identification failed", 0);
        fail_cnt++;
    }

    /* Check if the appropriate status bit is set in Device status register */
    val_pcie_find_capability(e_bdf, PCIE_CAP, CID_PCIECS, &pciecs_base);
    val_pcie_read_cfg(e_bdf, pciecs_base + DCTLR_OFFSET, &reg_value);
    if (!((reg_value >> DSTS_SHIFT) & DS_UNCORR_MASK))
    {
        val_print(ACS_PRINT_ERR, "\n       Device reg of EP not set", 0);
        fail_cnt++;
    }

    /* Clear the Error status bit in the RP */
    val_pcie_write_cfg(erp_bdf, rp_aer_offset + AER_ROOT_ERR_OFFSET, 0x7F);
    val_pcie_read_cfg(erp_bdf, rp_aer_offset + AER_ROOT_ERR_OFFSET, &value);
    if ((value & 0x7F))
    {
        val_print(ACS_PRINT_ERR, "\n       Err bit is not cleared %x", value);
        fail_cnt++;
    }

    if (fail_cnt)
        return 1;

    return 0;

}

static
uint32_t
inject_error(uint32_t e_bdf, uint32_t instance, uint32_t aer_offset)
{
    uint32_t err_code;
    uint32_t status, value;
    uint32_t res, timeout;

    for (err_code = 0; err_code <= ERR_CNT; err_code++)
    {
        irq_pending = 1;

        status = val_exerciser_set_param(ERROR_INJECT_TYPE, err_code, 0, instance);
        value = val_exerciser_ops(INJECT_ERROR, err_code, instance);

        /*Interrupt must be generated on error detection if errors are not masked*/
        if (mask_value == 0) {
            timeout = TIMEOUT_LARGE;
            while ((--timeout > 0) && irq_pending)
            {};

            if (timeout == 0)
            {
                val_gic_free_irq(irq_pending, 0);
                val_print(ACS_PRINT_ERR,
                          "\n       Intr not trigerred on err injection bdf 0x%x", e_bdf);
                return 1;
            }
        }

        /* Check if error injected is correctable or uncorrectable*/
        if (status == ERR_CORR) {
            val_print(ACS_PRINT_INFO, "\n       Correctable error recieved", 0);
            res = correctable_err_status_chk(e_bdf, aer_offset, value);
            if (res) {
                val_print(ACS_PRINT_ERR,
                          "\n       Correctable error check failed for bdf %x", e_bdf);
                return 1;
            }
        }

        else if (status == ERR_UNCORR) {
            val_print(ACS_PRINT_INFO, "\n       UnCorrectable error recieved", 0);
            res = uncorrectable_error_chk(e_bdf, aer_offset, value);
            if (res) {
                val_print(ACS_PRINT_ERR,
                          "\n       Uncorrectable error check failed for bdf %x", e_bdf);
                return 1;
            }
        }
    }

    return 0;
}


static
void
payload(void)
{

  uint32_t instance;
  uint32_t pe_index;
  uint32_t e_bdf;
  uint32_t erp_bdf;
  uint32_t aer_offset;
  uint32_t rp_aer_offset;
  uint32_t value = 0;
  uint32_t reg_value = 0;
  uint32_t test_skip = 1;

  uint32_t status;
  uint32_t device_id = 0;
  uint32_t stream_id = 0;
  uint32_t its_id = 0;
  uint32_t msi_index = 0;
  uint32_t msi_cap_offset = 0;
  uint32_t dpc_cap_base = 0;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  instance = val_exerciser_get_info(EXERCISER_NUM_CARDS);

  while (instance-- != 0) {

      /* if init fail moves to next exerciser */
      if (val_exerciser_init(instance))
          continue;

     e_bdf = val_exerciser_get_bdf(instance);
     val_print(ACS_PRINT_DEBUG, "\n       Exerciser BDF - 0x%x", e_bdf);

     val_pcie_enable_eru(e_bdf);
     if (val_pcie_get_rootport(e_bdf, &erp_bdf))
         continue;

     val_pcie_enable_eru(erp_bdf);

     /*Check AER capability for exerciser and its RP */
      if (val_pcie_find_capability(e_bdf, PCIE_ECAP, ECID_AER, &aer_offset) != PCIE_SUCCESS) {
          val_print(ACS_PRINT_ERR, "\n       No AER Capability, Skipping for Bdf : 0x%x", e_bdf);
          continue;
      }

      if (val_pcie_find_capability(erp_bdf, PCIE_ECAP, ECID_AER, &rp_aer_offset) != PCIE_SUCCESS) {
          val_print(ACS_PRINT_ERR, "\n       AER Capability not supported for RP : 0x%x", erp_bdf);
          val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
          return;
      }

      /* Check DPC capability */
      status = val_pcie_find_capability(erp_bdf, PCIE_ECAP, ECID_DPC, &dpc_cap_base);
      if (status == PCIE_CAP_NOT_FOUND)
      {
          val_print(ACS_PRINT_ERR, "\n       ECID_DPC not found", 0);
          val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
          return;
      }

      /* Warn if DPC enabled */
      val_pcie_read_cfg(erp_bdf, dpc_cap_base + DPC_CTRL_OFFSET, &reg_value);
      if ((reg_value & 0x3) != 0)
          val_print(ACS_PRINT_WARN, "\n       DPC enabled for bdf : 0x%x", erp_bdf);


      /* Search for MSI-X Capability */
      if (val_pcie_find_capability(e_bdf, PCIE_CAP, CID_MSIX, &msi_cap_offset)) {
          val_print(ACS_PRINT_DEBUG, "\n       No MSI-X Capability, Skipping for Bdf 0x%x", e_bdf);
          continue;
      }

      if (val_pcie_find_capability(erp_bdf, PCIE_CAP, CID_MSIX, &msi_cap_offset)) {
          val_print(ACS_PRINT_DEBUG, "\n       No MSI-X Capability for RP Bdf 0x%x", erp_bdf);
          continue;
      }

      /* Get DeviceID & ITS_ID for this device */
      status = val_iovirt_get_device_info(PCIE_CREATE_BDF_PACKED(erp_bdf),
                                        PCIE_EXTRACT_BDF_SEG(erp_bdf), &device_id,
                                        &stream_id, &its_id);

      if (status) {
          val_print(ACS_PRINT_ERR, "\n       iovirt_get_device failed for bdf 0x%x", e_bdf);
          val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 01));
          return;
      }

      /* MSI assignment */
      status = val_gic_request_msi(erp_bdf, device_id, its_id, lpi_int_id + instance, msi_index);
      if (status) {
          val_print(ACS_PRINT_ERR, "\n       MSI Assignment failed for bdf : 0x%x", erp_bdf);
          val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 02));
          return;
      }

      status = val_gic_install_isr(lpi_int_id + instance, intr_handler);

      if (status) {
          val_print(ACS_PRINT_ERR, "\n       Intr handler registration failed: 0x%x", lpi_int_id);
          val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 02));
          return;
      }

      test_skip = 0;
      val_pcie_find_capability(erp_bdf, PCIE_ECAP, ECID_AER, &rp_aer_offset);
      val_pcie_read_cfg(erp_bdf, rp_aer_offset + AER_ROOT_ERR_CMD_OFFSET, &value);
      val_pcie_write_cfg(erp_bdf, rp_aer_offset + AER_ROOT_ERR_CMD_OFFSET, (value | 0x7));

      /* Errors not masked and severity is non-fatal */
      mask_value = 0;
      clear_status_bits(e_bdf, aer_offset, 0, 0);
      if (inject_error(e_bdf, instance, aer_offset))
      {
          val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 03));
          return;
      }

      /* Errors masked and severity is non-fatal */
      mask_value = 1;
      clear_status_bits(e_bdf, aer_offset, AER_ERROR_MASK, 0);
      if (inject_error(e_bdf, instance, aer_offset))
      {
          val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 04));
          return;
      }
      mask_value = 0;

      /* Errors not masked and severity is fatal */
      clear_status_bits(e_bdf, aer_offset, 0, AER_UNCORR_SEVR_FATAL);
      if (inject_error(e_bdf, instance, aer_offset))
      {
          val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 05));
          return;
      }

      /* Disable error reporting of Exerciser and upstream Root Port */
      val_pcie_disable_eru(e_bdf);
      val_pcie_disable_eru(erp_bdf);

      /*
       * Clear unsupported request detected bit in Exerciser upstream
       * Rootport's Device Status Register to clear any pending urd status.
       */
      val_pcie_clear_urd(erp_bdf);
      val_gic_free_msi(erp_bdf, device_id, its_id, lpi_int_id + instance, msi_index);
  }

  if (test_skip == 1)
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 01));
  else
      val_set_status(pe_index, RESULT_PASS(TEST_NUM, 01));

  return;

}

uint32_t
e006_entry(void)
{
  uint32_t num_pe = 1;
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* Get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
