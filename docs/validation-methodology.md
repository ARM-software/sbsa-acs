# SBSA ACS Validation Methodology


## About the SBSA
    The Server Base Systems Architecture (SBSA) specification specifies a hardware system architecture, based on ARM 64-bit architecture, which server system software, such as operating systems, hypervisors and firmware can rely on. It addresses PE features and key aspects of system architecture. 
The primary goal is to ensure enough standard system architecture to enable a suitably-built single OS image to run on all hardware compliant with this specification. 
It also specifies features that firmware can rely on, allowing for some commonality in firmware implementation across platforms

The SBSA architecture that is described in the SBSA Architecture Specification defines the behavior of an abstract machine, referred to as an SBSA system. Implementations compliant with the SBSA architecture must conform to the described behavior of the SBSA Architecture Specification.

The Architecture Compliance Suite is a set of examples of the invariant behaviors that are
specified by the SBSA Specification. Use this suite to verify that these behaviors
are implemented correctly in your system.

## Terms are Abbreviations


This document uses the following terms and abbreviations.

| Term | Meaning                                     |
|------|---------------------------------------------|
| ACPI | Advanced Configuration and Power Interface  |
| GIC  | Generic Interrupt Controller                |
| SPI  | Shared Peripheral Interrupt                 |
| PPI  | Private Peripheral Interrupt                |
| LPI  | Locality Specific Peripheral Interrupt      |
| XSDT | Extended System Description Table           |
| SMC  | Secure Monitor Call                         |
| PE   | Processing Element                          |
| PSCI | Power State Coordination Interface          |
| PCIe | Peripheral Component Interconnect - Express |
| SBSA | Server Base Systems Architecture            |
| UART | Universal Asynchronous Receiver/Transmitter |
| UEFI | Unified Extensible Firmware Interface       |


## Additional reading

  This document refers to the following external documents.
  1. http://www.uefi.org/specifications
    - ACPI Specification Version 6.1
    - UEFI Specification Version 2.6
    - UEFI Platform Initialization Specification Version 1.4 (Volume 2)
  2.  http://www.uefi.org/acpi
    - WDRT
    - MCFG
    - IORT
    - DBG2

See Infocenter: http://infocenter.arm.com for access to ARM documentation

**ARM Publications**

  This document refers to the following documents:
    1. Server Base System Architecture (ARM-DEN-0029 Version 3.0)
    2. Server Base Boot Requirements (ARM-DEN-0044B)
    3. ARM® Architecture Reference Manual ARMv8, for ARMv8-A architecture profile (ARM DDI 0487).


## Compliance Test Suite

SBSA compliance tests are self-checking, portable C-based tests with directed stimulus.
The below table describes the various components of the compliance suite.

| Components   | Description                                            |
|--------------|--------------------------------------------------------|
| PE           | Tests to verify PE compliance                          |
| GIC          | Tests to verify GIC compliance                         |
| Timers       | Tests to verify PE timers and System timers compliance |
| Watchdog     | Tests to verify Watchdog compliance                    |
| PCIe         | Tests to verify PCIe subsystem compliance              |
| Peripherals  | Tests to verify Memory, USB, SATA and UART compliance  |
| Power states | Tests to verify System power states compliance         |
| SMMU         | Tests to verify SMMU subsystem compliance              | 




## Layered Software Stack
 
Compliance tests use the layered software-stack approach to enable porting across different test platforms. The constituents of the layered stack are:
1.	Test Suite
2.	Validation Abstraction Layer (VAL)
3.	Platform Abstraction Layer (PAL)


### Platform Abstraction Layer
  This layer abstracts features whose implementation varies from one target system to another. This layer requires porting from one target system to another and is closest to hardware and system software. 

### Validation Abstraction Layer
  This layer provides a uniform view of all the underlying hardware and test infrastructure to the test suite. 

### Test Suite
  This is a collection of targeted tests which validate the compliance of the target system. These tests use interfaces provided by the VAL layer.


## Coding Guidelines

  - All Tests call VAL layer APIs.
  - VAL layer APIs may call PAL layer APIs depending on the functionality requested.
  - A test does not directly interface with PAL layer functions.
  - The Test Layer does not need any code modifications when porting from one platform to another.
  - All the platform porting changes will be limited to PAL layer.
  - The VAL layer may require changes if there are architectural changes impacting multiple platforms.


## Test Platform Abstraction

| Abstraction      | Description                                                                                                                             |
|------------------|-----------------------------------------------------------------------------------------------------------------------------------------|
| UEFI /OS         | UEFI Shell Application or Operating system provide infrastructure for console and memory management. This module runs at EL2            |
|                  |                                                                                                                                         |
| Trusted Firmware | Firmware which runs at EL3                                                                                                              |
|                  |                                                                                                                                         |
| ACPI             | Interface layer which provides platform specific information, removing the need for the Test suite to be ported on a per platform basis |
|                  |                                                                                                                                         |
| Shared Memory    | Memory which is visible to all the PE and test peripherals.                                                                             |
|                  |                                                                                                                                         |
| Hardware         | PE and controllers which are specified as part of the SBSA specification.                                                               |



## Platform Abstraction Layer


These APIs may require porting depending on the system under test.

| API Name                         | Consumed by VAL Module |
|----------------------------------|------------------------|
|                                  |                        |
| pal_pe_create_info_table         | PE                     |
| pal_pe_call_smc                  | PE                     |
| pal_pe_execute_payload           | PE                     |
| pal_pe_install_esr               | PE                     |
| pal_gic_create_info_table        | GIC                    |
| pal_gic_install_isr              | GIC                    |
| pal_timer_create_info_table      | Timer                  |
| pal_timer_system_start_countdown | Timer                  |
| pal_wd_create_info_table         | Watchdog               |
| pal_pcie_create_info_table       | PCIe                   |
| pal_pcie_get_mcfg_ecam           | PCIe                   |
| pal_peripheral_create_info_table | Peripheral             |
| pal_memory_create_info_table     | Peripheral             |
| pal_peripheral_do_dma_write      | SMMU/PCIe              |
| pal_peripheral_do_dma_read       | SMMU/PCIe              |
| pal_smmu_create_info_table       | SMMU                   |
| pal_print                        | Test Infra             |
| pal_mem_alloc                    | Test Infra             |
| pal_mem_free                     | Test Infra             |
| pal_mem_allocate_shared          | Test Infra             |
| pal_mem_free_shared              | Test Infra             |
| pal_mem_get_shared_addr          | Test Infra             |
| pal_mem_get_shared_addr          | Test Infra             |
| pal_mmio_read                    | Test Infra             |
| pal_mmio_write                   | Test Infra             |

