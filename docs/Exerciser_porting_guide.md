# Exerciser Porting Guide
This document gives details of the various PCIe capabilities that exerciser device supports and how the exerciser is supposed to behave.

## Introduction to PCIe Exerciser Endpoint Device
PCIe Exerciser is a client device wrapped up by PCIe Endpoint. This device was created to meet SBSA (Server Based System Architecture) requirements for various PCIe capability validation tests.

### Generating DMA
- Before triggering DMA all the required DMA attribute fields like DMA bus address, DMA length, exerciser instance fields should be correctly set <br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)** <br/>
Type - DMA_ATTRIBUTES<br/>
Value1 - Buffer containing the data or the Buffer where the data to be copied<br/>
Value2 - Size of the data<br/>
BDF - BDF of the exerciser<br/>
- Trigger the DMA to/from the buffer<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - START_DMA<br/>
Param - EDMA_TO_DEVICE or EDMA_FROM_DEVICE<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### Generating DMA with PASID TLP Prefixes
- Program exerciser to start sending TLPs with PASID TLP Prefixes. This includes setting PASID Enable bit in exerciser PASID Control register and the implementation specific PASID Enable bit of the Root Port.<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - PASID_TLP_START<br/>
Param - Substream ID<br/>
Bdf - BDF of the exerciser<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - DMA_ATTRIBUTES<br/>
Value1 - Buffer containing the data or the Buffer where the data to be copied<br/>
Value2 - Size of the data<br/>
BDF - BDF of the exerciser<br/>
- Trigger the DMA to/from the buffer<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - START_DMA<br/>
Param - EDMA_TO_DEVICE or EDMA_FROM_DEVICE<br/>
Bdf - BDF of the exerciser<br/>
- Disable exerciser to stop sending TLPs with PASID TLP Prefixes.<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - PASID_TLP_STOP<br/>
Param - Substream ID<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### Generating DMA with No Snoop TLP
- Program exerciser hierarchy to start sending/receiving TLPs with No Snoop attribute header. This includes disabling No snoop bit in exerciser control register.<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - TXN_NO_SNOOP_DISABLE<br/>
Param - Null<br/>
Bdf - BDF of the exerciser<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - DMA_ATTRIBUTES<br/>
Value1 - Buffer containing the data or the Buffer where the data to be copied<br/>
Value2 - Size of the data<br/>
BDF - BDF of the exerciser<br/>
- Trigger the DMA to/from the buffer<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - START_DMA<br/>
Param - EDMA_TO_DEVICE or EDMA_FROM_DEVICE<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### ATS Request
- Before starting an ATS request, untranslated input address for ATSRequest must be written onto Bus Address Register<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - DMA_ATTRIBUTES<br/>
Value1 -  Untranslated input address<br/>
Value2 - Size<br/>
BDF - BDF of the exerciser<br/>
- Send an ATS Translation Request for the VA<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - ATS_TXN_REQ<br/>
Param - VA<br/>
Bdf - BDF of the exerciser<br/>
- Get ATS Translation Response<br/>
**pal_exerciser_get_param(Type, Value1, Value2, Bdf)**<br/>
Type - ATS_RES_ATTRIBUTES<br/>
Value1 - Buffer to store translated address<br/>
Value2 - Null<br/>
Bdf -  BDF of the exerciser<br/>
<br/>

### Generating DMA with Address Translated(AT)
- Configure Exerciser to issue subsequent DMA transactions with AT(Address Translated) bit Set<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - CFG_TXN_ATTRIBUTES<br/>
Value1 - TXN_ADDR_TYPE<br/>
Value2 - AT_TRANSLATED<br/>
BDF - BDF of the exerciser<br/>
**pal_exerciser_set_param(Type, Value1, Value2, Bdf)**<br/>
Type - DMA_ATTRIBUTES<br/>
Value1 - Buffer containing the data or the Buffer where the data to be copied<br/>
Value2 - Size of the data<br/>
BDF - BDF of the exerciser<br/>
- Trigger the DMA to/from the buffer<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - START_DMA<br/>
Param - EDMA_TO_DEVICE or EDMA_FROM_DEVICE<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### Trigerring MSI
- Trigger the interrupt for this Exerciser instance<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - GENERATE_MSI<br/>
Param - MSI index<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### Trigerring Legacy Interrupts
-  Clear any pending interrupts<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - CLEAR_INTR<br/>
Param - Legacy interrupt IRQ<br/>
Bdf - BDF of the exerciser<br/>
- Trigger the interrupt for this Exerciser instance<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - GENERATE_L_INTR<br/>
Param - Legacy interrupt IRQ<br/>
Bdf - BDF of the exerciser<br/>
<br/>

### Transaction Monitoring
- Transaction monitoring capabilities in the exerciser provides the ability to record the incoming transactions, for both config and memory transactions. This includes,<br/>
	-   config read and write transactions serviced in PCIe endpoints.<br/>
	-   memory transactions serviced in PCIe endpoint BARs.<br/>
	-   memory transactions which are forwarded from PCIe endpoint to device(like exerciser).<br/>
- To start transaction monitoring<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - START_TXN_MONITOR<br/>
Param - CFG_READ<br/>
Bdf - BDF of the exerciser<br/>
- After the transactions are performed, stop the transaction monitoring<br/>
**pal_exerciser_ops(Ops, Param, Bdf)**<br/>
Ops - STOP_TXN_MONITOR<br/>
Param - CFG_READ<br/>
Bdf - BDF of the exerciser<br/>
- Read the transaction trace<br/>
**pal_exerciser_get_param(Type, Value1, Value2, Bdf)**<br/>
Type -<br/>
	- CFG_TXN_ATTRIBUTES: Indicates transaction attributes. cfg or mem transaction<br/>
	- TRANSACTION_TYPE: Indicates transaction type. Read or Write transaction<br/>
	- ADDRESS_ATTRIBUTES: Config or memory address<br/>
	- DATA_ATTRIBUTES: Transaction data read or written to<br/>
Value1 - Requested transaction data<br/>
Value2 - Null<br/>
Bdf -  BDF of the exerciser<br/>
<br/>
<br/>

## License
Arm SBSA ACS is distributed under Apache v2.0 License.

--------------

*Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.*

