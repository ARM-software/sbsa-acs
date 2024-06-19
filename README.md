
# Server Base System Architecture - Architecture Compliance Suite

[![SBSA-ACS Build Check](https://github.com/ARM-software/sbsa-acs/actions/workflows/sbsa-acs_build_check.yml/badge.svg)](https://github.com/ARM-software/sbsa-acs/actions/workflows/sbsa-acs_build_check.yml)

## Server Base System Architecture
**Server Base System Architecture** (SBSA) specification specifies a hardware system architecture based on the Arm 64-bit architecture.
Server system software such as operating systems, hypervisors, and firmware rely on this. It addresses processing element features and key aspects of system architecture.

For more information, download the [SBSA specification](https://developer.arm.com/documentation/den0029/h/?lang=en)


## SBSA - Architecture Compliance Suite

SBSA **Architecture Compliance Suite** (ACS) is a collection of self-checking, portable C-based tests.
This suite includes a set of examples of the invariant behaviors that are provided by the [SBSA specification](https://developer.arm.com/documentation/den0029/h/?lang=en), so that implementers can verify if these behaviours have been interpreted correctly.

Most of the tests are executed from UEFI Shell by executing the SBSA UEFI shell application.
A few tests are executed by running the SBSA ACS Linux application which in turn depends on the SBSA ACS Linux kernel module.
The tests can also be executed in a Bare-metal environment. The initialization of the Bare-metal environment is specific to the environment and is out of scope of this document.

## Release details
 - Code Quality: REL v7.2.0 BETA-0
 - The tests are written for version 7.1 of the SBSA specification.
 - For complete coverage of the SBSA rules, availability of an Exerciser is required for Exerciser tests to be run during verficiation at Pre-Silicon level.
 - For complete coverage, both SBSA and BSA ACS should be run.
 - The SBSA specification compliments the [BSA specification](https://developer.arm.com/documentation/den0094/c/?lang=en).
 - The tests can be run at both the Pre-Silicon and Silicon level.
 - The compliance suite is not a substitute for design verification.
 - To review the SBSA ACS logs, Arm licensees can contact Arm directly through their partner managers.
 - To know about the SBSA rules not implemented in this release, see [Test Scenario Document](docs/arm_sbsa_architecture_compliance_test_scenario.pdf).


## GitHub branch
  - To pick up the release version of the code, checkout the corresponding tag from the master branch.
  - To get the latest version of the code with bug fixes and new features, use the master branch.

## Additional reading
  - For information about the implementable SBSA rules test algorithm and for unimplemented SBSA rules, see the [arm SBSA Test Scenario Document](docs/arm_sbsa_architecture_compliance_test_scenario.pdf).
  - For information on test category(UEFI, Linux, Bare-metal) and applicable systems(SR,Pre-Silicon), see [arm SBSA Test Checklist Document](docs/arm_sbsa_testcase_checklist.rst)
  - For details on the design of the SBSA ACS, see the [arm SBSA Validation Methodology Document](docs/arm_sbsa_architecture_compliance_validation_methodology.pdf).
  - For details on the SBSA ACS UEFI Shell Application, Linux Application and PMU Linux Application see the [arm SBSA User Guide Document](docs/arm_sbsa_architecture_compliance_user_guide.pdf).
  - For details on the SBSA ACS Bare-metal support, see the
    - [arm SBSA Bare-metal User Guide Document](docs/arm_sbsa_architecture_compliance_bare-metal_user_guide.pdf).
    - [Bare-metal Code](https://github.com/ARM-software/bsa-acs/tree/main/pal/baremetal). <br />
Note: The Bare-metal PCIe enumeration code provided as part of the SBSA ACS should be used and should not be replaced. This code is vital in analyzing of the test result.

### Running Exerciser tests for complete coverage

Exerciser is a client device wrapped up by PCIe Endpoint. This device is created to meet SBSA requirements for various PCIe capability validation tests. Running the Exerciser tests provides additional test coverage on the platform.

Note: To run the exerciser tests on a UEFI Based platform with Exerciser, the Exerciser PAL API's need to be implemented. For details on the reference Exerciser implementation and support, see the [Exerciser.md](docs/PCIe_Exerciser/Exerciser.md) and [Exerciser_API_porting_guide.md](docs/PCIe_Exerciser/Exerciser_API_porting_guide.md)

## SBSA ACS Linux kernel module
To enable the export of a few kernel APIs that are necessary for PCIe and SMMU tests, Linux kernel module and a kernel patch file are required. These files are available at [linux-acs](https://gitlab.arm.com/linux-arm/linux-acs).

## Target platforms
  Any AARCH64 Enterprise Platform that boots UEFI and Linux OS.


## ACS build steps - UEFI Shell application

This section details the steps to build standalone SBSA UEFI application and also to build full SBSA image, which includes UEFI, Linux and PMU tests.

### Prebuilt images
For each release prebuilt images are available in the prebuilt_images folder of the master branch.
You can choose to use these images or build your own image by following the steps below.
If you choose to use the prebuilt image, jump to the test suite execution section below for details on how to run the application.


### Prerequisites
Before starting the build, ensure that the following requirements are met.

- Any mainstream Linux based OS distribution running on a x86 or AArch64 machine.
- Install GCC-ARM 13.2 [toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).
- Install the build prerequisite packages to build EDK2.
Note: The details of the packages are beyond the scope of this document.

### 1. Build Steps

1.  git clone the edk2-stable202402 tag of EDK2 tree
> git clone --recursive --branch edk2-stable202402 https://github.com/tianocore/edk2
2.  git clone the EDK2 port of libc
> git clone https://github.com/tianocore/edk2-libc edk2/edk2-libc
3.  git clone sbsa-acs
> git clone https://github.com/ARM-software/sbsa-acs edk2/ShellPkg/Application/sbsa-acs
4. git clone bsa-acs
> git clone https://github.com/ARM-software/bsa-acs.git edk2/ShellPkg/Application/bsa-acs
5.  Add the following to the [LibraryClasses.common] section in edk2/ShellPkg/ShellPkg.dsc
> SbsaValLib|ShellPkg/Application/bsa-acs/val/SbsaValLib.inf

> SbsaPalLib|ShellPkg/Application/bsa-acs/pal/uefi_acpi/SbsaPalLib.inf
6.  Add the following to the [components] section of edk2/ShellPkg/ShellPkg.dsc
> ShellPkg/Application/sbsa-acs/uefi_app/SbsaAvs.inf

### 1.1 On Linux build environment, perform the following steps:
- On x86 machine
> wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-linux-gnu.tar.xz

> tar -xf arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-linux-gnu.tar.xz

> export GCC49_AARCH64_PREFIX= GCC 13.2 toolchain path pointing to arm-gnu-toolchain-13.2.rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-

- On AArch64 machine
> export GCC49_AARCH64_PREFIX=/usr/bin

- From inside the edk2 directory in console

> export PACKAGES_PATH=$PWD/edk2-libc

> source edksetup.sh

> make -C BaseTools/Source/C

> source ShellPkg/Application/sbsa-acs/tools/scripts/acsbuild.sh
### 2. Build output

The EFI executable file is generated at <edk2_path>/Build/Shell/DEBUG_GCC49/AARCH64/Sbsa.efi

### 3. Execution Steps

The execution of the compliance suite varies depending on the test environment.
These steps assume that the test suite is invoked through the ACS UEFI shell application.

#### Firmware Dependencies
- If the system supports LPI’s (Interrupt ID > 8192), then Firmware should have support for installing handler for LPI interrupts.
    - If you are using edk2, change the ArmGic driver in the ArmPkg to obtain support for installing handler for LPI’s.
    - Add the following in edk2/ArmPkg/Drivers/ArmGic/GicV3/ArmGicV3Dxe.c
>        - After [#define ARM_GIC_DEFAULT_PRIORITY  0x80]
>          +#define ARM_GIC_MAX_NUM_INTERRUPT 16384
>        - Change this in GicV3DxeInitialize Function.
>          -mGicNumInterrupts      = ArmGicGetMaxNumInterrupts (mGicDistributorBase);
>          +mGicNumInterrupts      = ARM_GIC_MAX_NUM_INTERRUPT;

### 3.1 On Silicon
On a system where a USB port is available and functional, perform the following steps:

1. Copy 'Sbsa.efi' to a USB Flash drive. Path for 'Sbsa.efi' is present in step 2.
2. Plug in the USB Flash drive to one of the functional USB ports on the system.
3. Boot the system to UEFI shell.
4. To determine the file system number of the plugged in USB drive, execute 'map -r' command.
5. Type 'fsx' where 'x' is replaced by the number determined in step 4.
6. To start the compliance tests, run the executable Sbsa.efi with the appropriate parameters.
   For details on the parameters, refer to [arm SBSA User Guide Document](docs/arm_sbsa_architecture_compliance_user_guide.pdf)
> shell> Sbsa.efi
7. Copy the UART console output to a log file for analysis and certification.


### 3.2 Emulation environment with secondary storage
On an emulation environment with secondary storage, perform the following steps:

1. Create an image file which contains the 'Sbsa.efi' file. For Example:
  - mkfs.vfat -C -n HD0 \<name of image\>.img 2097152.
    Here 2097152 is the size of the image.
  - sudo mount -o rw,loop=/dev/loop0,uid=`whoami`,gid=`whoami` \<name of image\>.img /mnt/sbsa.
    If loop0 is busy, specify the loop that is free
  - cp  "\<path to application\>/Sbsa.efi" /mnt/sbsa/
  - sudo umount /mnt/sbsa
2. Load the image file to the secondary storage using a backdoor.
   The steps followed to load the image file are emulation environment-specific and beyond the scope of this document.
3. Boot the system to UEFI shell.
4. To determine the file system number of the secondary storage, execute 'map -r' command.
5. Type 'fsx' where 'x' is replaced by the number determined in step 4.
6. To start the compliance tests, run the executable Sbsa.efi with the appropriate parameters.
   For details on the parameters, see the [arm SBSA User Guide Document](docs/arm_sbsa_architecture_compliance_user_guide.pdf)
> shell> Sbsa.efi
7. Copy the UART console output to a log file for analysis and certification.


### 3.3 Emulation environment without secondary storage
On an emulation platform where secondary storage is not available, perform the following steps:

1. Add the path to 'Sbsa.efi' file in the UEFI FD file.
2. Build UEFI image including the UEFI Shell.
3. Boot the system to UEFI shell.
4. Run the executable 'Sbsa.efi' to start the compliance tests.
   For details about the parameters, see the [arm SBSA User Guide Document](docs/arm_sbsa_architecture_compliance_user_guide.pdf).
> shell> Sbsa.efi
5. Copy the UART console output to a log file for analysis and certification.


## ACS build steps - Linux application

Certain PCIe and SMMU tests require Linux operating system.This chapter provides information on building and executing these tests from the Linux application.

### 1. Build steps and environment setup
This section lists the porting and build steps for the kernel module.
The patch for the kernel tree and the Linux PAL are hosted separately on [linux-acs](https://gitlab.arm.com/linux-arm/linux-acs) repository.

#### Prerequisites
- Linux kernel source version 5.11, 5.13, 5.15, 6.0, 6.4, 6.7
- Install GCC-ARM 13.2 [toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).
- Build environment for AArch64 Linux kernel.<br />

NOTE: <br />
Linux version 6.7 is the recommended version.

### 1.1 Building the kernel module
Download the below repository to the local <workspace_dir> directory
> git clone https://gitlab.arm.com/linux-arm/linux-acs

> git clone https://github.com/ARM-software/sbsa-acs.git

> git clone https://github.com/ARM-software/bsa-acs.git

> git clone https://github.com/torvalds/linux.git -b v6.7

1. export CROSS_COMPILE=<GCC13.2 or higher toolchain path> pointing to /bin/aarch64-linux-gnu-
2. cd <workspace_dir>/linux
3. git apply <workspace_dir>/linux-acs/kernel/src/0001-BSA-ACS-Linux-6.7.patch to your kernel source tree.
4. make ARCH=arm64 defconfig && make -j $(nproc) ARCH=arm64

Successful completion of above steps will generate **Image**  in <workspace_dir>/linux/arch/arm64/boot/

NOTE: The steps mentions Linux version 6.7, as it is latest version which is verified at ACS end.

#### 1.2 Build steps for SBSA kernel module
1. cd <workspace_dir>/linux-acs/acs-drv/files
2. export CROSS_COMPILE=<GCC13.2 or higher toolchain path> pointing to /bin/aarch64-linux-gnu-
3. export KERNEL_SRC=<workspace_dir>/linux
4. ./sbsa_setup.sh <local_dir/bsa-acs> <local_dir/sbsa-acs>
5. ./linux_sbsa_acs.sh

Successful completion of above steps will generate **sbsa_acs.ko**  in <workspace_dir>/linux-acs/acs-drv/files

#### 1.3 SBSA Linux application build
1. cd <workspace_dir>/sbsa-acs/linux_app/sbsa-acs-app
2. export CROSS_COMPILE=<ARM64 toolchain path>/bin/aarch64-linux-gnu-
3. make

Successful completion of above steps will generate executable file **sbsa**  in <workspace_dir>/sbsa-acs/linux_app/sbsa-acs-app

### 2. Loading the kernel module
Before the SBSA ACS Linux application can be run, load the SBSA ACS kernel module using the insmod command.
```sh
shell> insmod sbsa_acs.ko
```

### 3. Running SBSA ACS
```sh
shell> ./sbsa
```
  - For information on the SBSA Linux application parameters, see the [User Guide](docs/arm_sbsa_architecture_compliance_user_guide.pdf).

## ACS build steps - Bare-metal abstraction

The Bare-metal build environment is platform specific.

To execute the Bare-metal code from UEFI Shell, checkout to [bare-metal](https://github.com/ARM-software/sbsa-acs/tree/baremetal_reference) branch of SBSA and the build steps to integrate and run the same from UEFI shell are provided in the [README.md](https://github.com/ARM-software/sbsa-acs/blob/baremetal_reference/README.md)

For generating SBSA binary to run on Bare-metal environment, perform the following steps
1. cd sbsa-acs
2. export CROSS_COMPILE=<path_to_the_toolchain>/bin/aarch64-none-elf-
3. mkdir build
4. cd build
5. cmake ../ -G"Unix Makefiles" -DCROSS_COMPILE=$CROSS_COMPILE -DTARGET="Target platform" -DBSA_DIR=<bsa-acs_path>
6. make

Note: Reference Cmake file for SBSA is present at [CMakeLists.txt](CMakeLists.txt).

*Recommended*: CMake v3.17, GCC v12.2
```
CMake Command Line Options:
 -DARM_ARCH_MAJOR = Arch major version. Default value is 9.
 -DARM_ARCH_MINOR = Arch minor version. Default value is 0.
 -DCROSS_COMPILE  = Cross compiler path
 -DTARGET         = Target platform. Should be same as folder under baremetal/target/
 -DBSA_DIR        = BSA path for SBSA compilation
```

On a successful build, *.bin, *.elf, *.img and debug binaries are generated at *build/output* directory. The output library files will be generated at *build/tools/cmake/* of the sbsa-acs directory.

For more details on running the generated binaries on Bare-metal environment, refer [README.md](https://github.com/ARM-software/bsa-acs/tree/main/pal/baremetal/README.md)

## Security implication
SBSA ACS test suite may run at higher privilege level. An attacker may utilize these tests as a means to elevate privilege which can potentially reveal the platform security assets. To prevent the leakage of secure information, it is strongly recommended that the ACS test suite is run only on development platforms. If it is run on production systems, the system should be scrubbed after running the test suite.

## Limitations
Validating the compliance of certain PCIe rules defined in the SBSA specification requires the PCIe end-point to generate specific stimulus during the runtime of the test. Examples of such stimulus are  P2P, PASID, ATC, etc. The tests that requires these stimuli are grouped together in the exerciser module. The exerciser layer is an abstraction layer that enables the integration of hardware capable of generating such stimuli to the test framework.
The details of the hardware or Verification IP which enable these exerciser tests are platform specific and are beyond the scope of this document.

### SBSA Tests dependencies
 - MPAM tests will require EL3 firmware to enable access to MPAM registers from lower EL's.
   If arm trusted firmware is used as EL3 fimrware, enable ENABLE_MPAM_FOR_LOWER_ELS=1 during arm TF build.
   If the above flags are not enabled, MPAM tests can lead to exception at EL3.
 - RAS test will require EL3 firmware to enable access to RAS registers from lower EL's and forward RAS related exceptions to lower EL's.
   If arm trusted firmware is used as EL3 fimrware, enable EL3_EXCEPTION_HANDLING=1 RAS_EXTENSION=1 HANDLE_EA_EL3_FIRST=1 RAS_TRAP_LOWER_EL_ERR_ACCESS=0 during arm TF build
   If the above flags are not enabled, RAS tests can lead to exception at EL3.
 - SBSA Future Requirements ETE test will require EL3 firmware to enable access to Trace registers from lower EL's.
   If arm trusted firmware is used as EL3 fimrware, ENABLE_TRF_FOR_NS=1 ENABLE_TRBE_FOR_NS=1 ENABLE_SYS_REG_TRACE_FOR_NS=1 during arm TF build
   If the above flags are not enabled, ETE tests can lead to exception at EL3.
 - MPAM test have dependency on MPAM, SRAT, HMAT, PPTT tables.
 - RAS test have dependency on AEST, RAS2, SRAT, HMAT, PPTT tables.
 - PMU test have dependency on APMT table.
 - Entrophy rule will require ACS to build with NIST-STS package

**Note:** To build the ACS with NIST Statistical Test Suite, see the [arm SBSA_NIST_User_Guide Document](docs/arm_sbsa_nist_user_guide.md)

|APIs                         |Description                                                                   |Affected tests          |
|-----------------------------|------------------------------------------------------------------------------|------------------------|
|pal_pcie_dev_p2p_support     |Return 0 if the test system PCIe supports peer to peer transaction, else 1    |856, 857                |
|pal_pcie_is_cache_present    |Return 1 if the test system supports PCIe address translation cache, else 0   |852                     |
|pal_pcie_get_legacy_irq_map  |Return 0 if system legacy irq map is filled, else 1                           |850                     |

   Below exerciser capabilities are required by exerciser test.
   - MSI-X interrupt generation.
   - Incoming Transaction Monitoring (order, type).
   - Initiating transactions from and to the exerciser.
   - Ability to check on BDF and register address seen for each configuration address along with access type.

 - SBSA Test 803 (Check ECAM Memory accessibility) execution time depends on the system PCIe hierarchy. For systems with multiple ECAMs the time taken to complete can be long which is normal. Please wait until the test completes.

### SBSA ACS version mapping
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
|   SBSA Spec Version   |   ACS Coverage Mapping   |   SBSA ACS Version   |        SBSA Tag ID         |   BSA ACS Version   |          BSA Tag ID         |    Pre-Si Support    |
|-----------------------|:------------------------:|:--------------------:|:--------------------------:|:-------------------:|:---------------------------:|:--------------------:|
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.2.0 BETA-0   |   v24.03_REL7.2.0_BETA-0   |        v1.0.8       | v24.03_SBSA_REL7.2.0_BETA-0 |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.5          |   v24.03_REL7.1.5          |        v1.0.8       |       v24.03_REL1.0.8       |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.4          |   v23.12_REL7.1.4          |        v1.0.7       |       v23.12_REL1.0.7       |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.3          |   v23.11_BootFramework     |        v1.0.6       |    v23.11_BootFramework     |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.3          |   v23.09_REL7.1.3          |        v1.0.6       |       v23.09_REL1.0.6       |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.2          |   v23.07_REL7.1.2          |        v1.0.5       |       v23.07_REL1.0.5       |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.1 BETA-1   |   v23.03_REL7.1.1_BETA-1   |        v1.0.4       |       v23.03_REL1.0.4       |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.0 BETA-0   |   v23.01_REL7.1.0_BETA-0   |        v1.0.3       |       v23.01_REL1.0.3       |       Yes            |
|       SBSA v6.1       |    BSA ACS + SBSA ACS    |      v6.1.0          |   v22.10_REL6.1.0          |        v1.0.2       |       v22.10_REL1.0.2       |       Yes            |
|       SBSA v6.0       |    SBSA ACS              |      v3.2            |   v22.07_REL3.2            |          -          |              -              |       Yes            |
|       SBSA v6.0       |    SBSA ACS              |      v3.1            |   v21.09_REL3.1            |          -          |              -              |       Yes            |
|       SBSA v5.0       |    SBSA ACS              |      v2.5            |   v20.08_RELv2.5           |          -          |              -              |       Yes            |
|       SBSA v3.0       |    SBSA ACS              |      v1.6            |   v18.12_REL1.7            |          -          |              -              |       No             |
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

## License
SBSA ACS is distributed under Apache v2.0 License.


## Feedback, contributions, and support

 - For feedback, use the GitHub Issue Tracker that is associated with this repository.
 - For support, send an email to "support-systemready-acs@arm.com" with details.
 - Arm licensees may contact Arm directly through their partner managers.
 - Arm welcomes code contributions through GitHub pull requests. See GitHub documentation on how to raise pull requests.

--------------

*Copyright (c) 2018-2024 Arm Limited and Contributors. All rights reserved.*
