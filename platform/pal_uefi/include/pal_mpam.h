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

#ifndef __PAL_MPAM_H__
#define __PAL_MPAM_H__

/* Not defined in EDK2: Remove once EDK2 supports MPAM*/
#define MEMORY_RESOURCE_PARTITIONING_AND_MONITORING_TABLE_SIGNATURE SIGNATURE_32('M', 'P', 'A', 'M')

#pragma pack(1)

typedef struct {
    EFI_ACPI_DESCRIPTION_HEADER header;
} EFI_ACPI_MPAM_TABLE;

typedef struct {
    UINT16 length;
    UINT8  InterfaceType;
    UINT8  reserved;
    UINT32 identifier;
    UINT64 base_address;
    UINT32 mmio_size;
    UINT32 overflow_interrupt;
    UINT32 overflow_int_flags;
    UINT32 reserved1;
    UINT32 overflow_int_aff;
    UINT32 error_interrupt;
    UINT32 error_int_flags;
    UINT32 reserved2;
    UINT32 error_int_aff;
    UINT32 max_nrdy_usec;
    UINT64 hardware_id;
    UINT32 instance_id;
    UINT32 num_resource_nodes;
} EFI_ACPI_MPAM_MSC_NODE_STRUCTURE;

typedef struct {
    UINT32 identifier;
    UINT8  ris_index;
    UINT16 reserved1;
    UINT8  locator_type;
    UINT64 descriptor1;
    UINT32 descriptor2;
    UINT32 num_dependencies;
} EFI_ACPI_MPAM_RESOURCE_NODE_STRUCTURE;

typedef struct {
    UINT32 producer;
    UINT32 reserved;
} EFI_ACPI_MPAM_FUNC_DEPEN_DESC_STRUCTURE;

#pragma pack()

#endif
