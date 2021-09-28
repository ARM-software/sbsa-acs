/** @file
 * Copyright (c) 2020, 2021 Arm Limited or its affiliates. All rights reserved.
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

#include "include/pal_common_support.h"
#include "include/pal_pcie_enum.h"
#include "FVP/include/platform_override_fvp.h"


uint64_t
pal_pcie_get_mcfg_ecam();

/**
  @brief This API will return the ECSR base address of particular BAR Index
**/
uint64_t
pal_exerciser_get_ecsr_base(uint32_t Bdf, uint32_t BarIndex)
{
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

    return pal_pcie_get_base(Bdf, BarIndex);
#endif
    return 0;
}

uint64_t
pal_exerciser_get_pcie_config_offset(uint32_t Bdf)
{
  uint32_t bus     = PCIE_EXTRACT_BDF_BUS(Bdf);
  uint32_t dev     = PCIE_EXTRACT_BDF_DEV(Bdf);
  uint32_t func    = PCIE_EXTRACT_BDF_FUNC(Bdf);
  uint64_t cfg_addr;

   /* There are 8 functions / device, 32 devices / Bus and each has a 4KB config space */
  cfg_addr = (bus * PCIE_MAX_DEV * PCIE_MAX_FUNC * 4096) + \
               (dev * PCIE_MAX_FUNC * 4096) + (func * 4096);

  return cfg_addr;
}

/**
  @brief  This API returns if the device is a exerciser
  @param  bdf - Bus/Device/Function
  @return 1 - true 0 - false
**/
uint32_t
pal_is_bdf_exerciser(uint32_t bdf)
{
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint32_t Ecam;
  uint32_t vendor_dev_id;
  Ecam = pal_pcie_get_mcfg_ecam();

  vendor_dev_id = pal_mmio_read(Ecam + pal_exerciser_get_pcie_config_offset(bdf));
  if (vendor_dev_id == EXERCISER_ID)
     return 1;
  else
     return 0;
#endif

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
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint32_t Data;
  uint64_t Base;

  Base = pal_exerciser_get_ecsr_base(Bdf,0);

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

      default:
          return 1;
  }
#endif
  return 1;
}

/**
  @brief This function triggers the DMA operation
**/
uint32_t pal_exerciser_start_dma_direction (uint64_t Base, EXERCISER_DMA_ATTR Direction)
{
#ifdef ENABLE_OOB
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
#endif
  return 0;
}


/**
  @brief This function finds the PCI capability and return 0 if it finds.
**/
uint32_t pal_exerciser_find_pcie_capability (uint32_t ID, uint32_t Bdf, uint32_t Value, uint32_t *Offset)
{
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint64_t NxtPtr;
  uint32_t Data;
  uint32_t TempId;
  uint32_t Ecam;
  uint32_t IdMask;
  uint32_t PtrMask;
  uint32_t PtrOffset;

  Ecam = pal_pcie_get_mcfg_ecam();
  NxtPtr = PCIE_CAP_OFFSET;

  if (Value == 1) {
      IdMask = PCIE_CAP_ID_MASK;
      PtrMask = PCIE_NXT_CAP_PTR_MASK;
      PtrOffset = PCIE_CAP_PTR_OFFSET;
      NxtPtr = PCIE_CAP_OFFSET;
  }
  else {
      IdMask = PCI_CAP_ID_MASK;
      PtrMask = PCI_NXT_CAP_PTR_MASK;
      PtrOffset = PCI_CAP_PTR_OFFSET;
      NxtPtr = (pal_mmio_read(Ecam + CAP_PTR_OFFSET + pal_exerciser_get_pcie_config_offset(Bdf))) & CAP_PTR_MASK;
  }
  while (NxtPtr != 0) {
    Data = pal_mmio_read(Ecam + pal_exerciser_get_pcie_config_offset(Bdf) + NxtPtr);
    TempId = Data & IdMask;
    if (TempId == ID){
        *Offset = NxtPtr;
        return 0;
    }
    NxtPtr = (Data >> PtrOffset) & PtrMask;
  }
  print(AVS_PRINT_ERR, "\n No capabilities found",0);
  return 1;
#endif
  return 0;
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
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint32_t Status;
  uint32_t Temp;
  uint64_t Base;

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
      default:
          return 1;
  }
#endif
  return 0;
}

/**
  @brief   This API obtains the state of the PCIe stimulus generation hardware
  @param   State        - State that is read from the stimulus hadrware
  @param   Bdf          - Stimulus hardware bdf number
  @return  Status       - SUCCESS if the state is successfully read from hardware
**/
uint32_t pal_exerciser_get_state(EXERCISER_STATE *State, uint32_t Bdf)
{
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

    *State = EXERCISER_ON;
    return 0;
#endif
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
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint64_t Base;
  uint32_t Ecam;
  uint32_t CapabilityOffset;
  uint32_t data;

  Base = pal_exerciser_get_ecsr_base(Bdf,0);
  Ecam = pal_pcie_get_mcfg_ecam(); // Getting the ECAM address

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

    default:
        return PCIE_CAP_NOT_FOUND;;
  }
#endif
  return 1;
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
#ifdef ENABLE_OOB
 /* Below code is not applicable for Bare-metal
  * Only for FVP OOB experience
  */

  uint32_t Index;
  uint64_t EcamBase;
  uint64_t EcamBAR0;

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
      default:
          return 1;
    }
#endif
  return 1;
}
