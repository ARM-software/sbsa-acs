/** @file
 * Copyright (c) 2016-2023 Arm Limited or its affiliates. All rights reserved.
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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_exerciser.h"
#include "include/sbsa_avs_pcie.h"
#include "include/sbsa_avs_smmu.h"

EXERCISER_INFO_TABLE g_exercier_info_table;
extern uint32_t pcie_bdf_table_list_flag;

/**
  @brief   This API popultaes information from all the PCIe stimulus generation IP available
           in the system into exerciser_info_table structure
  @param   exerciser_info_table - Table pointer to be filled by this API
  @return  exerciser_info_table - Contains info to communicate with stimulus generation hardware
**/
uint32_t val_exerciser_create_info_table(void)
{
  uint32_t num_ecam;
  uint32_t seg_num;
  uint32_t start_bus;
  uint32_t end_bus;
  uint32_t bus_index;
  uint32_t dev_index;
  uint32_t func_index;
  uint32_t ecam_index;
  uint32_t bdf;
  uint32_t reg_value;
  uint32_t cid_offset;
  uint32_t dp_type;

  num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);
  if (num_ecam == 0)
  {
      val_print(AVS_PRINT_ERR, "\n       No ECAMs discovered              ", 0);
      return 1;
  }

  for (ecam_index = 0; ecam_index < num_ecam; ecam_index++)
  {
      /* Derive ecam specific information */
      seg_num = val_pcie_get_info(PCIE_INFO_SEGMENT, ecam_index);
      start_bus = val_pcie_get_info(PCIE_INFO_START_BUS, ecam_index);
      end_bus = val_pcie_get_info(PCIE_INFO_END_BUS, ecam_index);

      /* Iterate over all buses, devices and functions in this ecam */
      for (bus_index = start_bus; bus_index <= end_bus; bus_index++)
      {
          for (dev_index = 0; dev_index < PCIE_MAX_DEV; dev_index++)
          {
              for (func_index = 0; func_index < PCIE_MAX_FUNC; func_index++)
              {
                  /* Form bdf using seg, bus, device, function numbers */
                  bdf = PCIE_CREATE_BDF(seg_num, bus_index, dev_index, func_index);

                  /* Probe pcie device Function with this bdf */
                  if (val_pcie_read_cfg(bdf, TYPE01_VIDR, &reg_value) == PCIE_NO_MAPPING)
                  {
                      /* Return if there is a bdf mapping issue */
                      val_print(AVS_PRINT_ERR, "\n       BDF 0x%x mapping issue", bdf);
                      return 1;
                  }

                  /* Store the Function's BDF if there was a valid response */
                  if (reg_value != PCIE_UNKNOWN_RESPONSE)
                  {
                      /* Skip if the device is a host bridge */
                      if (val_pcie_is_host_bridge(bdf))
                          continue;

                      /* Skip if the device is a PCI legacy device */
                      if (val_pcie_find_capability(bdf, PCIE_CAP, CID_PCIECS,  &cid_offset)
                                                                           != PCIE_SUCCESS)
                          continue;

                      dp_type = val_pcie_device_port_type(bdf);
                      val_print(AVS_PRINT_INFO, "\n       dp_type 0x%x ", dp_type);

                      /* Store the Function's BDF if there was a valid response */
                      if (pal_is_bdf_exerciser(bdf))
                      {
                          g_exercier_info_table.e_info[g_exercier_info_table.num_exerciser].bdf
                                                                                          = bdf;
                          g_exercier_info_table.e_info[g_exercier_info_table.num_exerciser++]
                                                                               .initialized = 0;
                          val_print(AVS_PRINT_DEBUG, "    exerciser Bdf %x\n", bdf);
                      }

                  }
              }
          }
      }
  }

  val_print(AVS_PRINT_ERR, " PCIE_INFO: Number of exerciser cards : %4d \n",
                                                             g_exercier_info_table.num_exerciser);
  return 0;
}

uint32_t val_get_exerciser_err_info(EXERCISER_ERROR_CODE type)
{
    switch (type) {
    case CORR_RCVR_ERR:
         return CORR_RCVR_ERR_OFFSET;
    case CORR_BAD_TLP:
         return CORR_BAD_TLP_OFFSET;
    case CORR_BAD_DLLP:
         return CORR_BAD_DLLP_OFFSET;
    case CORR_RPL_NUM_ROLL:
         return CORR_RPL_NUM_ROLL_OFFSET;
    case CORR_RPL_TMR_TIMEOUT:
         return CORR_RPL_TMR_TIMEOUT_OFFSET;
    case CORR_ADV_NF_ERR:
         return CORR_ADV_NF_ERR_OFFSET;
    case CORR_INT_ERR:
         return CORR_INT_ERR_OFFSET;
    case CORR_HDR_LOG_OVRFL:
         return CORR_HDR_LOG_OVRFL_OFFSET;
    case UNCORR_DL_ERROR:
         return UNCORR_DL_ERROR_OFFSET;
    case UNCORR_SD_ERROR:
         return UNCORR_SD_ERROR_OFFSET;
    case UNCORR_PTLP_REC:
         return UNCORR_PTLP_REC_OFFSET;
    case UNCORR_FL_CTRL_ERR:
         return UNCORR_FL_CTRL_ERR_OFFSET;
    case UNCORR_CMPT_TO:
         return UNCORR_CMPT_TO_OFFSET;
    case UNCORR_AMPT_ABORT:
         return UNCORR_AMPT_ABORT_OFFSET;
    case UNCORR_UNEXP_CMPT:
         return UNCORR_UNEXP_CMPT_OFFSET;
    case UNCORR_RCVR_ERR:
         return UNCORR_RCVR_ERR_OFFSET;
    case UNCORR_MAL_TLP:
         return UNCORR_MAL_TLP_OFFSET;
    case UNCORR_ECRC_ERR:
         return UNCORR_ECRC_ERR_OFFSET;
    case UNCORR_UR:
         return UNCORR_UR_OFFSET;
    case UNCORR_ACS_VIOL:
         return UNCORR_ACS_VIOL_OFFSET;
    case UNCORR_INT_ERR:
         return UNCORR_INT_ERR_OFFSET;
    case UNCORR_MC_BLK_TLP:
         return UNCORR_MC_BLK_TLP_OFFSET;
    case UNCORR_ATOP_EGR_BLK:
         return UNCORR_ATOP_EGR_BLK_OFFSET;
    case UNCORR_TLP_PFX_EGR_BLK:
         return UNCORR_TLP_PFX_EGR_BLK_OFFSET;
    case UNCORR_PTLP_EGR_BLK:
         return UNCORR_PTLP_EGR_BLK_OFFSET;
    default:
         val_print(AVS_PRINT_ERR, "\n   Invalid error offset ", 0);
         return 0;
    }
}


/**
  @brief   This API returns the requested information about the PCIe stimulus hardware
  @param   type         - Information type required from the stimulus hadrware
  @param   instance     - Stimulus hadrware instance number
  @return  value        - Information value for input type
**/
uint32_t val_exerciser_get_info(EXERCISER_INFO_TYPE type, uint32_t instance)
{
    switch (type) {
    case EXERCISER_NUM_CARDS:
         return g_exercier_info_table.num_exerciser;
    default:
         return 0;
    }
}

/**
  @brief   This API writes the configuration parameters of the PCIe stimulus generation hardware
  @param   type         - Parameter type that needs to be set in the stimulus hadrware
  @param   value1       - Parameter 1 that needs to be set
  @param   value2       - Parameter 2 that needs to be set
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the input paramter type is successfully written
**/
uint32_t val_exerciser_set_param(EXERCISER_PARAM_TYPE type, uint64_t value1, uint64_t value2,
                                 uint32_t instance)
{
    return pal_exerciser_set_param(type, value1, value2,
                                   g_exercier_info_table.e_info[instance].bdf);
}

uint32_t val_exerciser_get_bdf(uint32_t instance)
{
    return g_exercier_info_table.e_info[instance].bdf;
}
/**
  @brief   This API reads the configuration parameters of the PCIe stimulus generation hardware
  @param   type         - Parameter type that needs to be read from the stimulus hadrware
  @param   value1       - Parameter 1 that is read from hardware
  @param   value2       - Parameter 2 that is read from hardware
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the requested paramter type is successfully read
**/
uint32_t val_exerciser_get_param(EXERCISER_PARAM_TYPE type, uint64_t *value1, uint64_t *value2,
                                 uint32_t instance)
{
    return pal_exerciser_get_param(type, value1, value2,
                                   g_exercier_info_table.e_info[instance].bdf);

}

/**
  @brief   This API sets the state of the PCIe stimulus generation hardware
  @param   state        - State that needs to be set for the stimulus hadrware
  @param   value        - Additional information associated with the state
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the input state is successfully written to hardware
**/
uint32_t val_exerciser_set_state(EXERCISER_STATE state, uint64_t *value, uint32_t instance)
{
    return pal_exerciser_set_state(state, value, g_exercier_info_table.e_info[instance].bdf);
}

/**
  @brief   This API obtains the state of the PCIe stimulus generation hardware
  @param   state        - State that is read from the stimulus hadrware
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the state is successfully read from hardware
**/
uint32_t val_exerciser_get_state(EXERCISER_STATE *state, uint32_t instance)
{
    return pal_exerciser_get_state(state, g_exercier_info_table.e_info[instance].bdf);
}

/**
  @brief   This API obtains initializes
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the state is successfully read from hardware
**/
uint32_t val_exerciser_init(uint32_t instance)
{
  uint32_t Bdf;
  uint64_t Ecam;
  uint64_t cfg_addr;
  EXERCISER_STATE state;

  if (!g_exercier_info_table.e_info[instance].initialized)
  {
      Bdf = g_exercier_info_table.e_info[instance].bdf;
      if (pal_exerciser_get_state(&state, Bdf) || (state != EXERCISER_ON)) {
          val_print(AVS_PRINT_ERR, "\n   Exerciser Bdf %lx not ready", Bdf);
          return 1;
      }

      // setting command register for Memory Space Enable and Bus Master Enable
      Ecam = val_pcie_get_ecam_base(Bdf);

      /* There are 8 functions / device, 32 devices / Bus and each has a 4KB config space */
      cfg_addr = (PCIE_EXTRACT_BDF_BUS(Bdf) * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
                 (PCIE_EXTRACT_BDF_DEV(Bdf) * PCIE_MAX_FUNC * 4096) + \
                 (PCIE_EXTRACT_BDF_FUNC(Bdf) * 4096);

      pal_mmio_write((Ecam + cfg_addr + COMMAND_REG_OFFSET),
                  (pal_mmio_read((Ecam + cfg_addr) + COMMAND_REG_OFFSET) | BUS_MEM_EN_MASK));

      g_exercier_info_table.e_info[instance].initialized = 1;
  }
  else
           val_print(AVS_PRINT_INFO, "\n  Already initialized %d", instance);
  return 0;
}
/**
  @brief   This API performs the input operation using the PCIe stimulus generation hardware
  @param   ops          - Operation that needs to be performed with the stimulus hadrware
  @param   value        - Additional information to perform the operation
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the operation is successfully performed using the hardware
**/
uint32_t val_exerciser_ops(EXERCISER_OPS ops, uint64_t param, uint32_t instance)
{
    return pal_exerciser_ops(ops, param, g_exercier_info_table.e_info[instance].bdf);
}

/**
  @brief   This API returns test specific data from the PCIe stimulus generation hardware
  @param   type         - data type for which the data needs to be returned
  @param   data         - test specific data to be be filled by pal layer
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the requested data is successfully filled
**/
uint32_t val_exerciser_get_data(EXERCISER_DATA_TYPE type, exerciser_data_t *data,
                                uint32_t instance)
{
    uint32_t bdf = g_exercier_info_table.e_info[instance].bdf;
    uint64_t ecam = val_pcie_get_ecam_base(bdf);
    return pal_exerciser_get_data(type, data, bdf, ecam);
}

/**
  @brief   This API executes all the Exerciser tests sequentially
           1. Caller       -  Application layer.
  @param   level  - level of compliance being tested for.
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_exerciser_execute_tests(uint32_t level)
{
  uint32_t status, i;
  uint32_t num_instances;
  uint32_t instance;
  uint32_t num_smmu;

  if (level == 3) {
    val_print(AVS_PRINT_WARN, "Exerciser Sbsa compliance is only from Level %d \n", 4);
    return AVS_STATUS_SKIP;
  }

  for (i = 0; i < MAX_TEST_SKIP_NUM; i++){
      if (g_skip_test_num[i] == AVS_EXERCISER_TEST_NUM_BASE) {
          val_print(AVS_PRINT_TEST, "\n USER Override - Skipping the Exerciser tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  if (g_single_module != SINGLE_MODULE_SENTINEL && g_single_module != AVS_EXERCISER_TEST_NUM_BASE &&
       (g_single_test == SINGLE_MODULE_SENTINEL ||
         (g_single_test - AVS_EXERCISER_TEST_NUM_BASE > 100 ||
          g_single_test - AVS_EXERCISER_TEST_NUM_BASE < 0))) {
    val_print(AVS_PRINT_TEST, " USER Override - Skipping all Exerciser tests \n", 0);
    val_print(AVS_PRINT_TEST, " (Running only a single module)\n", 0);
    return AVS_STATUS_SKIP;
  }

  if (val_exerciser_create_info_table()) {
      val_print(AVS_PRINT_WARN, "\n     Create BDF Table Failed, Skipping Exerciser tests...\n", 0);
      return AVS_STATUS_SKIP;
  }

  num_instances = val_exerciser_get_info(EXERCISER_NUM_CARDS, 0);

  if (num_instances == 0) {
      val_print(AVS_PRINT_WARN, "\n     No Exerciser Devices Found, Skipping Exerciser tests...\n", 0);
      return AVS_STATUS_SKIP;
  }


  num_smmu = val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0);
  val_smmu_init();

  /* Disable All SMMU's */
  for (instance = 0; instance < num_smmu; ++instance)
      val_smmu_disable(instance);


  g_curr_module = 1 << EXERCISER_MODULE;
  status = e001_entry();
  status |= e002_entry();
  status |= e003_entry();
  status |= e004_entry();
  status |= e005_entry();
  status |= e006_entry();
  status |= e007_entry();
  status |= e008_entry();
  status |= e009_entry();
  status |= e010_entry();
  status |= e011_entry();
  status |= e012_entry();
  status |= e013_entry();
  status |= e014_entry();
  status |= e015_entry();
  status |= e016_entry();

  if (level > 6) {
      status = e017_entry();
      status |= e018_entry();
      status |= e019_entry();
      status |= e020_entry();
  }
  val_print_test_end(status, "Exerciser");

  val_smmu_stop();

  return status;
}

