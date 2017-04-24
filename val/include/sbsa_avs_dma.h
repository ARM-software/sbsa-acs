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

#ifndef __SBSA_AVS_DMA_H__
#define __SBSA_AVS_DMA_H__

#define WIDTH_BIT8     0x1
#define WIDTH_BIT16    0x2
#define WIDTH_BIT32    0x4

#define DMA_NOT_SUPPORTED  0x0
#define DMA_COHERENT       0x1
#define DMA_NOT_COHERENT   0x2
#define DMA_COHERENT_MASK  0xF

#define IOMMU_ATTACHED      0x10
#define IOMMU_ATTACHED_MASK 0xF0

#define PCI_EP      0x100
#define PCI_EP_MASK 0xF00

addr_t val_dma_mem_alloc(void **buffer, uint32_t size, uint32_t dev_index, uint32_t flags);

void val_dma_free_info_table(void);


#endif // __SBSA_AVS_DMA_H__
