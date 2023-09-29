
# Server Base System Architecture - Architecture Compliance Suite


## Server Base System Architecture
**Server Base System Architecture** (SBSA) specification specifies a hardware system architecture based on the Arm 64-bit architecture. Server system software such as operating systems, hypervisors, and firmware rely on this. It addresses processing element features and key aspects of system architecture.

For more information, download the [SBSA specification](https://developer.arm.com/documentation/den0029/h/?lang=en)


## SBSA - Architecture Compliance Suite

SBSA **Architecture Compliance Suite** (ACS) is a collection of self-checking, portable C-based tests.
This suite includes a set of examples of the invariant behaviors that are provided by the [SBSA](https://developer.arm.com/documentation/den0029/h/?lang=en) specification, so that implementers can verify if these behaviours have been interpreted correctly.
The tests are executed in a baremetal environment. The initialization of the baremetal environment is specific to the environment and is out of scope of this document.

## Release details
 - Code Quality: REL v7.1.2 EAC
 - The tests are written for version 7.1 of the SBSA specification.
 - The compliance suite is not a substitute for design verification.
 - To review the SBSA ACS logs, Arm licensees can contact Arm directly through their partner managers.
 - To know about the SBSA rules not implemented in this release, see [Test Scenario Document](docs/Arm_SBSA_Architecture_Compliance_Test_Scenario.pdf).

### EDA vendors
Contact your EDA vendor and ask if they include these tests as part of their verificatoin IP package.

## GitHub branch
  - To pick up the release version of the code, checkout the baremetal_reference branch.
  - To get the latest version of the code with bug fixes and new features, use the master branch.

## Additional reading
  - For details on the SBSA ACS test execution, see the [Arm SBSA ACS User Guide](docs/Arm_SBSA_ACS_Bare-metal_User_Guide.pdf).
  - For details on the Design of the SBSA ACS, see the [Arm SBSA Validation Methodology Document](docs/Arm_SBSA_Architecture_Compliance_Validation_Methodology.pdf).
  - For information about the test coverage scenarios that are implemented in the current release of ACS and the scenarios that are  planned for the future releases, see the [Testcase checklist](docs/Arm_SBSA_testcase-checklist.rst). <br />
Note: The Baremetal PCIe enumeration code provided as part of the SBSA ACS should be used and should not be replaced. This code is vital in analyzing of the test result.

### Running Exerciser tests for complete coverage

Exerciser is a client device wrapped up by PCIe Endpoint. This device is created to meet SBSA requirements for various PCIe capability validation tests. Running the Exerciser tests provides additional test coverage on the platform.

Note: To run the exerciser tests on a UEFI Based platform with Exerciser, the Exerciser PAL API's need to be implemented. For details on the reference Exerciser implementation and support, see the [Exerciser.md](docs/PCIe_Exerciser/Exerciser.md) and [Exerciser_API_porting_guide.md](docs/PCIe_Exerciser/Exerciser_API_porting_guide.md)

## Target platforms
  Any 64-bit Arm based Server design presented as a full chip Emulation or Simulation environment

## ACS build steps - UEFI Shell application

The baremetal build environment is platform specific. To provide a baseline, the build steps to integrate and run the tests from UEFI shell are provided here.


### Prebuilt images
Prebuilt images for each release are available in the prebuilt_images folder of the release branch. You can choose to use these images or build your own image by following the steps below. If you choose to use the prebuilt image, jump to the test suite execution section below for details on how to run the application.

### Prerequisites
    Before starting the ACS build, ensure that the following requirements are met.

- Any mainstream Linux based OS distribution running on a x86 or AArch64 machine.
- git clone the edk2-stable202208 branch of [EDK2 tree](https://github.com/tianocore/edk2).
- Install GCC 10.3 or later toolchain for Linux from [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads).
- Install the build prerequisite packages to build EDK2.


To start the ACS build, perform the following steps:
1.  git clone -b edk2-stable202208 https://github.com/tianocore/edk2.git
2.  cd edk2
3.  git clone https://github.com/tianocore/edk2-libc
4.  git submodule update --init --recursive
5.  git clone -b baremetal_reference https://github.com/ARM-software/sbsa-acs.git ShellPkg/Application/sbsa-acs
6.  Add the following to the [LibraryClasses.common] section in ShellPkg/ShellPkg.dsc
   - UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
   - SbsaValLib|ShellPkg/Application/sbsa-acs/val/SbsaValLib.inf
   - SbsaPalBaremetalLib|ShellPkg/Application/sbsa-acs/platform/pal_baremetal/SbsaPalBaremetalLib.inf
   - SbsaPalFVPLib|ShellPkg/Application/sbsa-acs/platform/pal_baremetal/FVP/RDN2/SbsaPalFVPLib.inf
7.  Add ShellPkg/Application/sbsa-acs/baremetal_app/SbsaAvs.inf in the [components] section of ShellPkg/ShellPkg.dsc
8.  Modify CC Flags in the [BuildOptions] section of ShellPkg/ShellPkg.dsc
```
      *_*_*_CC_FLAGS = -DTARGET_EMULATION -DENABLE_OOB

      !include StdLib/StdLib.inc
```
9.  Modify the following in the edk2-libc/StdLib/LibC/Main/Main.c
```
      -extern int main( int, char**);
      +extern int ShellAppMainsbsa( int, char**);
```
10.  Modify the following in ShellAppMain() of edk2-libc/StdLib/LibC/Main/Main.c
```
      -ExitVal = (INTN)main( (int)Argc, gMD->NArgV);
      +ExitVal = (INTN)ShellAppMainsbsa( (int)Argc, gMD->NArgV);
```
11.  Comment the map[] variable in edk2-libc/StdLib/LibC/Main/Arm/flt_rounds.c to avoid -werror=unused-variable
```
          +#if 0
          static const int map[] = {
          1,  /* round to nearest */
          2,  /* round to positive infinity */
          3,  /* round to negative infinity */
          0   /* round to zero */
          };
          +#endif
```

### Linux build environment
If the build environment is Linux, perform the following steps:
1.  export GCC49_AARCH64_PREFIX= GCC10.3 toolchain path pointing to /bin/aarch64-linux-gnu- in case of x86 machine. For AArch64 build it should point to /usr/bin/
2.  export PACKAGES_PATH= path pointing to edk2-libc
3.  source edksetup.sh
4.  make -C BaseTools/Source/C
5.  source ShellPkg/Application/sbsa-acs/tools/scripts/avsbuild.sh ENABLE_OOB



### Build output

The EFI executable file is generated at <edk2_path>/Build/Shell/DEBUG_GCC49/AARCH64/Sbsa.efi


## Test suite execution

The execution of the compliance suite varies depending on the test environment. These steps assume that the test suite is invoked through the ACS UEFI shell application.

For details about the SBSA ACS UEFI Shell application, see the [SBSA ACS USER Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf)

### On-Silicon

Executing the tests in a baremetal environment is platform specific. As a baseline, the following steps are provided to run the tests from UEFI shell on silicon.

On a system where a USB port is available and functional, perform the following steps:

1. Copy 'Sbsa.efi' to a USB Flash drive.
2. Plug in the USB Flash drive to one of the functional USB ports on the system.
3. Boot the system to UEFI shell.
4. To determine the file system number of the plugged in USB drive, execute 'map -r' command.
5. Type 'fsx' where 'x' is replaced by the number determined in step 4.
6. To start the compliance tests, run the executable Sbsa.efi with the appropriate parameters.
   For details on the parameters, refer to [SBSA ACS User Guide](docs/Arm_SBSA_Architecture_Compliance_User_Guide.pdf)

## Limitations
Validating the compliance of certain PCIe rules defined in the SBSA specification requires the PCIe end-point to generate specific stimulus during the runtime of the test. Examples of such stimulus are  P2P, PASID, ATC, etc. The tests that requires these stimuli are grouped together in the exerciser module. The exerciser layer is an abstraction layer that enables the integration of hardware capable of generating such stimuli to the test framework.
The details of the hardware or Verification IP which enable these exerciser tests are platform specific and are beyond the scope of this document.

 - Some PCIe and Exerciser test are dependent on PCIe features supported by the test system.
   Please fill the required API's with test system information.

|APIs                         |Description                                                                   |Affected tests          |
|-----------------------------|------------------------------------------------------------------------------|------------------------|
|pal_pcie_p2p_support         |Return 0 if the test system PCIe supports peer to peer transaction, else 1    |856                     |
|pal_pcie_is_cache_present    |Return 1 if the test system supports PCIe address translation cache, else 0   |852                     |
|pal_pcie_get_legacy_irq_map  |Return 0 if system legacy irq map is filled, else 1                           |850                  |

   Below exerciser capabilities are required by exerciser test.
   - MSI-X interrupt generation.
   - Incoming Transaction Monitoring(order, type).
   - Initiating transacions from and to the exerciser.
   - Ability to check on BDF and register address seen for each configuration address along with access type.

### SBSA ACS version mapping
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
|   SBSA Spec Version   |   ACS Coverage Mapping   |   SBSA ACS Version   |        SBSA Tag ID         |   BSA ACS Version   |      BSA Tag ID     |    Pre-Si Support    |
|-----------------------|:------------------------:|:--------------------:|:--------------------------:|:-------------------:|:-------------------:|:--------------------:|
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.2 EAC      |   v23.07_REL7.1.2          |        v1.0.5       |   v23.07_REL1.0.5   |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.1 BETA-1   |   v23.03_REL7.1.1_BETA-1   |        v1.0.4       |   v23.03_REL1.0.4   |       Yes            |
|       SBSA v7.1       |    BSA ACS + SBSA ACS    |      v7.1.0 BETA-0   |   v23.01_REL7.1.0_BETA-0   |        v1.0.3       |   v23.01_REL1.0.3   |       Yes            |
|       SBSA v6.1       |    BSA ACS + SBSA ACS    |      v6.1.0          |   v22.10_REL6.1.0          |        v1.0.2       |   v22.10_REL1.0.2   |       Yes            |
|       SBSA v6.0       |    SBSA ACS              |      v3.2            |   v22.07_REL3.2            |          -          |          -          |       Yes            |
|       SBSA v6.0       |    SBSA ACS              |      v3.1            |   v21.09_REL3.1            |          -          |          -          |       Yes            |
|       SBSA v5.0       |    SBSA ACS              |      v2.5            |   v20.08_RELv2.5           |          -          |          -          |       Yes            |
|       SBSA v3.0       |    SBSA ACS              |      v1.6            |   v18.12_REL1.7            |          -          |          -          |       No             |
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------

## License
SBSA ACS is distributed under Apache v2.0 License.


## Feedback, contributions and support

 - For feedback, use the GitHub Issue Tracker that is associated with this repository.
 - For support, send an email to "support-systemready-acs@arm.com" with details.
 - Arm licensees may contact Arm directly through their partner managers.
 - Arm welcomes code contributions through GitHub pull requests. See GitHub documentation on how to raise pull requests.

--------------

*Copyright (c) 2018-2023, Arm Limited and Contributors. All rights reserved.*
