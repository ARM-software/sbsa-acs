# ARM Enterprise - Architecture Compliance Suite (ACS)

## Overview 

This test suite is a collection of tests which validate an implementation against
the various ARM System Architecture specifications.

In the current release, an implementation is checked for compliance against the
following specifications:
  - Server Base System Architecture (SBSA) 
    ~ For details on how to Build and Run SBSA ACS, refer to the [SBSA ACS README](sbsa/README.md)
  - Server Base Boot Requirement specifications (SBBR).
    ~ For details on how to Build and Run SBBR ACS, refer to the [SBBR ACS README](sbbr/README.md)

##Pre-requisites
  1. Operating system - Ubuntu 16.xx and above
  2. Linaro GCC 5.3 or above tool chain.

## Dependencies

  This suite consumes the following external source code databases.

  1. UEFI EDK2 source code - This source is available in public domain and used to build the SBSA UEFI shell application.

  2. UEFI SCT Framework - The access to this is limited to UEFI forum members only. This is a requirement to run SBBR compliance tests.

  3. Canonical LuvOS with FWTS suite - This suite is available in the public domain. This source code is downloaded by the below mentioned scripts. No user intervention is required.


## Directory organisation
  1. sbsa
     - This directory contains the source code pertaining to the SBSA ACS.
  2. sbbr
     - This directory contains the source code pertaining to the SBBR ACS which has not yet been upstreamed to the UEFI-SCT and FWTS repositories..
  3. package
     - This directory contains the build and packaging scripts to generate a unified LuvOS image which contains the SBSA and SBBR test suites.


## Build Steps

### Build the ARM Enterprise - Compliance Suite package

  1. git clone https://github.com/ARM-software/arm-enterprise-acs
  2. cd arm-enterprise-acs
  3. ./scripts/acs_sync.sh
      - Download the luvOS repository and apply the patches
  4. ./build_luvos.sh
      - Build SBSA binaries, SBBR (including UEFI-SCT and FWTS binaries) and will create a luv-live-image.img

### Build only SBSA ACS UEFI Application
  - Refer to the [README.md](sbsa/README.md) file in the sbsa directory.

## RUN steps
### Run the Enterprise ACS image
  1. Copy the luv-live-image.img file created in step 4 above to a USB key.
      - The steps to create a boot-disk from an img file are beyond the scope of this file.

  2. Plug in the USB key to a target system.

  3. Choose the appropriate option at the Grub Boot menu
      - LuvOS (OS Conetext Test Cases)
      - SBBR/SBSA (UEFI Shell based test cases)

### Run only the SBSA ACS
   - Refer to the [README.md](sbsa/README.md) file in the sbsa directory.



## License

ARM Enterprise ACS is distributed under Apache v2.0 License.


## Feedback, Contrubutions and Support

 - Please use the GitHub Issue Tracker associated with this repository for feedback.
 - ARM licensees may contact ARM directly via their partner managers.
 - We welcome code contributions via GitHub Pull requests. Please see "Contributing Code" section of the documentation for details.

