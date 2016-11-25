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

#ifndef __SBSA_PCIE_ENUM_H__
#define __SBSA_PCIE_ENUM_H__


#define PCIE_EXTRACT_BDF_SEG(bdf)  ((bdf >> 24) & 0xFF)
#define PCIE_EXTRACT_BDF_BUS(bdf)  ((bdf >> 16) & 0xFF)
#define PCIE_EXTRACT_BDF_DEV(bdf)  ((bdf >> 8) & 0xFF)
#define PCIE_EXTRACT_BDF_FUNC(bdf) (bdf & 0xFF)

#define PCIE_CREATE_BDF(Seg, Bus, Dev, Func) ((Seg << 24) | (Bus << 16) | (Dev << 8) | Func)


UINT32
incrementBusDev(UINT32 StartBdf);

UINT32
palPcieGetBdf(UINT32 class_code, UINT32 start_busdev);

UINT64
palPcieGetBase(UINT32 bdf, UINT32 bar_index);

#endif
