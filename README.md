
# Server Base System Architecture - Architecture Compliance Suite


## Server Base System Architecture (SBSA)
The **Server Base Systems Architecture** (SBSA) specification specifies a hardware system architecture, based on ARM 64-bit architecture, which server system software, such as operating systems, hypervisors and firmware can rely on. It addresses PE features and key aspects of system architecture. 

For more information, download the [SBSA specification](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0029/index.html)


## SBSA - Architecture Compliance Suite (ACS)

SBSA **Architecture compliance suite** is a collection of self-checking, portable C-based tests.
These tests check an implementation for compliance against SBSA specification version 3.0.
Amajority of the tests are executed from UEFI Shell by executing the SBSA UEFI shell application.
A few tests are executed by running the SBSA Linux application which in turn depends on the SBSA Linux kernel module.


## Release details
 - Code Quality  : Beta
 - Test results should not be taken as a true indication of compliance; there may be false positives/negetives.
 - Test coverage is not complete yet. See the [Testcase checklist](docs/testcase-checklist.md) for details.

1. For details on the SBSA ACS UEFI Shell Application, refer to the [SBSA ACS User Guide](docs/SBSA_ACS_User_Guide.pdf)
2. For more details on the Design of the SBSA ACS, refer to the [Validation Methodology Document](docs/SBSA_Val_Methodolgy.pdf)
3. For details on the test coverage, the scenarios implemented in the present release of the ACS and the scenarios planned in the future release, refer to the [Testcase checklist](docs/testcase-checklist.md)

## Github branch
  - To pick up the release version of the code, checkout the release branch.
  - To get the latest version of the code with bug fixes and new features, use the master branch.

### SBSA ACS Linux kernel module
  - The Linux kernel module files and the kernel patch file to enable the export of a few kernel APIs needed for the PCIe and IOMMU tests are located at [linux-arm.org/git](http://www.linux-arm.org/git?p=linux-acs.git).

## Target platforms
  Any AARCH64 enterprise Platform which boots UEFI and Linux OS.

### Prerequisites

1. Any mainstream Linux based OS distribution
2. git clone [EDK2 tree](https://github.com/tianocore/edk2)
3. Install GCC 5.3 or later toolchain for Linux from [here](https://releases.linaro.org/components/toolchain/binaries/)
4. Install the Build pre-requisite packages to build EDK2. The details are beyond the scope of this document.


### ACS build steps - UEFI Shell application

1.  cd local_edk2_path
2.  git clone https://github.com/ARM-software/sbsa-acs AppPkg/Applications/sbsa-acs
3.  Add the following to the [LibraryClasses.common] section in ShellPkg/ShellPkg.dsc
   - Add  SbsaValLib|AppPkg/Applications/sbsa-acs/val/SbsaValLib.inf
   - Add  SbsaPalLib|AppPkg/Applications/sbsa-acs/platform/pal_uefi/SbsaPalLib.inf
4.  Add AppPkg/Applications/sbsa-acs/uefi_app/SbsaAvs.inf in the [components] section of ShellPkg/ShellPkg.dsc

#### Linux build environment
1.  export GCC49_AARCH64_PREFIX= GCC5.3 toolchain path pointing to /bin/aarch64-linux-gnu-
2.  source edksetup.sh
3.  make -C BaseTools/Source/C
4.  source AppPkg/Applications/sbsa-acs/tools/scripts/avsbuild.sh

#### Windows build environment
1. Set the toolchain path to GCC53 or above.
2. Setup the environment for AARCH64 EDK2 build.
3. Build the SBSA shell application. Example: build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m
   AppPkg/Applications/sbsa-acs/uefi_app/SbsaAvs.inf

### Build output

The EFI executable file is generated at 
edk2_path /Build/Shell/DEBUG_GCC49/AARCH64/Sbsa.efi


## Test suite execution

The compliance suite execution will vary depending on the Test environment. The below steps assume that the test suite is invoked via the UEFI shell application which is provided as part of the Kit.

For more information on the SBSA UEFI Shell application, please refer to the USER Guide which is a part of the SBSA Architecture Compliance Kit.

### Post-Silicon

On a system where a USB port is available and functional, follow the below steps

1. Copy the Sbsa.efi to a USB Flash drive
2. Plug in the USB Flash drive to one of the functional USB ports on the system
3. Boot the system to UEFI shell
4. Execute “map –r” command to determine the file system number of the plugged in USB drive
5. Type “fs<x>” where <x> is replaced by the number determined in the step above
6. Run the executable “Sbsa.efi” with the appropriate parameters to start the compliance tests
   - for details on the parameters, refer to [SBSA ACS User Guide](docs/SBSA_ACS_User_Guide.pdf)
7. Copy the UART console output to a log file for analysis and certification


### Emulation environment with secondary storage

1. Create an Image file which contains the Sbsa.efi file. For Example:
  - mkfs.vfat -C -n HD0 hda.img 2097152
  - sudo mount -o rw,loop=/dev/loop0,uid=`whoami`,gid=`whoami` hda.img /mnt/sbsa
  - cp  "<path to application>/Sbsa.efi" /mnt/sbsa/
  - sudo umount /mnt/sbsa
2. Load the image file to the secondary storage using a backdoor. The steps to do this is Emulation environment specific and beyond the scope of this documentation. 
3. Boot the system to UEFI shell.
4. Execute “map –r” command to determine the file system number of the secondary storage.
5. Type “fs<x>” where <x> is replaced by the number determined in the step above.
6. Run the executable “Sbsa.efi” to start the compliance tests.
    - for details on the parameters, refer to [SBSA ACS User Guide](docs/SBSA_ACS_User_Guide.pdf)
7. Copy the UART console output to a log file for analysis and certification

### Emulation environment without secondary storage

On an Emulation platform where secondary storage is not available.

1. Add the Sbsa.efi file as part of the UEFI FD file.
    - Add SbsaAvs.inf and the dependant VAL and PAL inf files to the Platform dsc and fdf files.
2. Build UEFI image including the UEFI Shell.
3. Boot the system to UEFI shell.
4. Run the executable “Sbsa.efi” to start the compliance tests.
    - for details on the parameters, refer to [SBSA ACS User Guide](docs/SBSA_ACS_User_Guide.pdf)
5. Copy the UART console output to a log file for analysis and certification.


## Linux OS based tests
Certain PCIe and IOMMU tests require Linux operating system with kernel version 4.10 or above. The description on how to build and run these tests are part of the [SBSA ACS User Guide](docs/SBSA_ACS_User_Guide.pdf).


## License
SBSA ACS is distributed under Apache v2.0 License.


## Feedback, contrubutions and support

 - Please use the GitHub Issue Tracker associated with this repository for feedback.
 - ARM licensees may contact ARM directly via their partner managers.
 - We welcome code contributions via GitHub Pull requests. Please see "Contributing Code" section of the documentation for details.
