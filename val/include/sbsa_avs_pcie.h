/** @file
 * Copyright (c) 2016-2021, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __SBSA_AVS_PCIE_H__
#define __SBSA_AVS_PCIE_H__

#include "sbsa_avs_pcie_spec.h"

#define PCIE_EXTRACT_BDF_SEG(bdf)  ((bdf >> 24) & 0xFF)
#define PCIE_EXTRACT_BDF_BUS(bdf)  ((bdf >> 16) & 0xFF)
#define PCIE_EXTRACT_BDF_DEV(bdf)  ((bdf >> 8) & 0xFF)
#define PCIE_EXTRACT_BDF_FUNC(bdf) (bdf & 0xFF)

#define PCIE_CREATE_BDF_PACKED(bdf)  PCIE_EXTRACT_BDF_FUNC(bdf) | \
                                    (PCIE_EXTRACT_BDF_DEV(bdf) << 3) | \
                                    (PCIE_EXTRACT_BDF_BUS(bdf) << 8)

#define PCIE_CREATE_BDF(seg, bus, dev, func) ((seg << 24) | ((bus & 0xFF) << 16) | ((dev & 0xFF) << 8) | func)

#define GET_DEVICE_ID(bus, dev, func) ((bus << 8) | (dev << 3) | func)

#define PCIE_MAX_BUS   256
#define PCIE_MAX_DEV    32
#define PCIE_MAX_FUNC    8

#define PCIE_CFG_SIZE  4096

#define PCIE_INTERRUPT_LINE  0x3c
#define PCIE_INTERRUPT_PIN   0x3d
#define PCIE_INTERRUPT_PIN_SHIFT 0x8
#define PCIE_INTERRUPT_PIN_MASK  0xFF
#define PCIE_TYPE_ROOT_PORT  0x04 /* Root Port */
#define PCIE_TYPE_DOWNSTREAM 0x06 /* Downstream Port */
#define PCIE_TYPE_ENDPOINT   0x0  /* Express Endpoint */
#define PCIE_TYPE_LEG_END    0x01 /* Legacy Endpoint */
#define PCIE_TYPE_UPSTREAM   0x05 /* Upstream Port */
#define PCIE_TYPE_RC_END     0x09 /* Root Complex Integrated Endpoint */
#define PCI_EXT_CAPID_ACS    0x0D /* Access Control Services */
#define PCI_CAPID_ACS        0x04 /* ACS Capability Register */

#define WORD_ALIGN_MASK 0x3
#define BITS_IN_BYTE 0x8

#define PCIE_DLL_LINK_STATUS_NOT_ACTIVE       0x0
#define PCIE_DLL_LINK_STATUS_ACTIVE           0x1
#define PCIE_DLL_LINK_ACTIVE_NOT_SUPPORTED    0x2

#define REG_MASK(end, start) (((~(uint32_t)0 << (start)) & \
                             (((uint32_t)1 << ((end)+1)) - 1)) >> (start))
#define REG_SHIFT(alignment_byte_cnt, start) (((alignment_byte_cnt)*BITS_IN_BYTE) + start)

#define MAX_BITFIELD_ENTRIES 100
#define ERR_STRING_SIZE 64

#define MEM_OFFSET_10   0x10

/* Allows storage of 2048 valid BDFs */
#define PCIE_DEVICE_BDF_TABLE_SZ 8192

typedef enum {
  HEADER = 0,
  PCIE_CAP = 1,
  PCIE_ECAP = 2
} BITFIELD_REGISTER_TYPE;

typedef enum {
  HW_INIT = 0,
  READ_ONLY = 1,
  STICKY_RO = 2,
  RSVDP_RO = 3,
  RSVDZ_RO = 4,
  READ_WRITE = 5,
  STICKY_RW = 6
} BITFIELD_ATTR_TYPE;

/**
  @brief    Structure describes bit-field representation of PCIe config registers
  @reg_type         Bit-filed can be part of one of the following registers:
                        0: PCIe header register
                        1: PCIe capability register
                        2: PCie extended capability register
  @icap_id          Applicable only to PCIe capability register type
  @ecap_id          Applicable only to PCIe extended cabality register type
  @reg_offset       Offset is from one of the following memory mapped base addresses:
                        ECAM base address for reg_type = 0
                        ECAM cap_id structure base address for reg_type = 1
                        ECAM ecap_id structure base address for reg_type = 2
  @dev_port_bitmask Bitmask containing the type of PCIe device/port to which this
		            bf is applicable. Can take one of values in DEVICE_BITSMASK enum.
  @start            Bit-field start position within a register
  @end              Bit-field end position in a register
  @cfg_value        Bit-field configured value
  @attr             Bit-field configured attribute
  @err_str1         Error string related to lost config value
  @err_str2         Error string related to lost attribute
**/

typedef struct {
  BITFIELD_REGISTER_TYPE reg_type;
  uint16_t               cap_id;
  uint16_t               ecap_id;
  uint16_t               reg_offset;
  uint16_t               dev_port_bitmask;
  uint8_t                start;
  uint8_t                end;
  uint32_t               cfg_value;
  BITFIELD_ATTR_TYPE     attr;
  char                   err_str1[ERR_STRING_SIZE];
  char                   err_str2[ERR_STRING_SIZE];
} pcie_cfgreg_bitfield_entry;

typedef enum {
  MMIO = 0,
  IO = 1
} MEM_INDICATOR_TYPE;

typedef enum {
  BITS_32 = 0,
  BITS_64 = 2
} MEM_DECODE_TYPE;

typedef enum {
  NON_PREFETCHABLE = 0,
  PREFETCHABLE = 1
} MEM_TYPE;

typedef struct {
  uint32_t bdf;
  uint32_t rp_bdf;
} pcie_device_attr;

typedef struct {
  uint32_t num_entries;
  pcie_device_attr device[];         ///< in the format of Segment/Bus/Dev/Func
} pcie_device_bdf_table;

void     val_pcie_write_cfg(uint32_t bdf, uint32_t offset, uint32_t data);
void     val_pcie_io_write_cfg(uint32_t bdf, uint32_t offset, uint32_t data);
uint32_t val_pcie_read_cfg(uint32_t bdf, uint32_t offset, uint32_t *data);
uint32_t val_get_msi_vectors (uint32_t bdf, PERIPHERAL_VECTOR_LIST **mvector);
uint64_t val_pcie_get_bdf_config_addr(uint32_t bdf);


typedef enum {
  PCIE_INFO_NUM_ECAM = 1,
  PCIE_INFO_ECAM,
  PCIE_INFO_MCFG_ECAM,
  PCIE_INFO_START_BUS,
  PCIE_INFO_END_BUS,
  PCIE_INFO_SEGMENT
}PCIE_INFO_e;

uint64_t
val_pcie_get_info(PCIE_INFO_e type, uint32_t index);

uint32_t
val_pci_get_legacy_irq_map (uint32_t bdf, PERIPHERAL_IRQ_MAP *irq_map);

uint32_t
val_pcie_is_device_behind_smmu(uint32_t bdf);

uint32_t
val_pcie_get_device_type(uint32_t bdf);

uint32_t
val_pcie_get_root_port_bdf(uint32_t *bdf);

uint32_t
val_pcie_get_snoop_bit (uint32_t bdf);

uint32_t
val_pcie_get_dma_support(uint32_t bdf);

uint32_t
val_pcie_get_dma_coherent(uint32_t bdf);

uint32_t
val_pcie_io_read_cfg(uint32_t bdf, uint32_t offset, uint32_t *data);

uint32_t
val_pcie_get_rp_transaction_frwd_support(uint32_t bdf);

uint32_t
val_pcie_get_atomicop_requester_capable(uint32_t bdf);

uint32_t
val_pcie_is_cache_present(uint32_t bdf);

uint32_t
val_pcie_data_link_layer_status(uint32_t bdf);

uint32_t
val_pcie_check_interrupt_status(uint32_t bdf);

uint32_t
val_pcie_get_max_pasid_width(uint32_t bdf, uint32_t *max_pasid_width);

uint32_t
p001_entry(uint32_t num_pe);

uint32_t
p002_entry(uint32_t num_pe);

uint32_t
p003_entry(uint32_t num_pe);

uint32_t
p005_entry(uint32_t num_pe);

uint32_t
p006_entry(uint32_t num_pe);

uint32_t
p007_entry(uint32_t num_pe);

uint32_t
p008_entry (uint32_t num_pe);

uint32_t
p009_entry (uint32_t num_pe);

uint32_t
p010_entry (uint32_t num_pe);

uint32_t
p011_entry (uint32_t num_pe);

uint32_t
p012_entry (uint32_t num_pe);

uint32_t
p013_entry (uint32_t num_pe);

uint32_t
p014_entry (uint32_t num_pe);

uint32_t
p015_entry (uint32_t num_pe);

uint32_t
p016_entry (uint32_t num_pe);

uint32_t
p017_entry (uint32_t num_pe);

uint32_t
p018_entry (uint32_t num_pe);

uint32_t
p019_entry (uint32_t num_pe);

uint32_t
p020_entry(uint32_t num_pe);

uint32_t
p021_entry(uint32_t num_pe);

uint32_t
p022_entry(uint32_t num_pe);

uint32_t
p023_entry(uint32_t num_pe);

uint32_t
p024_entry(uint32_t num_pe);

uint32_t
p025_entry(uint32_t num_pe);

uint32_t
p026_entry(uint32_t num_pe);

uint32_t
p027_entry(uint32_t num_pe);

uint32_t
p028_entry(uint32_t num_pe);

uint32_t
p029_entry(uint32_t num_pe);

uint32_t
p030_entry(uint32_t num_pe);

uint32_t
p031_entry(uint32_t num_pe);

uint32_t
p032_entry(uint32_t num_pe);

uint32_t
p033_entry(uint32_t num_pe);

uint32_t
p034_entry(uint32_t num_pe);

uint32_t
p035_entry(uint32_t num_pe);

uint32_t
p036_entry(uint32_t num_pe);

uint32_t
p037_entry(uint32_t num_pe);

uint32_t
p038_entry(uint32_t num_pe);

uint32_t
p039_entry(uint32_t num_pe);

uint32_t
p040_entry(uint32_t num_pe);

uint32_t
p041_entry(uint32_t num_pe);

uint32_t
p042_entry(uint32_t num_pe);

uint32_t
p043_entry(uint32_t num_pe);

uint32_t
p044_entry(uint32_t num_pe);

uint32_t
p045_entry(uint32_t num_pe);

uint32_t
p046_entry(uint32_t num_pe);

uint32_t
p047_entry(uint32_t num_pe);

uint32_t
p048_entry(uint32_t num_pe);

uint32_t
p049_entry(uint32_t num_pe);

uint32_t
p050_entry(uint32_t num_pe);

uint32_t
p051_entry(uint32_t num_pe);

uint32_t
p052_entry(uint32_t num_pe);

uint32_t
p053_entry(uint32_t num_pe);

uint32_t
p054_entry(uint32_t num_pe);

uint32_t
p055_entry(uint32_t num_pe);

uint32_t
p056_entry(uint32_t num_pe);

uint32_t
p057_entry(uint32_t num_pe);

#endif
