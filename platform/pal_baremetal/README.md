# Baremetal README
**Please Note**: The code in the "pal_baremetal" directory is only a reference code for implementation of PAL API's and it has not been verified on any model or SoC's.
The directory pal_baremetal consists of the reference code of the PAL API's specific to a platform.
Description of each directory are as follows:

## Directory Structure
1. **common**: The implementation common to all the baremetal platforms for all modules are in this directory.\
&emsp; 1.1 **include**:\
&emsp; &emsp; -  pal_common_support.h: Implementation that is common to all platforms.\
&emsp; &emsp; -  pal_pcie_enum.h: Implementation needed for enumeration.\
&emsp; &emsp; -  pal_pl011_uart.h: Implementation needed for PL011 UART driver. \
&emsp; &emsp; -  pal_pmu.h: Implementation needed for PMU specific data structures.\
&emsp; 1.2. **src**:\
&emsp; &emsp; Source files common to all baremetal platforms which do not require user modification.\
&emsp; &emsp; Eg: Info tables parsing, PCIe enumeration code, etc.
&emsp;
2. **RDN2**: Contains Platform specific code. The details in this folder need to be modified w.r.t the platform.

## Build Steps

Reference Cmake file is present at [CMakeLists.txt](../../CMakeLists.txt). To compile SBSA, perform following steps.

1. cd sbsa-acs
2. export CROSS_COMPILE=<path_to_the_toolchain>/bin/aarch64-none-linux-gnu-
3. mkdir build
4. cd build
5. cmake ../ -G"Unix Makefiles" -DCROSS_COMPILE=$CROSS_COMPILE -DTARGET="Target platform"
6. make

*Recommended*: CMake v3.17, GCC v12.2
```
CMake Command Line Options:
 -DARM_ARCH_MAJOR = Arch major version. Default value is 9.
 -DARM_ARCH_MINOR = Arch minor version. Default value is 0.
 -DCROSS_COMPILE  = Cross compiler path
 -DTARGET         = Target platform. Should be same as folder under pal_baremetal
```

On a successful build, *.bin, *.elf, *.img and debug binaries are generated at *build/output* directory. The output library files will be generated at *build/tools/cmake/acs_host* of the sbsa-acs directory.

## Running SBSA ACS with Bootwrapper on RDN2

**1. In RDN2 software stack make following change:**

  In <rdn2_path>/build-scripts/build-target-bins.sh - replace uefi.bin with acs_latest.bin

```
  if [ "${!tfa_tbbr_enabled}" == "1" ]; then
      $TOP_DIR/$TF_A_PATH/tools/cert_create/cert_create  \
      ${cert_tool_param} \
-     ${bl33_param_id} ${OUTDIR}/${!uefi_out}/uefi.bin
+     ${bl33_param_id} ${OUTDIR}/${!uefi_out}/acs_latest.bin
  fi

  ${fip_tool} update \
  ${fip_param} \
- ${bl33_param_id} ${OUTDIR}/${!uefi_out}/uefi.bin \
+ ${bl33_param_id} ${OUTDIR}/${!uefi_out}/acs_latest.bin \
  ${PLATDIR}/${!target_name}/fip-uefi.bin

```

**2. Repackage the FIP image with this new binary**
- cp <sbsa_acs>/build/output/acs_host.bin <rdn2_path>/output/rdn2/components/css-common/acs_latest.bin

- cd <rdn2_path>

- ./build-scripts/rdinfra/build-test-acs.sh -p rdn2 package

- export MODEL=<path_to_FVP_RDN2_model>

- cd <rdn2>/model-scripts/rdinfra/platforms/rdn2

- ./run_model.sh

**Note:** Any platform specific changes can be done by using TARGET_BM_BOOT macro defintion. The pal_baremetal reference code is located in [pal_baremetal](.). To customize the bare-metal code for different platforms, create a directory <platform_name> in [pal_baremetal](.) folder and copy the reference code from [include](RDN2/include) and [source](RDN2/src) folders from [RDN2](RDN2) to <platform_name>.


For more details on how to port the reference code to a specific platform and for further customisation please refer to the [User Guide](../../docs/arm_sbsa_architecture_compliance_bare-metal_user_guide.pdf)

-----------------

*Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.*
