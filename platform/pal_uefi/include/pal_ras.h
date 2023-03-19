/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#ifndef __PAL_RAS_H__
#define __PAL_RAS_H__

#pragma pack(1)

#define MAX_NUM_OF_RAS_SUPPORTED  140

typedef union {
  EFI_ACPI_AEST_PROCESSOR_STRUCT         Proc;
  EFI_ACPI_AEST_MEMORY_CONTROLLER_STRUCT Mem;
  EFI_ACPI_AEST_SMMU_STRUCT              Smmu;
  EFI_ACPI_AEST_VENDOR_DEFINED_STRUCT    Vendor;
  EFI_ACPI_AEST_GIC_STRUCT               Gic;
} AEST_NODE_RESOURCE;

typedef struct {
  AEST_NODE_RESOURCE     NodeResource;
  EFI_ACPI_AEST_INTERFACE_STRUCT  NodeInterface;
  EFI_ACPI_AEST_INTERRUPT_STRUCT  NodeInterrupt[2];
} AEST_NODE;

typedef struct {
  EFI_ACPI_ARM_ERROR_SOURCE_TABLE Header;
  AEST_NODE     node[];
} AEST_TABLE;


/* RAS2 Feature ACPI Table Structures and Definitions*/

/* EDK2 doesn't define RAS2 table signature and structures, remove and use EDK2 definitions
   once EDK2 extends support */

#define EFI_ACPI_6_5_RAS2_FEATURE_TABLE_SIGNATURE SIGNATURE_32('R', 'A', 'S', '2')
#define RAS2_FEATURE_TYPE_MEMORY 0x0
#define RAS2_PLATFORM_FEATURE_PATROL_SCRUB_BITMASK 0x1ULL

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER Header;
  UINT16 Reserved;
  UINT16 NumOfPccDescriptors;
} RAS_FEATURE_2_TABLE_HEADER;

typedef struct {
  UINT8  PccIdentifier;
  UINT16 Reserved;
  UINT8  FeatureType;
  UINT32 Instance;
} RAS2_PCC_DESCRIPTOR;

typedef struct {
  UINT32 Signature;
  UINT16 Command;
  UINT16 Status;
  UINT16 Version;
  UINT64 RasFeatures[2];
  UINT64 SetRasCapabilities[2];
  UINT16 NumOfRas2ParameterBlocks;
  UINT32 SetRasCapabilitiesStatus;
} RAS2_PCC_SHARED_MEMORY_REGION;

#pragma pack()

#endif
