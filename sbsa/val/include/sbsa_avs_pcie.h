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

#ifndef __SBSA_AVS_PCIE_H__
#define __SBSA_AVS_PCIE_H__

#define PCIE_EXTRACT_BDF_BUS(bdf)  ((bdf >> 16) & 0xFF)
#define PCIE_EXTRACT_BDF_DEV(bdf)  ((bdf >> 8) & 0xFF)
#define PCIE_EXTRACT_BDF_FUNC(bdf) (bdf & 0xFF)

#define PCIE_CREATE_BDF(seg, bus, dev, func) ()

#define PCIE_MAX_BUS   256
#define PCIE_MAX_DEV    32
#define PCIE_MAX_FUNC    8

void     val_pcie_write_cfg(uint32_t bdf, uint32_t offset, uint32_t data);
uint32_t val_pcie_read_cfg(uint32_t bdf, uint32_t offset);

typedef enum {
  PCIE_INFO_ECAM=1,
  PCIE_INFO_MCFG_ECAM
}PCIE_INFO_e;

uint64_t
val_pcie_get_info(PCIE_INFO_e type);

uint32_t
p001_entry(uint32_t num_pe);

uint32_t
p002_entry(uint32_t num_pe);

uint32_t
p003_entry(uint32_t num_pe);

uint32_t
p004_entry(uint32_t num_pe);

#endif
