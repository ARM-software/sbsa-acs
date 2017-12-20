/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

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

#ifndef __PAL_IOVIRT_H__
#define __PAL_IOVIRT_H__

#pragma pack(1)

typedef struct {
        EFI_ACPI_DESCRIPTION_HEADER header;
        UINT32 node_count;
        UINT32 node_offset;
        UINT32 reserved;
}IORT_TABLE;

typedef struct {
         UINT8 type;
         UINT16 length;
         UINT8 revision;
         UINT32 reserved;
         UINT32 mapping_count;
         UINT32 mapping_offset;
         CHAR8 node_data[1];
}IORT_NODE;

/* Values for subtable Type above */


typedef struct {
        UINT32 input_base;         /* Lowest value in input range */
        UINT32 id_count;           /* Number of IDs */
        UINT32 output_base;        /* Lowest value in output range */
        UINT32 output_reference;   /* A reference to the output node */
        UINT32 flags;
}IORT_ID_MAPPING;

typedef struct {
        UINT32 its_count;
        UINT32 identifiers[1];     /* GIC ITS identifier arrary */
}IORT_ITS_GROUP;

typedef struct {
        UINT32 node_flags;
        UINT64 memory_properties;  /* Memory access properties */
        UINT8 memory_address_limit;        /* Memory address size limit */
        CHAR8 device_name[1];    /* Path of namespace object */
}IORT_NAMED_COMPONENT;

typedef struct {
        UINT64 memory_properties;  /* Memory access properties */
        UINT32 ats_attribute;
        UINT32 pci_segment_number;
}IORT_ROOT_COMPLEX;

typedef struct {
        UINT64 base_address;       /* SMMU base address */
        UINT64 span;               /* Length of memory range */
        UINT32 model;
        UINT32 flags;
        UINT32 global_interrupt_offset;
        UINT32 context_interrupt_count;
        UINT32 context_interrupt_offset;
        UINT32 pmu_interrupt_count;
        UINT32 pmu_interrupt_offset;
        UINT64 interrupts[1];      /* Interrupt array */
}IORT_SMMU;

typedef struct {
        UINT64 base_address;       /* SMMUv3 base address */
        UINT32 flags;
        UINT32 reserved;
        UINT64 vatos_address;
        UINT32 model;              /* O: generic SMMUv3 */
        UINT32 event_gsiv;
        UINT32 pri_gsiv;
        UINT32 gerr_gsiv;
        UINT32 sync_gsiv;
}IORT_SMMU_V3;

typedef struct {
        UINT64 base_address;
        UINT32 overflow_interrupt_gsiv;
        UINT32 node_reference;
}IORT_PMCG;
#pragma pack()

#endif
