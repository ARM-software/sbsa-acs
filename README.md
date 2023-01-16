
# Server Base System Architecture - Architecture Compliance Suite


## Server Base System Architecture
**Server Base System Architecture** (SBSA) specification specifies a hardware system architecture based on the Arm 64-bit architecture.  
Server system software such as operating systems, hypervisors, and firmware rely on this. It addresses processing element features and key aspects of system architecture.

For more information, download the [SBSA specification](https://developer.arm.com/documentation/den0029/h/?lang=en)


## SBSA - Architecture Compliance Suite

SBSA **Architecture Compliance Suite** (ACS) is a collection of self-checking, portable C-based tests.  
This suite includes a set of examples of the invariant behaviors that are provided by the [SBSA specification](https://developer.arm.com/documentation/den0029/h/?lang=en), so that implementers can verify if these behaviours have been interpreted correctly.
Most of the tests are executed from UEFI Shell by executing the SBSA UEFI shell application.  
A few tests are executed by running the SBSA ACS Linux application which in turn depends on the SBSA ACS Linux kernel module.  


## Release details
 - Code Quality: REL v7.1 BETA-0
 - The tests are written for version 7.1 of the SBSA specification.
 - The release includes test which are not verified due to platform dependency. Please refer to [testcase-checklist](docs/testcase-checklist.md) for these tests. 
 - The compliance suite is not a substitute for design verification.
 - To review the SBSA ACS logs, Arm licensees can contact Arm directly through their partner managers.
 - To know about the gaps in the test coverage, see [Test Scenario Document](docs/Arm_SBSA_Architecture_Compliance_Test_Scenario.pdf).


## GitHub branch
  - To pick up the release version of the code, checkout the corresponding tag from master branch.
  - To get the latest version of the code with bug fixes and new features, use the master branch.

## Additional reading
  - For details on the SBSA ACS UEFI Shell Application, Linux Application and PMU Linux Application see the [Arm SBSA ACS User Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf).
  - For details on the design of the SBSA ACS, see the [Arm SBSA Validation Methodology Document](docs/Arm_SBSA_Architecture_Compliance_Validation_Methodology.pdf).
  - For information about the test coverage scenarios that are implemented in the current release of ACS and the scenarios that are planned for the future releases, see the [Test Scenario Document](docs/Arm_SBSA_Architecture_Compliance_Test_Scenario.pdf).

## SBSA ACS Baremetal Reference Code
Bare-metal reference code is added as part of this release. For more information, see
  - [Arm SBSA ACS Bare-metal User Guide](platform/pal_baremetal/docs/Arm_SBSA_ACS_Bare-metal_User_Guide.pdf).
  - [Bare-metal Code](platform/pal_baremetal/). <br />
Note: The Baremetal PCIe enumeration code provided as part of the SBSA ACS should be used and should not be replaced. This code is vital in analyzing of the test result.

## SBSA ACS Linux kernel module
To enable the export of a few kernel APIs that are necessary for PCIe and IOMMU tests, Linux kernel module and a kernel patch file are required. These files are available at [linux-acs](https://gitlab.arm.com/linux-arm/linux-acs).

## Target platforms
  Any AARCH64 Enterprise Platform that boots UEFI and Linux OS.

## ACS build steps - UEFI Shell application

### Prebuilt images
Prebuilt images for v23_01_REL7.1.0_BETA-0 release is available in the prebuilt_images folder of the master branch. The prebuilt images of previous release are present in [release branch](https://github.com/chetan-rathore/sbsa-acs/tree/release/prebuilt_images). You can choose to use this images or build your own image by following the steps below. If you choose to use the prebuilt image, jump to the test suite execution section below for details on how to run the application.

### Prerequisites
Before starting the build, ensure that the following requirements are met.

- Any mainstream Linux based OS distribution running on a x86 or AArch64 machine.
- Install GCC-ARM 10.3 or later toolchain for Linux from [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads).
- git clone the edk2-stable202208 tag of [EDK2 tree](https://github.com/tianocore/edk2).
- git clone the [EDK2 port of libc](https://github.com/tianocore/edk2-libc) SHA: 61687168fe02ac4d933a36c9145fdd242ac424d1.
- Install the build prerequisite packages to build EDK2.<br />
Note:<br /> 
- The details of the packages are beyond the scope of this document.

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
1.  export GCC49_AARCH64_PREFIX= GCC 10.3 toolchain path pointing to /bin/aarch64-none-linux-gnu- in case of x86 machine.<br /> For AArch64 build it should point to /usr/bin/
2.  export PACKAGES_PATH= path pointing to edk2-libc
3.  source edksetup.sh
4.  make -C BaseTools/Source/C
5.  source ShellPkg/Application/sbsa-acs/tools/scripts/avsbuild.sh

**Note:** To build the ACS with NIST Statistical Test Suite, see the [SBSA_NIST_User_Guide](docs/Arm_SBSA_NIST_User_Guide.md)

### Build output

The EFI executable file is generated at <edk2_path>/Build/Shell/DEBUG_GCC49/AARCH64/Sbsa.efi


## Test suite execution

The execution of the compliance suite varies depending on the test environment. These steps assume that the test suite is invoked through the ACS UEFI shell application.

For details about the SBSA ACS UEFI Shell application, see [SBSA ACS USER Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf)

### Prerequisites
- If the system supports LPI’s (Interrupt ID > 8192), then Firmware should have support for installing handler for LPI interrupts.
    - If you are using edk2, change the ArmGic driver in the ArmPkg to obtain support for installing handler for LPI’s.
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
  - sudo mount -o rw,loop=/dev/loop0,uid=`whoami`,gid=`whoami` hda.img /mnt/sbsa.  
    If loop0 is busy, specify the loop that is free
  - cp  "<path to application>/Sbsa.efi" /mnt/sbsa/
  - sudo umount /mnt/sbsa
2. Load the image file to the secondary storage using a backdoor.    
   The steps followed to load the image file are emulation environment-specific and beyond the scope of this document. 
3. Boot the system to UEFI shell.
4. To determine the file system number of the secondary storage, execute 'map -r' command.
5. Type 'fsx' where 'x' is replaced by the number determined in step 4.
6. To start the compliance tests, run the executable Sbsa.efi with the appropriate parameters.    
   For details on the parameters, see the [SBSA ACS User Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf)
7. Copy the UART console output to a log file for analysis and certification.


### Emulation environment without secondary storage

On an emulation platform where secondary storage is not available, perform the following steps:

1. Add the path to 'Sbsa.efi' file in the UEFI FD file.
2. Build UEFI image including the UEFI Shell.
3. Boot the system to UEFI shell.
4. Run the executable 'Sbsa.efi' to start the compliance tests.    
   For details about the parameters, see the [SBSA ACS User Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf).
> shell> Sbsa.efi
5. Copy the UART console output to a log file for analysis and certification.


## ACS build steps - Linux application

Certain Peripheral, PCIe and Memory map tests require Linux operating system.This chapter provides information on building and executing these tests from the Linux application.

### 1. Build steps and environment setup
This section lists the porting and build steps for the kernel module.
The patch for the kernel tree and the Linux PAL are hosted separately on [linux-acs](https://gitlab.arm.com/linux-arm/linux-acs) repository.

### 1.1 Building the kernel module
#### Prerequisites
- Linux kernel source version 5.11, 5.13, 5.15, 6.0.
- Linaro GCC tool chain 10.3 or above.
- Build environment for AArch64 Linux kernel.<br />
NOTE: <br />
- Linux version 6.0 is recommened version.
- GCC 10.3 is recommended toolchain. 

#### Porting steps for Linux kernel
1. git clone https://gitlab.arm.com/linux-arm/linux-acs sbsa-acs-drv
2. git clone https://github.com/ARM-software/sbsa-acs.git sbsa-acs
3. git clone https://github.com/torvalds/linux.git -b v6.0
4. export CROSS_COMPILE=<GCC10.3 toolchain path> pointing to /bin/aarch64-linux-gnu-
5. git apply <local_dir>/sbsa-acs-drv/kernel/src/0001-BSA-ACS-Linux-6.0.patch to your kernel source tree.
6. make ARCH=arm64 defconfig && make -j $(nproc) ARCH=arm64

NOTE: The steps mentions Linux version 6.0, as it is latest version which is verified at ACS end.

#### 1.2 Build steps for SBSA kernel module
1. cd <local_dir>/sbsa-acs-drv/files
2. export CROSS_COMPILE=<ARM64 toolchain path>/bin/aarch64-linux-gnu-
3. export KERNEL_SRC=<linux kernel path>
4. ./setup.sh <local_dir/sbsa-acs>
5. ./linux_sbsa_acs.sh

Successful completion of above steps will generate sbsa_acs.ko

#### 1.3 SBSA Linux application build
1. cd <sbsa-acs path>/linux_app/sbsa-acs-app
2. export CROSS_COMPILE=<ARM64 toolchain path>/bin/aarch64-linux-gnu-
3. make

Successful completion of above steps will generate executable file sbsa

### 2. Loading the kernel module
Before the SBSA ACS Linux application can be run, load the SBSA ACS kernel module using the insmod command.
```sh
shell> insmod sbsa_acs.ko
```

### 3. Running SBSA ACS
```sh
shell> ./sbsa
```
  - For information on the SBSA Linux application parameters, see the [SBSA ACS User Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf).


## Security implication
Arm Enterprise ACS test suite may run at higher privilege level. An attacker may utilize these tests as a means to elevate privilege which can potentially reveal the platform security assets. To prevent the leakage of secure information, it is strongly recommended that the ACS test suite is run only on development platforms. If it is run on production systems, the system should be scrubbed after running the test suite.

## Limitations
Validating the compliance of certain PCIe rules defined in the SBSA specification requires the PCIe end-point to generate specific stimulus during the runtime of the test. Examples of such stimulus are  P2P, PASID, ATC, etc. The tests that requires these stimuli are grouped together in the exerciser module. The exerciser layer is an abstraction layer that enables the integration of hardware capable of generating such stimuli to the test framework.
The details of the hardware or Verification IP which enable these exerciser tests are platform specific and are beyond the scope of this document.

### SBSA Level 7 tests dependencies
 - MPAM test will require EL3 firmware to enable access to MPAM registers from lower EL's. 
   If arm trusted firmware is used as EL3 fimrware, enable ENABLE_MPAM_FOR_LOWER_ELS=1 in tf build
 - RAS test will require EL3 firmware to enable access to RAS registers from lower EL's and forward RAS related exceptions to lower EL's. 
   If arm trusted firmware is used as EL3 fimrware, enable EL3_EXCEPTION_HANDLING=1 RAS_EXTENSION=1 HANDLE_EA_EL3_FIRST=1 RAS_TRAP_LOWER_EL_ERR_ACCESS=0 in tf build
 - If the above flags are not enabled, MPAM and RAS test can lead to exception at EL3.
 - MPAM test will require system to implement MPAM, SRAT, HMAT, PPTT tables.
 - RAS test will require system to implement AEST, RAS2, SRAT, HMAT, PPTT tables.
 - PMU test will require system to implement APMT table.
 - Entrophy rule will require ACS to build with STS package
 
**Note:** To build the ACS with NIST Statistical Test Suite, see the [SBSA_NIST_User_Guide](docs/Arm_SBSA_NIST_User_Guide.md)

|APIs                         |Description                                                                   |Affected tests          |
|-----------------------------|------------------------------------------------------------------------------|------------------------|
|pal_pcie_p2p_support         |Return 0 if the test system PCIe supports peer to peer transaction, else 1    |453, 454, 456, 812, 813 |
|pal_pcie_is_cache_present    |Return 1 if the test system supports PCIe address translation cache, else 0   |452                     |
|pal_pcie_get_legacy_irq_map  |Return 0 if system legacy irq map is filled, else 1                           |412, 450, 806           |

   Below exerciser capabilities are required by exerciser test.
   - MSI-X interrupt generation.
   - Incoming Transaction Monitoring (order, type).
   - Initiating transactions from and to the exerciser.
   - Ability to check on BDF and register address seen for each configuration address along with access type.

 - SBSA Test 403 (Check ECAM Memory accessibility) execution time depends on the system PCIe hierarchy. For systems with multiple ECAMs the time taken to complete can be long which is normal. Please wait until the test completes.

## License
SBSA ACS is distributed under Apache v2.0 License.


## Feedback, contributions, and support

 - For feedback, use the GitHub Issue Tracker that is associated with this repository.
 - For support, send an email to "support-systemready-acs@arm.com" with details.
 - Arm licensees may contact Arm directly through their partner managers.
 - Arm welcomes code contributions through GitHub pull requests. See GitHub documentation on how to raise pull requests.

--------------

*Copyright (c) 2018-2023 Arm Limited and Contributors. All rights reserved.*
