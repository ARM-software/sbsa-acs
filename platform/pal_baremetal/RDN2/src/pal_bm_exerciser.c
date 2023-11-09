/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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

#include "pal_common_support.h"
#include "pal_pcie_enum.h"
#include "platform_override_struct.h"

uint64_t
pal_exerciser_get_ecam(uint32_t Bdf);

uint64_t
pal_exerciser_get_pcie_config_offset(uint32_t Bdf);

uint64_t
pal_exerciser_get_ecsr_base(uint32_t Bdf, uint32_t BarIndex);

uint32_t
pal_exerciser_find_pcie_capability (uint32_t ID, uint32_t Bdf, uint32_t Value, uint32_t *Offset);
/**
  @brief  This API returns if the device is a exerciser
  @param  bdf - Bus/Device/Function
  @return 1 - true 0 - false
**/
uint32_t
pal_is_bdf_exerciser(uint32_t bdf)
{
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint64_t Ecam;
  uint32_t vendor_dev_id;
  Ecam = pal_exerciser_get_ecam(bdf);

  vendor_dev_id = pal_mmio_read(Ecam + pal_exerciser_get_pcie_config_offset(bdf));
  if (vendor_dev_id == EXERCISER_ID)
     return 1;
  else
     return 0;
}

/**
  @brief   This API writes the configuration parameters of the PCIe stimulus generation hardware
  @param   Type         - Parameter type that needs to be set in the stimulus hadrware
  @param   Value1       - Parameter 1 that needs to be set
  @param   Value2       - Parameter 2 that needs to be set
  @param   Instance     - Stimulus hardware instance number
  @return  Status       - SUCCESS if the input paramter type is successfully written
**/
uint32_t pal_exerciser_set_param(EXERCISER_PARAM_TYPE Type, uint64_t Value1, uint64_t Value2, uint32_t Bdf)
{
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint32_t CapabilityOffset = 0;
  uint32_t Data;
  uint64_t Base;
  uint64_t Ecam;

  Base = pal_exerciser_get_ecsr_base(Bdf,0);
  Ecam = pal_exerciser_get_ecam(Bdf); // Getting the ECAM address

  switch (Type) {

      case SNOOP_ATTRIBUTES:
          return 0;

      case LEGACY_IRQ:
          return 0;

      case DMA_ATTRIBUTES:
          pal_mmio_write(Base + DMA_BUS_ADDR,Value1);// wrting into the DMA Control Register 2
          pal_mmio_write(Base + DMA_LEN,Value2);// writing into the DMA Control Register 3
          return 0;

      case P2P_ATTRIBUTES:
          return 0;

      case PASID_ATTRIBUTES:
          Data = pal_mmio_read(Base + DMACTL1);
          Data &= ~(PASID_LEN_MASK << PASID_LEN_SHIFT);
          Data |= ((Value1 - 16) & PASID_LEN_MASK) << PASID_LEN_SHIFT;
          pal_mmio_write(Base + DMACTL1, Data);
          return 0;

      case MSIX_ATTRIBUTES:
          return 0;

      case CFG_TXN_ATTRIBUTES:
          switch (Value1) {

            case TXN_REQ_ID:
                /* Change Requester ID for DMA Transaction.*/
                Data = (Value2 & RID_VALUE_MASK) | RID_VALID_MASK;
                pal_mmio_write(Base + RID_CTL_REG, Data);
                return 0;
            case TXN_REQ_ID_VALID:
                switch (Value2)
                {
                    case RID_VALID:
                        Data = pal_mmio_read(Base + RID_CTL_REG);
                        Data |= RID_VALID_MASK;
                        pal_mmio_write(Base + RID_CTL_REG, Data);
                        return 0;
                    case RID_NOT_VALID:
                        pal_mmio_write(Base + RID_CTL_REG, 0);
                        return 0;
                }
                return 0;
            case TXN_ADDR_TYPE:
                /* Change Address Type for DMA Transaction.*/
                switch (Value2)
                {
                    case AT_UNTRANSLATED:
                        Data = 0x1;
                        pal_mmio_write(Base + DMACTL1, pal_mmio_read(Base + DMACTL1) | (Data << 10));
                        break;
                    case AT_TRANSLATED:
                        Data = 0x2;
                        pal_mmio_write(Base + DMACTL1, pal_mmio_read(Base + DMACTL1) | (Data << 10));
                        break;
                    case AT_RESERVED:
                        Data = 0x3;
                        pal_mmio_write(Base + DMACTL1, pal_mmio_read(Base + DMACTL1) | (Data << 10));
                        break;
                }
                return 0;
            default:
                return 1;
          }

     case ERROR_INJECT_TYPE:
        pal_exerciser_find_pcie_capability(DVSEC, Bdf, PCIE, &CapabilityOffset);
        Data = pal_mmio_read(Ecam + CapabilityOffset +
                             pal_exerciser_get_pcie_config_offset(Bdf) + DVSEC_CTRL);
        Data = ((Value1 << ERR_CODE_SHIFT) | (Value2 << FATAL_SHIFT));
        pal_mmio_write(Ecam + CapabilityOffset + DVSEC_CTRL +
                             pal_exerciser_get_pcie_config_offset(Bdf), Data);
        if (Value1 <= 0x7)
                return 2;
        else
                return 3;
      default:
          return 1;
  }
}

/**
  @brief This function triggers the DMA operation
**/
uint32_t pal_exerciser_start_dma_direction (uint64_t Base, EXERCISER_DMA_ATTR Direction)
{
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint32_t Mask;
  uint32_t Status;

  if (Direction == EDMA_TO_DEVICE) {

      Mask = DMA_TO_DEVICE_MASK;//  DMA direction:to Device
      // Setting DMA direction in DMA control register 1
      pal_mmio_write(Base + DMACTL1, (pal_mmio_read(Base + DMACTL1) & Mask));
  }
  if (Direction == EDMA_FROM_DEVICE) {
      Mask = (MASK_BIT << SHIFT_4BIT);// DMA direction:from device
      // Setting DMA direction in DMA control register 1
      pal_mmio_write(Base + DMACTL1, (pal_mmio_read(Base + DMACTL1) | Mask));
  }
  // Triggering the DMA
  pal_mmio_write(Base + DMACTL1, (pal_mmio_read(Base + DMACTL1) | MASK_BIT));

  // Reading the Status of the DMA

  Status = (pal_mmio_read(Base + DMASTATUS) & ((MASK_BIT << 1) | MASK_BIT));
  return Status;
}

/**
  @brief   This API reads the configuration parameters of the PCIe stimulus generation hardware
  @param   Type         - Parameter type that needs to be read from the stimulus hadrware
  @param   Value1       - Parameter 1 that is read from hardware
  @param   Value2       - Parameter 2 that is read from hardware
  @param   Instance     - Stimulus hardware instance number
  @return  Status       - SUCCESS if the requested paramter type is successfully read
**/
uint32_t pal_exerciser_get_param(EXERCISER_PARAM_TYPE Type, uint64_t *Value1, uint64_t *Value2, uint32_t Bdf)
{
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint32_t Status;
  uint32_t Temp;
  uint64_t Base;
  uint32_t tx_attr;
  uint32_t addr_low = 0;
  uint32_t addr_high = 0;
  uint32_t data_low = 0;
  uint32_t data_high = 0;

  Base = pal_exerciser_get_ecsr_base(Bdf,0);
  switch (Type) {

      case SNOOP_ATTRIBUTES:
          return 0;
      case LEGACY_IRQ:
          *Value1 = pal_mmio_read(Base + INTXCTL);
          return pal_mmio_read(Base + INTXCTL) | MASK_BIT ;
      case DMA_ATTRIBUTES:
          *Value1 = pal_mmio_read(Base + DMA_BUS_ADDR); // Reading the data from DMA Control Register 2
          *Value2 = pal_mmio_read(Base + DMA_LEN); // Reading the data from DMA Control Register 3
          Temp = pal_mmio_read(Base + DMASTATUS);
          Status = Temp & MASK_BIT;// returning the DMA status
          return Status;
      case P2P_ATTRIBUTES:
          return 0;
      case PASID_ATTRIBUTES:
          *Value1 = ((pal_mmio_read(Base + DMACTL1) >> PASID_LEN_SHIFT) & PASID_LEN_MASK) + 16;
          return 0;
      case MSIX_ATTRIBUTES:
          *Value1 = pal_mmio_read(Base + MSICTL);
          return pal_mmio_read(Base + MSICTL) | MASK_BIT;
      case ATS_RES_ATTRIBUTES:
          *Value1 = pal_mmio_read(Base + ATS_ADDR);
          return 0;
      case CFG_TXN_ATTRIBUTES:
      case TRANSACTION_TYPE:
      case ADDRESS_ATTRIBUTES:
      case DATA_ATTRIBUTES:
          /* Get the first entry and check for validity */
          tx_attr = pal_mmio_read(Base + TXN_TRACE);
          if (tx_attr == TXN_INVALID)
              return 1;

          /* Obtain all the recorded information of the packet in the format.
               ________________________________
              |         TX ATTRIBUTES          |
              |      CFG/MEM ADDRESS_LO        |
              |      CFG/MEM ADDRESS_HI        |
              |           DATA_LO              |
              |           DATA_HI              |
              |________________________________|
          */
          addr_low = pal_mmio_read(Base + TXN_TRACE);
          addr_high = pal_mmio_read(Base + TXN_TRACE);
          data_low = pal_mmio_read(Base + TXN_TRACE);
          data_high = pal_mmio_read(Base + TXN_TRACE);

          if (Type == CFG_TXN_ATTRIBUTES)
              *Value1 = tx_attr & MASK_BIT;

          /* 0 - Read, 1 - Write */
          else if (Type == TRANSACTION_TYPE)
              if (tx_attr & 0x2)
                  *Value2 = CFG_READ;
              else
                  *Value2 = CFG_WRITE;

          else if (Type == ADDRESS_ATTRIBUTES)
               *Value1 = addr_low | ((uint64_t)addr_high << 32);

          else if (Type == DATA_ATTRIBUTES)
               *Value1 = data_low | ((uint64_t)data_high << 32);

          return 0;
      default:
          return 1;
  }
}

/**
  @brief   This API obtains the state of the PCIe stimulus generation hardware
  @param   State        - State that is read from the stimulus hadrware
  @param   Bdf          - Stimulus hardware bdf number
  @return  Status       - SUCCESS if the state is successfully read from hardware
**/
uint32_t pal_exerciser_get_state(EXERCISER_STATE *State, uint32_t Bdf)
{
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */
    (void) Bdf;

    *State = EXERCISER_ON;
    return 0;
}

/**
  @brief   This API performs the input operation using the PCIe stimulus generation hardware
  @param   Ops          - Operation thta needs to be performed with the stimulus hadrware
  @param   Value        - Additional information to perform the operation
  @param   Instance     - Stimulus hardware instance number
  @return  Status       - SUCCESS if the operation is successfully performed using the hardware
**/
uint32_t pal_exerciser_ops(EXERCISER_OPS Ops, uint64_t Param, uint32_t Bdf)
{
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint64_t Base;
  uint64_t Ecam;
  uint32_t CapabilityOffset = 0;
  uint32_t data;

  Base = pal_exerciser_get_ecsr_base(Bdf,0);
  Ecam = pal_exerciser_get_ecam(Bdf); // Getting the ECAM address

  switch(Ops){

    case START_DMA:
        switch (Param) {

            case EDMA_NO_SUPPORT:
                return 0;
            case EDMA_COHERENT:
                return 0;
            case EDMA_NOT_COHERENT:
                return 0;
            case EDMA_FROM_DEVICE:
                return pal_exerciser_start_dma_direction(Base, EDMA_FROM_DEVICE);// DMA from Device
            case EDMA_TO_DEVICE:
                return pal_exerciser_start_dma_direction(Base, EDMA_TO_DEVICE);// DMA to Device
            default:
                return 1;
        }

    case GENERATE_MSI:
        /* Param is the msi_index */
        pal_mmio_write( Base + MSICTL ,(pal_mmio_read(Base + MSICTL) | (MSI_GENERATION_MASK) | (Param)));
        return 0;

    case GENERATE_L_INTR:
        pal_mmio_write(Base + INTXCTL , (pal_mmio_read(Base + INTXCTL) | MASK_BIT));
        return 0; //Legacy interrupt

    case MEM_READ:
        return 0;

    case MEM_WRITE:
        return 0;

    case CLEAR_INTR:
        pal_mmio_write(Base + INTXCTL , (pal_mmio_read(Base + INTXCTL) & CLR_INTR_MASK));
        return 0;

    case PASID_TLP_START:
        data = pal_mmio_read(Base + DMACTL1);
        data |= (MASK_BIT << PASID_EN_SHIFT);
        pal_mmio_write(Base + DMACTL1, data);
        data = ((Param & PASID_VAL_MASK));
        pal_mmio_write(Base + PASID_VAL, data);

        if (!pal_exerciser_find_pcie_capability(PASID, Bdf, PCIE, &CapabilityOffset)) {

            pal_mmio_write(Ecam + pal_exerciser_get_pcie_config_offset(Bdf) + CapabilityOffset + PCIE_CAP_CTRL_OFFSET,
                            (pal_mmio_read(Ecam + pal_exerciser_get_pcie_config_offset(Bdf) + CapabilityOffset + PCIE_CAP_CTRL_OFFSET)) | PCIE_CAP_EN_MASK);
            return 0;
        }
        return 1;

    case PASID_TLP_STOP:
        pal_mmio_write(Base + DMACTL1, (pal_mmio_read(Base + DMACTL1) & PASID_TLP_STOP_MASK));

        if (!pal_exerciser_find_pcie_capability(PASID, Bdf, PCIE, &CapabilityOffset)) {
            pal_mmio_write(Ecam + pal_exerciser_get_pcie_config_offset(Bdf) + CapabilityOffset + PCIE_CAP_CTRL_OFFSET,
                            (pal_mmio_read(Ecam + pal_exerciser_get_pcie_config_offset(Bdf) + CapabilityOffset + PCIE_CAP_CTRL_OFFSET)) & PCIE_CAP_DIS_MASK);
            return 0;
        }
        return 1;

    case TXN_NO_SNOOP_ENABLE:
        pal_mmio_write(Base + DMACTL1, (pal_mmio_read(Base + DMACTL1)) | NO_SNOOP_START_MASK);//enabling the NO SNOOP
        return 0;

    case TXN_NO_SNOOP_DISABLE:
        pal_mmio_write(Base + DMACTL1, (pal_mmio_read(Base + DMACTL1)) & NO_SNOOP_STOP_MASK);//disabling the NO SNOOP
        return 0;

    case ATS_TXN_REQ:
        pal_mmio_write(Base + DMA_BUS_ADDR, Param);
        pal_mmio_write(Base + ATSCTL, ATS_TRIGGER);
        return !(pal_mmio_read(Base + ATSCTL) & ATS_STATUS);

    case START_TXN_MONITOR:
        pal_mmio_write(Base + TXN_CTRL_BASE, TXN_START);
        return 0;

    case STOP_TXN_MONITOR:
        pal_mmio_write(Base + TXN_CTRL_BASE, TXN_STOP);
        return 0;

    case INJECT_ERROR:
        pal_exerciser_find_pcie_capability(DVSEC, Bdf, PCIE, &CapabilityOffset);
        data = pal_mmio_read(Ecam + pal_exerciser_get_pcie_config_offset(Bdf) +
                             CapabilityOffset + DVSEC_CTRL);
        data = data | (1 << ERROR_INJECT_BIT);
        pal_mmio_write(Ecam + pal_exerciser_get_pcie_config_offset(Bdf) + CapabilityOffset +
                       DVSEC_CTRL, data);
        return Param;

    default:
        return PCIE_CAP_NOT_FOUND;
  }
}



/**
  @brief   This API sets the state of the PCIe stimulus generation hardware
  @param   State        - State that needs to be set for the stimulus hadrware
  @param   Value        - Additional information associated with the state
  @param   Instance     - Stimulus hardware instance number
  @return  Status       - SUCCESS if the input state is successfully written to hardware
**/
uint32_t pal_exerciser_set_state (EXERCISER_STATE State, uint64_t *Value, uint32_t Instance)
{
    (void) State;
    (void) Value;
    (void) Instance;

    return 0;
}

/**
  @brief   This API returns test specific data from the PCIe stimulus generation hardware
  @param   type         - data type for which the data needs to be returned
  @param   data         - test specific data to be be filled by pal layer
  @param   instance     - Stimulus hardware instance number
  @return  status       - SUCCESS if the requested data is successfully filled
**/
uint32_t pal_exerciser_get_data(EXERCISER_DATA_TYPE Type, exerciser_data_t *Data, uint32_t Bdf, uint64_t Ecam)
{
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint32_t Index;
  uint64_t EcamBase;
  uint64_t EcamBAR0;
  uint64_t EcamBAR;

  EcamBase = (Ecam + pal_exerciser_get_pcie_config_offset(Bdf));

  //In the Latest version of SBSA 6.0 this part of the test is obsolete hence filling the reg with same data
  uint32_t offset_table[TEST_REG_COUNT] = {0x00,0x08,0x00,0x08,0x00,0x08,0x00,0x08,0x00,0x08};
  uint32_t attr_table[TEST_REG_COUNT] = {ACCESS_TYPE_RD,ACCESS_TYPE_RD,ACCESS_TYPE_RD,ACCESS_TYPE_RD,ACCESS_TYPE_RD,
                                       ACCESS_TYPE_RD,ACCESS_TYPE_RD,ACCESS_TYPE_RD,ACCESS_TYPE_RD,ACCESS_TYPE_RD,};

  switch(Type){
      case EXERCISER_DATA_CFG_SPACE:
          for (Index = 0; Index < TEST_REG_COUNT; Index++) {
              Data->cfg_space.reg[Index].offset = (offset_table[Index] + pal_exerciser_get_pcie_config_offset (Bdf));
              Data->cfg_space.reg[Index].attribute = attr_table[Index];
              Data->cfg_space.reg[Index].value = pal_mmio_read(EcamBase + offset_table[Index]);
          }
          return 0;
      case EXERCISER_DATA_BAR0_SPACE:
          EcamBAR0 = pal_exerciser_get_ecsr_base(Bdf, 0);
          Data->bar_space.base_addr = (void *)EcamBAR0;
          if (((pal_exerciser_get_ecsr_base(Bdf,0) >> PREFETCHABLE_BIT_SHIFT) & MASK_BIT) == 0x1)
              Data->bar_space.type = MMIO_PREFETCHABLE;
          else
              Data->bar_space.type = MMIO_NON_PREFETCHABLE;
          return 0;
      case EXERCISER_DATA_MMIO_SPACE:
          Index = 0;
          Data->bar_space.base_addr = 0;
          while (Index < TYPE0_MAX_BARS)
          {
              EcamBAR = pal_exerciser_get_ecsr_base(Bdf, Index * 4);

              /* Check if the BAR is Memory Mapped IO type */
              if (((EcamBAR >> BAR_MIT_SHIFT) & BAR_MIT_MASK) == MMIO)
              {
                  Data->bar_space.base_addr = (void *)(EcamBAR);
                  if (((EcamBAR >> PREFETCHABLE_BIT_SHIFT) & MASK_BIT) == 0x1)
                      Data->bar_space.type = MMIO_PREFETCHABLE;
                  else
                      Data->bar_space.type = MMIO_NON_PREFETCHABLE;

                  Data->bar_space.base_addr = (void *)EcamBAR;
                  return 0;
              }

              if (((EcamBAR >> BAR_MDT_SHIFT) & BAR_MDT_MASK) == BITS_64)
              {
                  /* Adjust the index to skip next sequential BAR */
                  Index++;
              }

              /* Adjust index to point to next BAR */
              Index++;
          }

          return 1;
      default:
          return 1;
    }
}
