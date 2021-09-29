
Please Note: The code in the "pal_baremetal" directory is only a reference code for implementation of PAL API's and it has not been verified on any model or SoC's.
The directory pal_baremetal consists of the reference code of the PAL API's specific to a platform.
Description of each directory are as follows:

1. src: The implementation common to all the baremetal platforms (like FVP and juno) for all modules are in the source directory.
2. include:
  -  pcie_enum.h: Implementation needed for enumeration.
  -  pal_common_support.h: Implementation that is common to both platforms (FVP and juno).
3. FVP: Contains Platform specific code. The details in this folder need to be modified w.r.t the platform.
4. Juno: Contains Platform specific code.

For more details on how to port the reference code to a specific platform and for further customisation please refer to the [User Guide](docs/Arm_SBSA_ACS_Bare-metal_User_Guide.pdf)

-----------------

*Copyright (c) 2020-2021, Arm Limited and Contributors. All rights reserved.*
