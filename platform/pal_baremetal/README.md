
The directory pal_baremetal consists of the reference code of the PAL API's specific to a platform.
Description of each directory are as follows:

1. src: The code common to all the baremetal platforms (like FVP and juno) are in the source directory (PCIe enumeration).
2. include:
  -  pcie_enum.h: Header file needed for enumeration.
  -  pal_common_support.h: Header file that is common to both platforms (FVP and juno).
3. FVP: Contains PAL API's specific to FVP.
4. Juno: Contains PAL API's specific to Juno.

--------------

*Copyright (c) 2020, Arm Limited and Contributors. All rights reserved.*
