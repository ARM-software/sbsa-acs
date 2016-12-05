
# Server Base System Architecture - Architecture Compliance Suite


## Server Base System Architecture (SBSA)
The **Server Base Systems Architecture** (SBSA) specification specifies a hardware system architecture, based on ARM 64-bit architecture, which server system software, such as operating systems, hypervisors and firmware can rely on. It addresses PE features and key aspects of system architecture. 

For more information, download the [SBSA specification](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0029/index.html)


## SBSA - Architecture Compliance Suite (ACS)

SBSA **Architecture compliance tests** are self-checking, portable C-based tests with directed stimulus.
The present release implements a UEFI shell application to execute these tests from a UEFI Shell.


## Release Details
 - Code Quality  : Alpha
 - The result of a test should not be taken as a true indication of compliance. There is a possibility of false positives / false negetives.
 - There are gaps in the test coverage. Refer to the [Testcase checklist](docs/testcase-checklist.md) for details.


### Prerequisites

1. Any mainstream Linux based OS distribution
2. git clone [EDK2 tree](https://github.com/tianocore/edk2)
3. Install GCC 5.3 or later toolchain for Linux from [here](https://releases.linaro.org/components/toolchain/binaries/)
4. Install the Build pre-requisite packages from [here](https://community.arm.com/docs/DOC-10804)

## Additional Reading
1. For details on the SBSA ACS UEFI Shell Application, refer to the [SBSA ACS User Guide](docs/SBSA_ACS_UEFI_App_User_Guide.pdf)
2. For more details on the Design of the SBSA ACS, refer to the [Validation Methodology Document](docs/SBSA_Val_Methodolgy.pdf)
3. For details on the test coverage, the scenarios implemented in the present release of the ACS and the scenarios planned in the future release, refer to the [Testcase checklist](docs/testcase-checklist.md)

### ACS Build steps

1.  git clone https://github.com/ARM-software/sbsa-acs
2.  cd sbsa-acs
3.  source tools/scripts/avssetup.sh [local acs path] [local_edk2_path]
4.  cd local_edk2_path
5.  Add the following to the [LibraryClasses.common] section in ShellPkg/ShellPkg.dsc
    Add  SbsaValLib|AppPkg/Applications/Sbsa/val/SbsaValLib.inf
    Add  SbsaPalLib|AppPkg/Applications/Sbsa/pal/SbsaPalLib.inf
6.  Add AppPkg/Applications/Sbsa/SbsaAvs.inf in the [components] section of ShellPkg/ShellPkg.dsc
7.  export GCC49_AARCH64_PREFIX= GCC5.3 toolchain path /bin/aarch64-linux-gnu-
8.  source edksetup.sh
9.  make -C BaseTools/Source/C
10. source AppPkg/Applications/Sbsa/avsbuild.sh

### Build Output

The EFI executable file is generated at 
edk2_path /Build/Shell/DEBUG_GCC49/AARCH64/Sbsa.efi


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
   a. for details on the parameters, refer to [SBSA ACS User Guide](docs/SBSA_ACS_UEFI_App_User_Guide.pdf)
7. Copy the UART console output to a log file for analysis and certification


### Emulation environment with secondary storage

1. Create an Image file which contains the Sbsa.efi file.
    a. For Example:
    b. mkfs.vfat -C -n HD0 hda.img 2097152
    c. sudo mount -o rw,loop=/dev/loop0,uid=`whoami`,gid=`whoami` hda.img /mnt/sbsa
    d. cp  <path to application>/Sbsa.efi /mnt/sbsa/
    e. sudo umount /mnt/sbsa
2. Load the image file to the secondary storage using a backdoor. The steps to do this is Emulation environment specific and beyond the scope of this documentation. 
3. Boot the system to UEFI shell.
4. Execute “map –r” command to determine the file system number of the plugged in USB drive.
5. Type “fs<x>” where <x> is replaced by the number determined in the step above.
6. Run the executable “Sbsa.efi” to start the compliance tests.
   a. for details on the parameters, refer to [SBSA ACS User Guide](docs/SBSA_ACS_UEFI_App_User_Guide.pdf)
7. Copy the UART console output to a log file for analysis and certification

### Emulation environment without secondary storage

On an Emulation platform where secondary storage is not available.

1. Add the Sbsa.efi file as part of the UEFI FD file.
    a. Add Sbsa.efi as a binary to the UefiShellCommandLib
    b. Or add the SbsaAvsMain.inf to the UefiShellCommandLib.inf
2. Build UEFI image including the UEFI Shell.
3. Boot the system to UEFI shell.
4. Run the executable “Sbsa.efi” to start the compliance tests.
   a. for details on the parameters, refer to [SBSA ACS User Guide](docs/SBSA_ACS_UEFI_App_User_Guide.pdf)
5. Copy the UART console output to a log file for analysis and certification.

## License

SBSA ACS is distributed under Apache v2.0 License.


## Feedback, Contrubutions and Support

 - Please use the GitHub Issue Tracker associated with this repository for feedback.
 - ARM licensees may contact ARM directly via their partner managers.
 - We welcome code contributions via GitHub Pull requests. Please see "Contributing Code" section of the documentation for details.
