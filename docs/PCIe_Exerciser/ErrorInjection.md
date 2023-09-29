# PCIe Error Injection user documentation
This document gives details of the error injection mechanism for PCIe endpoints and how it can be used to inject errors and check for error reporting inside PCIe subsystem.

## Introduction to PCIe error injection mechanism

A new Error injection extended capability, implemented as a DVSEC, has been added to enable user to inject errors in an PCIe endpoint. This capability can be enabled using a new parameter : `error_injection_supported`. The layout of this capability is as follows:

```
 Register Name                Description                                   Address offset
 -------------                -----------                                   --------------

 Extended Capability Header   Standard DVSEC header.                             0x0

 DVSEC Header 1               Error injection DVSEC details.                     0x4

 Control Register             Configure and inject errors (ERROR_CTL_REG)        0x8

```

## Register description

```
 Extended Capability Header  Bits   Description                                   r/w  Value at reset
 --------------------------  -----  -----------                                   ---  --------------

 extended_capability_id      15:0   Identifies which extended capability          RO      0x0023
                                    structure this is.

 capability_version          19:16  Identifies the version of this                RO      0x1
                                    structure.

 next_capability_offset      31:20  Provides the byte offset from the top of      RO     Depends on
                                    config space to the next extended capability         position of
                                    structure. A value of 000h ends the linked         next capability
                                    list of extended capability structures.


 DVSEC Header 1              Bits   Description                                   r/w  Value at reset
 --------------------------  -----  -----------                                   ---  --------------

 dvsec_vendor_id             15:0   Holds a designated VendorID assigned by the   RO       0x13B5
                                    PCI-SIG. This VendorID would be assigned to
                                    a group of companies collaborating on a
                                    common set of register definitions which
                                    would like in this structure in PCI config
                                    space.

 dvsec_rev                   19:16  Holds a vendor-defined version number         RO       0x0

 dvsec_length                31:20  Indicates the size (in bytes) of this         RO       0xC
                                    extended capability structure, including
                                    the Extended Capability Header, DVSEC
                                    Header1 and DVSEC Header 2.


 Control Register            Bits   Description                                   r/w  Value at reset
 --------------------------  -----  -----------                                   ---  --------------

 dvsec_id                    15:0   Holds a vendor-defined identification         RO       0x1
                                    number to help determine the nature and
                                    format of this structure.

 inject_error_on_dma         16     Put endpoint in Corrupt DMA mode. See         RW       0x0
                                    Corrupt DMA section for more details.

 inject_error_immediately    17     Inject error in this endpoint configured      RW       0x0
                                    using the error_code field.

 reserved                    19:18  Reserved                                      RO       0x0

 error_code                  30:20  Error code configuration for corrupt DMA      RW       0x0
                                    mode and inject_error_immediately

 treat_uncorrectable_as_     31     If error code is an uncorrectable error,      RW       0x0
 fatal                              then this bit configures severity of the
                                    error. This bit has no effect if endpoint
                                    supports Advanced Error Reporting (AER).
                                    If AER is supported, this bit is
                                    overridden by AER uncorrectable severity
                                    register.
```

## How to inject errors

There are 2 ways to inject errors:
  * Inject immediately : Using the `inject_error_immediately` bit set, the user can inject an error at that endpoint with the error configured using the `error_code` field in control register. The error_codes are defined in Error Codes section. This bit is cleared once the error has been injected.

  * Corrupt DMA : With the `inject_error_on_dma` bit set, the endpoint is put in Corrupt DMA mode. Any peer-to-peer DMAs generated in corrupt DMA mode, will lead to an error injection in destination endpoint. All DMAs will fail by default in this mode, so this bit will need to be cleared for normal functioning of DMAs for this endpoint. The injected error in destination endpoint will be as configured by `error_code` field in control register. The error_codes are defined in Error Codes section.

## Error Codes
```
    Error Name                             Error Code
    ----------                             ----------
    Correctable Receiver Error                0x00
    Correctable Bad TLP                       0x01
    Correctable Bad DLLP                      0x02
    Correctable Replay Num Rollover           0x03
    Correctable Replay Timer Timeout          0x04
    Correctable Advisory Non-Fatal Error      0x05
    Correctable Internal Error                0x06
    Correctable Header Log OverFlow           0x07
    Uncorrectable Data Link Error             0x08
    Uncorrectable Surprise Down Error         0x09
    Uncorrectable Poisoned TLP Received       0x0A
    Uncorrectable Flow Control Error          0x0B
    Uncorrectable Completion Timeout          0x0C
    Uncorrectable Completer Abort             0x0D
    Uncorrectable Unexpected Completion       0x0E
    Uncorrectable Receiver Overflow           0x0F
    Uncorrectable Malformed TLP               0x10
    Uncorrectable ECRC Error                  0x11
    Uncorrectable Unsupported Request         0x12
    Uncorrectable ACS Violation               0x13
    Uncorrectable Internal Error              0x14
    Uncorrectable MultiCast Blocked TLP       0x15
    Uncorrectable Atomic Op Egress Blocked    0x16
    Uncorrectable TLP Prefix Blocked Egress   0x17
    Uncorrectable Poisoned TLP Egress Blocked 0x18
    Invalid configuration                     0x19 onwards
```

## Error reporting in intermediate components:
When an corrupt DMA passes through an intermediate component such as a switch, it can detect and report an uncorrectable error as an advisory non-fatal error. The detecting bridges (inside the switch) will, in this case, log a correctable advisory non-fatal error and optionally report an correctable error message to its parent root port. This detection and reporting of intermediate errors can be enabled using a new parameter `report_advisory_non_fatal_errors` for a switch.

--------------

*Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.*
