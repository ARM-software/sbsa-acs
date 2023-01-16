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
/*
 * Generate an ELF image describing code regions.
 */

#ifndef __included_genelf_h
#define __included_genelf_h

typedef struct elf *elf_t;
typedef struct segment *elf_segment_t;
typedef struct symbol *elf_symbol_t;

elf_t elf_create(void);

/*
 * Add a code segment.
 */
elf_segment_t elf_add_code(elf_t, void const *addr, unsigned long size);

/*
 * Add a data segment.
 */
elf_segment_t elf_add_data(elf_t, void const *addr, unsigned long size);

/*
 * Add a symbol.
 */
elf_symbol_t elf_add_symbol(elf_t, char const *name, void const *addr, unsigned long size);

/*
 * Set the entry point.
 */
void elf_set_entry(elf_t, void const *entry);

/*
 * Return a location to the ELF image in memory.
 */
void const *elf_image(elf_t);

/*
 * Return the image size in bytes, exclusive of the actual code/data.
 */
unsigned int elf_image_size(elf_t);

/*
 * Write the ELF image to a file, including the described code.
 */
int elf_dump(elf_t, char const *fn);

/*
 * Destroy an ELF object, freeing its image in memory.
 */
void elf_destroy(elf_t);

#endif /* included */

