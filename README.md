
# Server Base System Architecture - Architecture Compliance Suite


## Server Base System Architecture
**Server Base System Architecture** (SBSA) specification specifies a hardware system architecture based on the ARM 64-bit architecture. Server system software such as operating systems, hypervisors, and firmware rely on this. It addresses processing element features and key aspects of system architecture. 

For more information, download the [SBSA specification](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0029/index.html)


## SBSA - Architecture Compliance Suite

SBSA **Architecture Compliance Suite** (ACS) is a collection of self-checking, portable C-based tests.
This suite includes a set of examples of the invariant behaviours that are provided by the [SBSA](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0029/index.html) specification, so that implementers can verify if these behaviours have been interpreted correctly.
Most of the tests are executed from UEFI Shell by executing the SBSA UEFI shell application.
A few tests are executed by running the SBSA ACS Linux application which in turn depends on the SBSA ACS Linux kernel module.


## Release details
 - Code Quality: REL v1.0
 - The tests are written for version 3.0 of the SBSA specification.
 - The compliance suite is not a substitute for design verification.
 - To review the SBSA ACS logs, ARM licensees can contact ARM directly through their partner managers.
 - To know about the gaps in the test coverage, see [Testcase checklist](docs/testcase-checklist.md).


## GitHub branch
  - To pick up the release version of the code, checkout the release branch.
  - To get the latest version of the code with bug fixes and new features, use the master branch.

## Additional reading
  - For details on the SBSA ACS UEFI Shell Application, see [SBSA ACS User Guide](docs/SBSA_ACS_User_Guide.pdf).
  - For details on the Design of the SBSA ACS, see [Validation Methodology Document](docs/SBSA_Val_Methodolgy.pdf).
  - For information about the test coverage scenarios that are implemented in the current release of ACS and the scenarios that are   planned for the future releases, see [Testcase checklist](docs/testcase-checklist.md).


## SBSA ACS Linux kernel module
To enable the export of a few kernel APIs that are necessary for PCIe and IOMMU tests, Linux kernel module and a kernel patch file are required. These files are available at [linux-arm.org/git](http://www.linux-arm.org/git?p=linux-acs.git).

## Target platforms
  Any AARCH64 Enterprise Platform that boots UEFI and Linux OS.

## ACS build steps - UEFI Shell application

### Prebuilt images
Prebuilt images for each release are available in the prebuilt_images folder of the release branch. You can choose to use these images or build your own image by following the steps below. If you choose to use the prebuilt image, please jump to the test suite execution section below for details on how to run the application.

### Prerequisites
    Before starting the ACS build, ensure that the following requirements are met.

- Any mainstream Linux based OS distribution.
- git clone [EDK2 tree](https://github.com/tianocore/edk2).
- Install GCC 5.3 or later toolchain for Linux from [here](https://releases.linaro.org/components/toolchain/binaries/).
- Install the build prerequisite packages to build EDK2. 
Note: The details of the packages are beyond the scope of this document.

To start the ACS build, perform the following steps:

1.  cd local_edk2_path
2.  git clone https://github.com/ARM-software/sbsa-acs AppPkg/Applications/sbsa-acs
3.  Add the following to the [LibraryClasses.common] section in ShellPkg/ShellPkg.dsc
   - Add  SbsaValLib|AppPkg/Applications/sbsa-acs/val/SbsaValLib.inf
   - Add  SbsaPalLib|AppPkg/Applications/sbsa-acs/platform/pal_uefi/SbsaPalLib.inf
4.  Add AppPkg/Applications/sbsa-acs/uefi_app/SbsaAvs.inf in the [components] section of ShellPkg/ShellPkg.dsc

### Linux build environment
If the build environment is Linux, perform the following steps:
1.  export GCC49_AARCH64_PREFIX= GCC5.3 toolchain path pointing to /bin/aarch64-linux-gnu-
2.  source edksetup.sh
3.  make -C BaseTools/Source/C
4.  source AppPkg/Applications/sbsa-acs/tools/scripts/avsbuild.sh

### Windows build environment
If the build environment is Windows, perform the following steps:
1. Set the toolchain path to GCC53 or above.
2. Setup the environment for AARCH64 EDK2 build.
3. Build the SBSA shell application.
   For example,
   build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m
   AppPkg/Applications/sbsa-acs/uefi_app/SbsaAvs.inf

### Build output

The EFI executable file is generated at 
edk2_path /Build/Shell/DEBUG_GCC49/AARCH64/Sbsa.efi


## Test suite execution

The execution of the compliance suite varies depending on the test environment. These steps assume that the test suite is invoked through the ACS UEFI shell application.

For details about the SBSA ACS UEFI Shell application, see [SBSA ACS USER Guide](docs/SBSA_ACS_User_Guide.pdf)

### Post-Silicon

On a system where a USB port is available and functional, perform the following steps:

1. Copy 'Sbsa.efi' to a USB Flash drive.
2. Plug in the USB Flash drive to one of the functional USB ports on the system.
3. Boot the system to UEFI shell.
4. To determine the file system number of the plugged in USB drive, execute 'map -r' command. 
5. Type 'fsx' where 'x' is replaced by the number determined in step 4.
6. To start the compliance tests, run the executable Sbsa.efi with the appropriate parameters. 
   For details on the parameters, refer to [SBSA ACS User Guide](docs/SBSA_ACS_User_Guide.pdf)
7. Copy the UART console output to a log file for analysis and certification.


### Emulation environment with secondary storage
On an emulation environment with secondary storage, perform the following steps:

1. Create an image file which contains the 'Sbsa.efi' file. For Example:
  - mkfs.vfat -C -n HD0 hda.img 2097152
  - sudo mount -o rw,loop=/dev/loop0,uid=`whoami`,gid=`whoami` hda.img /mnt/sbsa
  - cp  "<path to application>/Sbsa.efi" /mnt/sbsa/
  - sudo umount /mnt/sbsa
2. Load the image file to the secondary storage using a backdoor. The steps followed to load the image file are Emulation environment specific and beyond the scope of this document. 
3. Boot the system to UEFI shell.
4. To determine the file system number of the secondary storage, execute 'map -r' command. 
5. Type 'fsx' where 'x' is replaced by the number determined in step 4.
6. To start the compliance tests, run the executable Sbsa.efi with the appropriate parameters. 
   For details on the parameters, see [SBSA ACS User Guide](docs/SBSA_ACS_User_Guide.pdf)
7. Copy the UART console output to a log file for analysis and certification.


### Emulation environment without secondary storage

On an Emulation platform where secondary storage is not available, perform the following steps:

1. Add the path to 'Sbsa.efi' file in the UEFI FD file.
2. Build UEFI image including the UEFI Shell.
3. Boot the system to UEFI shell.
4. Run the executable 'Sbsa.efi' to start the compliance tests. For details about the parameters, see [SBSA ACS User Guide](docs/SBSA_ACS_User_Guide.pdf).
5. Copy the UART console output to a log file for analysis and certification.


## Linux OS-based tests
Certain PCIe and IOMMU tests require Linux operating system with kernel version 4.10 or above. The procedure to build and run these tests is described in [SBSA ACS User Guide](docs/SBSA_ACS_User_Guide.pdf).


## License
SBSA ACS is distributed under Apache v2.0 License.


## Feedback, contributions and support

 - For feedback, use the GitHub Issue Tracker that is associated with this repository.
 - For support, please send an email to "support-enterprise-acs@arm.com" with details.
 - ARM licensees can contact ARM directly through their partner managers.
 - ARM welcomes code contributions through GitHub pull requests. See GitHub documentation on how to raise pull requests.
