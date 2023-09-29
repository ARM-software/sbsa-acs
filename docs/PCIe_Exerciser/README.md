
# Running SBSA ACS tests on FVP

The SBSA ACS tests can be executed on the Reference Design (RDN2) Fixed Virtual Platform (FVP). The FVP also offers enhanced support for running the exerciser tests, thereby expanding test coverage on the platform. The SBSA Exerciser reference implementation is specifically designed for the RDN2 Fixed Virtual Platform.


## Required modifications on RDN2 reference platform to run SBSA ACS tests

This section provides information from where to download the pre-built model for the RDN2 FVP and build the software stack for the same using the source code. It also provides information on how to enable the exerciser so all the SBSA tests, including the exerciser, can be run.

### Build Steps

To build the RDN2 software stack and download the pre-built model for the FVP, follow the below steps.

1. Create RDN2 directory and cd into it
   ```
   cd rdn2_path
   ```

2. Download the required sources for setting up the RDN2 software stack by following the steps outlined in the [Setup Workspace](https://neoverse-reference-design.docs.arm.com/en/latest/platforms/common/setup-workspace.html#setup-workspace-label) documentation.

3. Download the latest version of RDN2 FVP from [Arm Ecosystem FVPs](https://developer.arm.com/downloads/-/arm-ecosystem-fvps) and set the MODEL PATH
   ```
   export MODEL=model_path_to_Linux64_GCC-9.3/FVP_RD_N2
   ```

4. Modify the following in build-scripts/build-tf-a.sh
   ```diff
   - make -j $PARALLELISM PLAT=$plat ARCH=$TF_A_ARCH DEBUG=$TF_A_DEBUG_ENABLED ${tfa_build_flags} ${targets}
   + make -j $PARALLELISM PLAT=$plat ARCH=$TF_A_ARCH ENABLE_MPAM_FOR_LOWER_ELS=1 RAS_TRAP_LOWER_EL_ERR_ACCESS=0 DEBUG=$TF_A_DEBUG_ENABLED ${tfa_build_flags} ${targets}
   ```

5. Modify the following in uefi/edk2/ArmPkg/Drivers/ArmGic/GicV3/ArmGicV3Dxe.c
   ```diff
   - mGicNumInterrupts = ArmGicGetMaxNumInterrupts (mGicDistributorBase);
   + mGicNumInterrupts = 16384;
   ```

6. Build the platform using the command
  ```
   ./build-scripts/rdinfra/build-test-acs.sh -p rdn2 all
   ```


**Note**: The platform name for the build will be **RD-N2**. For more details on software stack and obtaining the latest version of RD-N2 FVP, see [RDN2](https://gitlab.arm.com/infra-solutions/reference-design/docs/infra-refdesign-docs/-/tree/main/platforms/rdn2)


### Modifications to enable the Exerciser in PCIe hierarchy.

This sections provides information on the modifications required to integrate and enable the Exerciser in the PCIe hierarchy as a programmable endpoint.

A reference of the example_pcie_hierarchy_0.json is present [here](example_pcie_hierarchy_0.json)

To run the SBSA exerciser tests make the following change in ${rdn2_path}/model-scripts/rdinfra/platforms/rdn2/run_model.sh

```diff
- -C pcie_group_0.pciex16.hierarchy_file_name=<default> \
- -C pcie_group_1.pciex16.hierarchy_file_name="example_pcie_hierarchy_2.json" \
- -C pcie_group_2.pciex16.hierarchy_file_name="example_pcie_hierarchy_3.json" \
- -C pcie_group_3.pciex16.hierarchy_file_name="example_pcie_hierarchy_4.json" \
+ -C pcie_group_0.pciex16.hierarchy_file_name="example_pcie_hierarchy_0.json" \
```

To run Exerciser test 905 - PE 2/4/8B writes to PCIe as 2/4/8B, please make the following change to all exerciser enteries in the example_pcie_hierarchy_0.json file located at ${rdn2_path}/model-scripts/rdinfra/platforms/rdn2/

```diff
-        "max_transaction_trace_entries": 8,
+        "max_transaction_trace_entries": 1,
```

## Useful links

- For information on configuring the [PCIe Hierarchy](PCIeConfigurableHierarchy.md)
- For more information on PCIe capabilities that exerciser supports, refer [Exerciser.md](Exerciser.md)

*Copyright (c) 2023 Arm Limited and Contributors. All rights reserved.*
