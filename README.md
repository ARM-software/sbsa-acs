
# Server Base System Architecture - Architecture Compliance Suite


## Server Base System Architecture (SBSA)
The **Server Base Systems Architecture** (SBSA) specification specifies a hardware system architecture, based on ARM 64-bit architecture, which server system software, such as operating systems, hypervisors and firmware can rely on. It addresses PE features and key aspects of system architecture. 

For more information, download the [SBSA specification](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0029/index.html)


## SBSA - Architecture Compliance Suite (ACS)

SBSA **Architecture compliance suite** is a collection of self-checking, portable C-based tests.
These tests check an implementation for compliance against SBSA specification version 3.0.
The present release implements a UEFI shell application to execute these tests from UEFI Shell.


## Release Details
 - Code Quality  : Alpha
 - The result of a test should not be taken as a true indication of compliance. There is a possibility of false positives / false negetives.
 - There are gaps in the test coverage. Refer to the [Testcase checklist](docs/testcase-checklist.md) for details.

## Github Branch
  - To pick up the release version of the code, checkout the release branch.
  - To get the latest version of the code with bug fixes and new features, use the master branch.

## Target Platforms
  Any AARCH64 enterprise Platform which boots UEFI.

### Prerequisites

1. EDK2 source tree. git clone [EDK2 tree](https://github.com/tianocore/edk2)
2. Install GCC 5.3 or later toolchain from [here](https://releases.linaro.org/components/toolchain/binaries/)
3. Install the Build pre-requisite packages to build EDK2. The details are beyond the scope of this document.


## Additional Reading
1. For details on the SBSA ACS UEFI Shell Application, refer to the [SBSA ACS User Guide](docs/SBSA_ACS_UEFI_App_User_Guide.pdf)
2. For more details on the Design of the SBSA ACS, refer to the [Validation Methodology Document](docs/SBSA_Val_Methodolgy.pdf)
3. For details on the test coverage, the scenarios implemented in the present release of the ACS and the scenarios planned in the future release, refer to the [Testcase checklist](docs/testcase-checklist.md)

### ACS Build steps

1.  cd local_edk2_path
2.  git clone https://github.com/ARM-software/sbsa-acs AppPkg/Applications/sbsa-acs
3.  Add the following to the [LibraryClasses.common] section in ShellPkg/ShellPkg.dsc
   - Add  SbsaValLib|AppPkg/Applications/sbsa-acs/val/SbsaValLib.inf
   - Add  SbsaPalLib|AppPkg/Applications/sbsa-acs/platform/pal_uefi/SbsaPalLib.inf
4.  Add AppPkg/Applications/sbsa-acs/uefi_app/SbsaAvs.inf in the [components] section of ShellPkg/ShellPkg.dsc

#### Linux
1.  export GCC49_AARCH64_PREFIX= "GCC5.3 toolchain path"/bin/aarch64-linux-gnu-
2.  source edksetup.sh
3.  make -C BaseTools/Source/C
4.  source AppPkg/Applications/sbsa-acs/tools/scripts/avsbuild.sh

#### Windows
1.  Set the toolchain path to GCC53.
2.  set up the environment for AARCH64 - edk2 build.
3.  Build the Sbsa Shell application. example:  build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m AppPkg/Applications/sbsa-acs/uefi_app/SbsaAvs.inf

### Build Output

The EFI executable file is generated at 
edk2_path/Build/Shell/DEBUG_GCC49/AARCH64/Sbsa.efi


## Test Suite Execution

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
   - for details on the parameters, refer to [SBSA ACS User Guide](docs/SBSA_ACS_UEFI_App_User_Guide.pdf)
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
    - for details on the parameters, refer to [SBSA ACS User Guide](docs/SBSA_ACS_UEFI_App_User_Guide.pdf)
7. Copy the UART console output to a log file for analysis and certification

### Emulation environment without secondary storage

On an Emulation platform where secondary storage is not available.

1. Add the Sbsa.efi file as part of the UEFI FD file.
    - Add SbsaAvs.inf and the dependant VAL and PAL inf files to the Platform dsc and fdf files.
    - Or add the SbsaAvs.inf and the dependant VAL and PAL inf files to the UefiShellCommandLib.inf
2. Build UEFI image including the UEFI Shell.
3. Boot the system to UEFI shell.
4. Run the executable “Sbsa.efi” to start the compliance tests.
    - for details on the parameters, refer to [SBSA ACS User Guide](docs/SBSA_ACS_UEFI_App_User_Guide.pdf)
5. Copy the UART console output to a log file for analysis and certification.

## License

SBSA ACS is distributed under Apache v2.0 License.


## Feedback, Contrubutions and Support

 - Please use the GitHub Issue Tracker associated with this repository for feedback.
 - ARM licensees may contact ARM directly via their partner managers.
 - We welcome code contributions via GitHub Pull requests. Please see "Contributing Code" section of the documentation for details.
