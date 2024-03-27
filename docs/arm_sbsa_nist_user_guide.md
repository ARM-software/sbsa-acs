# National Institute of Standards and Technology: Statistical Test Suite

**Need for Randomness?**

Randomness relates to many areas of computer science, in particular with cryptography. Well-designed cryptographic primitives like hash functions, stream ciphers should produce pseudorandom data. The outputs of such generators may be used in many cryptographic applications like the generation of key material. Generators suitable for use in cryptographic applications need to meet stronger requirements than for other applications. In particular, their outputs must be unpredictable in the absence of knowledge of the inputs.

**Statistical Test Suites**

Randomness testing plays a fundamental role in cryptography. Randomness is tested using test suites consisting of many tests of randomness each focusing on different feature. These tests can be used as first steps in determining whether or not a generator is suitable for a particular cryptographic application.

**NIST with SBSA**

There are five well-known statistical test suites -- NIST STS, Diehard, TestU01, ENT and CryptX. Only the first three test suites are commonly used for the randomness analysis since CryptX is a commercial software and ENT provides only basic randomness testing. Since NIST STS has a special position by being published as an official document, it is often used in the preparation of formal certifications or approvals.

**Building NIST STS with SBSA ACS**

To build NIST statistical test suite with SBSA ACS, NIST STS 2.1.2 package is required. This package is obtained from <https://csrc.nist.gov/CSRC/media/Projects/Random-Bit-Generation/documents/sts-2_1_2.zip>  and is downloaded automatically as part of the build process.

This is an updated version of [NIST Statistical Test Suite (STS)](http://csrc.nist.gov/groups/ST/toolkit/rng/documentation_software.html) tool for randomness testing. The reason for the update is, the original source code provided with NIST does not compile cleanly in UEFI because it does not provide erf() and erfc() functions in the standard math library and (harcoded the inputs -- needs to be rephrased). Implementation of these functions has been added as part of SBSA val and a patch file is created.

**Tool Requirement**

Current release require the below tool:

1. unzip

**Build steps**

To start the ACS build with NIST STS, perform the following steps:

1.  Add the following to the [LibraryClasses.common] section in ShellPkg/ShellPkg.dsc
```
    UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
    !ifdef $(ENABLE_NIST)
        SbsaNistLib|ShellPkg/Application/sbsa-acs/test_pool/nist_sts/SbsaNistLib.inf
        SbsaValNistLib|ShellPkg/Application/bsa-acs/val/SbsaValNistLib.inf
        SbsaPalNistLib|ShellPkg/Application/bsa-acs/pal/uefi_acpi/SbsaPalNistLib.inf
    !else
        SbsaValLib|ShellPkg/Application/bsa-acs/val/SbsaValLib.inf
        SbsaPalLib|ShellPkg/Application/bsa-acs/pal/uefi_acpi/SbsaPalLib.inf
    !endif
```
2.  Add the following in the [components] section of ShellPkg/ShellPkg.dsc
```
    !ifdef $(ENABLE_NIST)
        ShellPkg/Application/sbsa-acs/uefi_app/SbsaAvsNist.inf
    !else
        ShellPkg/Application/sbsa-acs/uefi_app/SbsaAvs.inf
    !endif
```
3.  Modify CC Flags in the [BuildOptions] section of ShellPkg/ShellPkg.dsc
```
    !ifdef $(ENABLE_NIST)
        *_*_*_CC_FLAGS = -DENABLE_NIST
    !else
        *_*_*_CC_FLAGS =
    !endif

    !include edk2-libc/StdLib/StdLib.inc
```
4.  Modify the following in the edk2-libc/StdLib/LibC/Main/Main.c
```
    -extern int main( int, char**);
    +extern int ShellAppMainsbsa( int, char**);
```
5.  Modify the following in ShellAppMain() of edk2-libc/StdLib/LibC/Main/Main.c
```
    -ExitVal = (INTN)main( (int)Argc, gMD->NArgV);
    +ExitVal = (INTN)ShellAppMainsbsa( (int)Argc, gMD->NArgV);
```
6. Comment the map[] variable in edk2-libc/StdLib/LibC/Main/Arm/flt_rounds.c to avoid -werror=unused-variable
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


To build the SBSA test suite with NIST STS, execute the following commands:
***Linux build environment***
```
source ShellPkg/Application/sbsa-acs/tools/scripts/acsbuild.sh NIST
```

***Windows build environment***
```
build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sbsa-acs/uefi_app/SbsaAvs.inf -D ENABLE_NIST
```

**Directory structure of SBSA ACS**

The following figure shows the source code directory for SBSA ACS

    sbsa<br/>
    ├── docs<br/>
    ├── linux_app<br/>
    ├── patches<br/>
    │   └── nist_sbsa_sts.diff  ────────> Patch to compile NIST STS with SBSA ACS<br/>
    │<br/>
    ├── platform<br/>
    │   ├── pal_baremetal<br/>
    │   ├── pal_uefi<br/>
    |
    ├── test_pool<br/>
    │   ├── exerciser<br/>
    │   ├── gic<br/>
    │   ├── io_virt<br/>
    │   ├── pcie<br/>
    │   ├── pe<br/>
    │   ├── peripherals<br/>
    │   ├── power_wakeup<br/>
    │   ├── mpam<br/>
    │   ├── ras<br/>
    │   ├── pmu<br/>
    │   ├── smmu<br/>
    │   ├── timer_wd<br/>
    │   └── nist_sts<br/>
    │       ├── test_n001.c     ────────>  NIST entry point to STS<br/>
    │       └── sts-2.1.2<br/>
    │           └── sts-2.1.2   ────────>  NIST STS package<br/>
    │<br/>
    ├── tools<br/>
    │   └── scripts<br/>
    ├── uefi_app<br/>
    └── val<br/>
        ├── include<br/>
        └── src<br/>
            └── avs_nist.c      ────────>  erf and erfc() implementations<br/>

**Running NIST STS**

Run the UEFI Shell application with the "-nist" as an argument argument

    uefi shell> sbsa.efi -nist

**Interpreting the results**

Final analysis report is generated when statistical testing is complete. The report contains a summary of empirical results which is displayed on the console. A test is unsuccessful when P-value < 0.01 and then the sequence under test should be considered as non-random. Example result as below

    ------------------------------------------------------------------------------
    RESULTS FOR THE UNIFORMITY OF P-VALUES AND THE PROPORTION OF PASSING SEQUENCES
    ------------------------------------------------------------------------------
    ------------------------------------------------------------------------------
     C1  C2  C3  C4  C5  C6  C7  C8  C9 C10  P-VALUE  PROPORTION  STATISTICAL TEST
    ------------------------------------------------------------------------------
      4   1   1   2   1   1   0   0   0   0  0.122325     10/10      OverlappingTemplate
     10   0   0   0   0   0   0   0   0   0  0.000000 *    0/10   *  Universal
      7   2   1   0   0   0   0   0   0   0  0.000001 *    8/10      ApproximateEntropy
      3   4   1   0   2   0   0   0   0   0  0.017912      9/10      Serial
      4   0   1   0   2   1   0   1   1   0  0.122325     10/10      Serial
      1   1   1   1   0   2   0   3   0   1  0.534146     10/10      LinearComplexity


    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    The minimum pass rate for each statistical test with the exception of the
    random excursion (variant) test is approximately = 8 for a
    sample size = 10 binary sequences.

    The minimum pass rate for the random excursion (variant) test is undefined.

    For further guidelines construct a probability table using the MAPLE program
    provided in the addendum section of the documentation.
    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


For more details on NIST STS, see: <https://doi.org/10.6028/NIST.SP.800-22r1a>

**Note**: For SBSA level 7 compliance, passing the NIST statistical test suite is required for S_L7ENT_1 rule.

--------------

*Copyright (c) 2020, 2023-2024, Arm Limited and Contributors. All rights reserved.*
