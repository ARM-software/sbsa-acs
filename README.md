
# Server Base System Architecture - Architecture Compliance Suite


## Server Base System Architecture
**Server Base System Architecture** (SBSA) specification specifies a hardware system architecture based on the Arm 64-bit architecture. Server system software such as operating systems, hypervisors, and firmware rely on this. It addresses processing element features and key aspects of system architecture.

For more information, download the [SBSA specification](https://developer.arm.com/documentation/den0029/d/?lang=en)


## SBSA - Architecture Compliance Suite

SBSA **Architecture Compliance Suite** (ACS) is a collection of self-checking, portable C-based tests.
This suite includes a set of examples of the invariant behaviors that are provided by the [SBSA specification](https://developer.arm.com/documentation/den0029/d/?lang=en), so that implementers can verify if these behaviours have been interpreted correctly.
Most of the tests are executed from UEFI Shell by executing the SBSA UEFI shell application.
A few tests are executed by running the SBSA ACS Linux application which in turn depends on the SBSA ACS Linux kernel module.


## Release details
 - Code Quality: REL v3.1
 - The tests are written for version 6.0 of the SBSA specification.
 - PCIe RCiEP tests for Appendix E of SBSA 6.0 specification are also included.
 - The compliance suite is not a substitute for design verification.
 - To review the SBSA ACS logs, Arm licensees can contact Arm directly through their partner managers.
 - To know about the gaps in the test coverage, see [Testcase checklist](docs/testcase-checklist.md).


## GitHub branch
  - To pick up the release version of the code, checkout the release branch.
  - To get the latest version of the code with bug fixes and new features, use the master branch.

## Additional reading
  - For details on the SBSA ACS UEFI Shell Application, see [Arm SBSA ACS User Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf).
  - For details on the Design of the SBSA ACS, see [Arm SBSA Validation Methodology Document](docs/Arm_SBSA_Architecture_Compliance_Validation_Methodology.pdf).
  - For information about the test coverage scenarios that are implemented in the current release of ACS and the scenarios that are   planned for the future releases, see [Testcase checklist](docs/testcase-checklist.md).

## SBSA ACS Baremetal Reference Code
Baremetal reference code is added as part of this release. For more information, see
  - [Baremetal User Guide](platform/pal_baremetal/docs/Arm_SBSA_ACS_Bare-metal_User_Guide.pdf).
  - [Baremetal Code](platform/pal_baremetal/). <br />
Note: The Baremetal PCIe enumeration code provided as part of the SBSA ACS should be used and should not be replaced. This code is vital in analyzing the test result.

## SBSA ACS Linux kernel module
To enable the export of a few kernel APIs that are necessary for PCIe and IOMMU tests, Linux kernel module and a kernel patch file are required. These files are available at [linux-acs](https://gitlab.arm.com/linux-arm/linux-acs).

## Target platforms
  Any AARCH64 Enterprise Platform that boots UEFI and Linux OS.

## ACS build steps - UEFI Shell application

### Prebuilt images
Prebuilt images for each release are available in the prebuilt_images folder of the release branch. You can choose to use these images or build your own image by following the steps below. If you choose to use the prebuilt image, jump to the test suite execution section below for details on how to run the application.

### Prerequisites
    Before starting the ACS build, ensure that the following requirements are met.

- Any mainstream Linux based OS distribution running on a x86 or aarch64 machine.
- git clone the edk2-stable202008 tag of [EDK2 tree](https://github.com/tianocore/edk2).
- git clone the [EDK2 port of libc](https://github.com/tianocore/edk2-libc) SHA: 61687168fe02ac4d933a36c9145fdd242ac424d1.
- Install GCC 5.3 or later toolchain for Linux from [here](https://releases.linaro.org/components/toolchain/binaries/).
- Install the build prerequisite packages to build EDK2. 
Note: The details of the packages are beyond the scope of this document.

To start the ACS build, perform the following steps:

1.  cd local_edk2_path
2.  git clone https://github.com/tianocore/edk2-libc
3.  git submodule update --init --recursive
4.  git clone https://github.com/ARM-software/sbsa-acs ShellPkg/Application/sbsa-acs
5.  Add the following to the [LibraryClasses.common] section in ShellPkg/ShellPkg.dsc
   - Add  SbsaValLib|ShellPkg/Application/sbsa-acs/val/SbsaValLib.inf
   - Add  SbsaPalLib|ShellPkg/Application/sbsa-acs/platform/pal_uefi/SbsaPalLib.inf
6.  Add ShellPkg/Application/sbsa-acs/uefi_app/SbsaAvs.inf in the [components] section of ShellPkg/ShellPkg.dsc

### Linux build environment
If the build environment is Linux, perform the following steps:
1.  export GCC49_AARCH64_PREFIX= GCC5.3 toolchain path pointing to /bin/aarch64-linux-gnu- in case of x86 machine. For aarch64 build it should point to /usr/bin/
2.  export PACKAGES_PATH= path pointing to edk2-libc
3.  source edksetup.sh
4.  make -C BaseTools/Source/C
5.  source ShellPkg/Application/sbsa-acs/tools/scripts/avsbuild.sh

### Windows build environment
If the build environment is Windows, perform the following steps:
1. Set the toolchain path to GCC53 or above.
2. Setup the environment for AARCH64 EDK2 build.
3. Setup the environment for PACKAGES_PATH.
4. Build the SBSA shell application.
   For example,
   build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m
   ShellPkg/Application/sbsa-acs/uefi_app/SbsaAvs.inf

**Note:** To build the ACS with NIST Statistical Test Suite, see [SBSA_NIST_User_Guide](docs/Arm_SBSA_NIST_User_Guide.md)

### Build output

The EFI executable file is generated at <edk2_path>/Build/Shell/DEBUG_GCC49/AARCH64/Sbsa.efi


## Test suite execution

The execution of the compliance suite varies depending on the test environment. These steps assume that the test suite is invoked through the ACS UEFI shell application.

For details about the SBSA ACS UEFI Shell application, see [SBSA ACS USER Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf)

### Prerequisites
- If the system supports LPI’s (Interrupt ID > 8192) then Firmware should have support for installing handler for LPI interrupts.
    - If you are using edk2, change the ArmGic driver in the ArmPkg to have support for installing handler for LPI’s.
    - Add the following in edk2/ArmPkg/Drivers/ArmGic/GicV3/ArmGicV3Dxe.c
>        - After [#define ARM_GIC_DEFAULT_PRIORITY  0x80]
>          +#define ARM_GIC_MAX_NUM_INTERRUPT 16384
>        - Change this in GicV3DxeInitialize Function.
>          -mGicNumInterrupts      = ArmGicGetMaxNumInterrupts (mGicDistributorBase);
>          +mGicNumInterrupts      = ARM_GIC_MAX_NUM_INTERRUPT;

### Post-Silicon

On a system where a USB port is available and functional, perform the following steps:

1. Copy 'Sbsa.efi' to a USB Flash drive.
2. Plug in the USB Flash drive to one of the functional USB ports on the system.
3. Boot the system to UEFI shell.
4. To determine the file system number of the plugged in USB drive, execute 'map -r' command. 
5. Type 'fsx' where 'x' is replaced by the number determined in step 4.
6. To start the compliance tests, run the executable Sbsa.efi with the appropriate parameters. 
   For details on the parameters, refer to [SBSA ACS User Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf)
7. Copy the UART console output to a log file for analysis and certification.


### Emulation environment with secondary storage
On an emulation environment with secondary storage, perform the following steps:

1. Create an image file which contains the 'Sbsa.efi' file. For Example:
  - mkfs.vfat -C -n HD0 hda.img 2097152
  - sudo mount -o rw,loop=/dev/loop0,uid=`whoami`,gid=`whoami` hda.img /mnt/sbsa. If loop0 is busy, specify the loop that is free
  - cp  "<path to application>/Sbsa.efi" /mnt/sbsa/
  - sudo umount /mnt/sbsa
2. Load the image file to the secondary storage using a backdoor. The steps followed to load the image file are Emulation environment specific and beyond the scope of this document. 
3. Boot the system to UEFI shell.
4. To determine the file system number of the secondary storage, execute 'map -r' command. 
5. Type 'fsx' where 'x' is replaced by the number determined in step 4.
6. To start the compliance tests, run the executable Sbsa.efi with the appropriate parameters. 
   For details on the parameters, see the [Arm SBSA ACS User Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf)
7. Copy the UART console output to a log file for analysis and certification.


### Emulation environment without secondary storage

On an Emulation platform where secondary storage is not available, perform the following steps:

1. Add the path to 'Sbsa.efi' file in the UEFI FD file.
2. Build UEFI image including the UEFI Shell.
3. Boot the system to UEFI shell.
4. Run the executable 'Sbsa.efi' to start the compliance tests. For details about the parameters,
   see the [SBSA ACS User Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf).
5. Copy the UART console output to a log file for analysis and certification.


## Linux OS-based tests
Certain PCIe and IOMMU tests require Linux operating system with kernel version 4.10 or above.
The procedure to build and run these tests is described in [SBSA ACS User Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf).

## Security implication
Arm Enterprise ACS test suite may run at higher privilege level. An attacker may utilize these tests as a means to elevate privilege which can potentially reveal the platform security assets. To prevent the leakage of secure information, it is strongly recommended that the ACS test suite is run only on development platforms. If it is run on production systems, the system should be scrubbed after running the test suite.

## Limitations
Validating the compliance of certain PCIe rules defined in the SBSA specification requires the PCIe end-point to generate specific stimulus during the runtime of the test. Examples of such stimulus are  P2P, PASID, ATC, etc. The tests that requires these stimuli are grouped together in the exerciser module. The exerciser layer is an abstraction layer that enables the integration of hardware capable of generating such stimuli to the test framework.
The details of the hardware or Verification IP which enable these exerciser tests are platform specific and are beyond the scope of this document.

 - Some PCIe and Exerciser test are dependent on PCIe features supported by the test system.
   Please fill the required API's with test system information.
   - pal_pcie_p2p_support : If the test system PCIe supports peer to peer transaction.
   - pal_pcie_is_cache_present : If the test system supports PCIe address translation cache.
   - pal_pcie_get_legacy_ir_map : Fill system legacy irq map.
   Below exerciser capabilities are required by exerciser test.
   - MSI-X interrupt generation.
   - Incoming Transaction Monitoring(order, type).
   - Initiating transacions from and to the exerciser.
   - Ability to check on BDF and register address seen for each configuration address along with access type.


## License
SBSA ACS is distributed under Apache v2.0 License.


## Feedback, contributions and support

 - For feedback, use the GitHub Issue Tracker that is associated with this repository.
 - For support, send an email to "support-enterprise-acs@arm.com" with details.
 - Arm licensees may contact Arm directly through their partner managers.
 - Arm welcomes code contributions through GitHub pull requests. See GitHub documentation on how to raise pull requests.

--------------

*Copyright (c) 2018-2021, Arm Limited and Contributors. All rights reserved.*
