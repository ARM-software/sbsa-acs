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
 * Generate a minimal ELF image to describe code.
 */

#include "genelf.h"

#include "arch.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>


#define SEGMENT_TYPE_CODE 0
#define SEGMENT_TYPE_DATA 1
#define BUFFER_SIZE 20    /* based on elf_add_string call */

struct string {
    struct string *next;
    unsigned int offset;        /* offset in section-header string section */
    char name[1];
};

struct segment {
    struct segment *next;
    struct elf *elf;
    void const *base;           /* Segment data in client's memory */
    unsigned long size;
    unsigned int type;
    unsigned long file_offset;
    unsigned int section_type;
    unsigned int link;
    unsigned int align;
    unsigned int entsize;
    struct string *name;
    unsigned int section_index;
};

struct symbol {
    struct symbol *next;
    struct string *name;
    void const *address;
    unsigned long size;
    unsigned int type;
    struct segment *segment;
};

struct elf {
    struct segment *segments;
    unsigned int n_segments;
    struct symbol *symbols;
    struct symbol *last_symbol;
    unsigned int n_symbols;
    void const *entry;          /* Entry point in client's memory */
    unsigned char *image;
    size_t image_size;
    int is_64:1;
    int do_segments:1;
    int do_sections:1;
    unsigned int e_ehsize;
    unsigned int e_phentsize;
    unsigned int e_shentsize;
    struct segment string_table;   /* special segment: string table section */
    struct string *strings;
    struct string *last_string;
    struct segment symbol_table;   /* special segment: symbol table section */
    unsigned int offset_to_segment_table;
    unsigned int offset_to_section_table;
    unsigned int offset_to_string_data;
    unsigned int offset_to_symbol_data;
};


static struct string *elf_add_string(elf_t e, char const *str);

#define STRING_TABLE_INDEX 1

static void elf_show(elf_t e);

elf_t elf_create(void)
{
    /* Freed in elf_destroy */
    elf_t e = (struct elf *)malloc(sizeof(struct elf));
    if (e) {
        memset(e, 0, sizeof(struct elf));
        e->is_64 = (sizeof(void *) == 8);
        e->e_ehsize = (e->is_64 ? 0x40 : 0x34);
        e->e_phentsize = (e->is_64 ? 0x38 : 0x20);
        e->e_shentsize = (e->is_64 ? 0x40 : 0x28);
        /* Set up string table. In a normal ELF file there would be separate
           string tables, .shstrtab for the section headers and .strtab for
           symbol strings. But we combine the two. */
        e->string_table.elf = e;
        e->string_table.section_type = 3;    /* SHT_STRTAB */
        e->string_table.size = 1;            /* for the leading NUL */
        e->string_table.align = 1;        
        e->string_table.name = elf_add_string(e, ".strtab");
        /* String section starts with a zero byte - so that the index of
           all the strings are non-zero */
        /* Set up symbol table. We don't create the name yet - if we have
           no symbols and don't create a symbol table section, we avoid
           adding the symbol table section name to the string table. */
        e->symbol_table.elf = e;
        e->symbol_table.section_type = 2;    /* SHT_SYMTAB */
        e->symbol_table.link = STRING_TABLE_INDEX;
        e->symbol_table.align = sizeof(uintptr_t);
        e->symbol_table.entsize = (e->is_64 ? 0x18 : 0x10);
    }
    return e;
}


static int segment_contains(struct segment const *s, void const *addr, unsigned long size)
{
    return ((unsigned char const *)s->base <= (unsigned char const *)addr &&
            (unsigned char const *)addr + size <= (unsigned char const *)s->base + s->size);
}


static struct segment *elf_find_segment(elf_t e, void const *addr, unsigned long size)
{
    struct segment *s;
    for (s = e->segments; s != NULL; s = s->next) {
        if (segment_contains(s, addr, size)) {
            return s;
        }
    }
    return NULL;
}


static struct segment *elf_add_segment(elf_t e, void const *base, unsigned long size, unsigned int type)
{
    struct segment *s;
    assert(e != NULL);
    assert(base != NULL);
    assert(size > 0);
    /* We should only build an image when we've added all the code segments. */
    assert(!e->image);

    s = (struct segment *)malloc(sizeof(struct segment));
    memset(s, 0, sizeof(struct segment));
    s->elf = e;
    s->type = type;
    s->base = base;
    s->size = size;
    s->section_type = 1;    /* SHT_PROGBITS */
    s->name = elf_add_string(e, (type == SEGMENT_TYPE_CODE ? ".text" : ".data"));
    /* add this section to the list */
    s->next = e->segments;
    e->segments = s;
    ++e->n_segments;
    return s;
}


struct segment *elf_add_code(elf_t e, void const *base, unsigned long size)
{
    return elf_add_segment(e, base, size, SEGMENT_TYPE_CODE);
}


/* TBD: eliminate duplicates */
static struct string *elf_add_string(elf_t e, char const *str)
{
    struct string *s;
    assert(e != NULL);
    assert(str != NULL);
    size_t len = strnlen(str, BUFFER_SIZE);
    assert(len > 0);
    s = (struct string *)malloc(sizeof(struct string) + len);
    assert(s);
    strncpy(s->name, str, len + 1);
    /* extend the string section */
    s->offset = e->string_table.size;
    e->string_table.size += len + 1;
    /* add this string to the end of the list */
    s->next = NULL;
    if (e->last_string == NULL) {
        e->strings = s;
    } else {
        e->last_string->next = s;
    }
    e->last_string = s;
    return s;
}


struct symbol *elf_add_symbol(elf_t e, char const *str, void const *address, unsigned long size)
{
    struct symbol *s;
    assert(e != NULL);
    assert(str != NULL);
    s = (struct symbol *)malloc(sizeof(struct symbol));
    assert(s);
    memset(s, 0, sizeof(struct symbol));
    if (!e->symbol_table.name) {
        e->symbol_table.name = elf_add_string(e, ".symtab");
    }
    s->name = elf_add_string(e, str);
    s->address = address;
    s->size = size;
    s->segment = elf_find_segment(e, address, size);
    if (s->segment == NULL) {
        fprintf(stderr, "Tried to create symbol \"%s\" @%p %lu outside segment\n",
            str, address, size);
        elf_show(e);
    }
    assert(s->segment != NULL);
    e->symbol_table.size += e->symbol_table.entsize;
    e->n_symbols += 1;
    if (e->last_symbol == NULL) {
        e->symbols = s;
    } else {
        e->last_symbol->next = s;
    }
    e->last_symbol = s;
    return s;
}


struct segment *elf_add_data(elf_t e, void const *base, unsigned long size)
{
    return elf_add_segment(e, base, size, SEGMENT_TYPE_DATA);
}


/*
 * Set entry point. We'd expect this to be within an executable (code) segment.
 */
void elf_set_entry(elf_t e, void const *entry)
{
    assert(e != NULL);
    e->entry = entry;
}


static unsigned int elf_segment_table_size(elf_t e)
{
    if (e->do_segments) {
        return e->e_phentsize * e->n_segments;
    } else {
        return 0;
    }
}

static unsigned int elf_n_sections(elf_t e)
{
    if (e->do_sections) {
        /* First add 1 for the NULL section table entry (SHN_UNDEF is 0) */
        unsigned int n = 1 + e->n_segments;
        if (e->string_table.size > 1) {
            n += 1;
        }
        if (e->symbol_table.size > 0) {
            n += 1;
        }
        return n;
    } else {
        return 0;
    }
}

static unsigned int elf_section_table_size(elf_t e)
{
    if (e->do_sections) {
        return e->e_shentsize * elf_n_sections(e);
    } else {
        return 0;
    }
}

#define STRING_ALIGNMENT 8

static unsigned int elf_string_section_size(elf_t e)
{
    if (e->do_sections) {
        unsigned int size = (e->string_table.size + (STRING_ALIGNMENT-1)) & ~(STRING_ALIGNMENT-1);
        assert((size & (STRING_ALIGNMENT-1)) == 0);
        assert(size >= e->string_table.size);
        return size;
    } else {
        return 0;
    }
}

static unsigned int elf_symbol_section_size(elf_t e)
{
    return e->do_sections ? e->symbol_table.size : 0;
}


/*
 * Find the total size of ELF headers and other metadata (e.g. string section)
 */
static unsigned int elf_total_headers(elf_t e)
{
    unsigned int n;
    n = e->e_ehsize + elf_segment_table_size(e) + elf_section_table_size(e);
    n += elf_string_section_size(e);
    n += elf_symbol_section_size(e);
    return n;
}


/*
 * Generate ELF header in memory, and also update the offsets
 */
static unsigned int elf_gen_header(elf_t e, unsigned char *h)
{
    unsigned int const len = e->e_ehsize;
    unsigned int offset;

    assert(e->do_segments || e->do_sections);
    memset(h, 0, len);
    e->offset_to_segment_table = len;
    /* prepare for writing sections, by updating the section counter */
    offset = len;
    offset += elf_segment_table_size(e);
    if (elf_n_sections(e) > 0) {
        e->offset_to_section_table = offset;
        offset += elf_section_table_size(e);
    } else {
        e->offset_to_section_table = 0;
    }
    if (e->strings != NULL) {
        e->offset_to_string_data = offset;
        offset += elf_string_section_size(e);
    } else {
        e->offset_to_string_data = 0;
    }
    if (e->symbols != NULL) {
        e->offset_to_symbol_data = offset;
        offset += e->symbol_table.size;
    } else {
        e->offset_to_symbol_data = 0;
    }
    h[0] = 0x7f;
    h[1] = 0x45;
    h[2] = 0x4c;
    h[3] = 0x46;
    h[4] = e->is_64 ? 2 : 1;
    h[5] = 1;    /* little endian */
    h[6] = 1;
    h[7] = 0;    /* System V */
    *(uint16_t *)(h + 0x10) = 2; /* 1: relocatable, 2: executable, 4: core */
#if defined(ARCH_AARCH32)
#define ELF_MACHINE_TYPE 0x28
#elif defined(ARCH_AARCH64)
#define ELF_MACHINE_TYPE 0xB7
#else
/* TBD: default to x86 */
#define ELF_MACHINE_TYPE (e->is_64 ? 0x3E : 0x03)
#endif
    *(uint16_t *)(h + 0x12) = ELF_MACHINE_TYPE;
    *(uint32_t *)(h + 0x14) = 1;
    *(uintptr_t *)(h + 0x18) = (uintptr_t)e->entry;
    *(uintptr_t *)(h + (e->is_64 ? 0x20 : 0x1C)) = e->offset_to_segment_table;
    *(uintptr_t *)(h + (e->is_64 ? 0x28 : 0x20)) = e->offset_to_section_table;  /* or zero */
    *(uint16_t *)(h + (e->is_64 ? 0x34 : 0x28)) = e->e_ehsize;
    *(uint16_t *)(h + (e->is_64 ? 0x36 : 0x2a)) = e->e_phentsize;
    *(uint16_t *)(h + (e->is_64 ? 0x38 : 0x2c)) = e->n_segments;
    *(uint16_t *)(h + (e->is_64 ? 0x3a : 0x2e)) = e->e_shentsize;  /* maybe zero if no sections? */
    *(uint16_t *)(h + (e->is_64 ? 0x3c : 0x30)) = elf_n_sections(e);
    *(uint16_t *)(h + (e->is_64 ? 0x3e : 0x32)) = STRING_TABLE_INDEX;
    return e->e_ehsize;
}


static unsigned int elf_gen_pheader(struct segment *s, unsigned char *h, unsigned int file_offset)
{
    elf_t const e = s->elf;
    assert(e != NULL);
    assert(e->e_phentsize > 0);
    assert(s->size > 0);
    memset(h, 0, e->e_phentsize);
    *(uint32_t *)(h + 0x00) = 1;    /* PT_LOAD */
    *(uint32_t *)(h + (e->is_64 ? 0x04 : 0x18)) = (s->type == SEGMENT_TYPE_CODE ? 0x05 : 0x04);   /* (PF_X+)PF_R */
    *(uintptr_t *)(h + (e->is_64 ? 0x08 : 0x04)) = file_offset;
    *(uintptr_t *)(h + (e->is_64 ? 0x20 : 0x10)) = s->size;
    *(uintptr_t *)(h + (e->is_64 ? 0x28 : 0x14)) = s->size;
    *(uintptr_t *)(h + (e->is_64 ? 0x10 : 0x08)) = (uintptr_t)s->base;
    return e->e_phentsize;
}


static unsigned int elf_gen_sheader(struct segment *s, unsigned char *h, unsigned int file_offset)
{
    elf_t const e = s->elf;
    unsigned int flags = 0;
    assert(e != NULL);
    assert(e->e_shentsize > 0);
    assert(e->e_shentsize <= 0x100);
    if (s->section_type == 1) {   /* SHT_PROGBITS */
        flags |= 0x002;   /* SHF_ALLOC */
        if (s->type == SEGMENT_TYPE_CODE) {
            flags |= 0x004;   /* SHF_EXECINSTR */
        }
    } else if (s->section_type == 3) {
        //flags |= 0x020;   /* SHF_STRINGS */
    }
    /* now clear the section table entry and fill in the fields */
    memset(h, 0, e->e_shentsize);
    if (s->name != NULL) {
        /* Index in the section header string table of the section's name. */
        *(uint32_t *)(h + 0x00) = s->name->offset;
    }
    *(uint32_t *)(h + 0x04) = s->section_type;
    *(uintptr_t *)(h + 0x08) = flags;
    *(uintptr_t *)(h + (e->is_64 ? 0x10 : 0x0c)) = (uintptr_t)s->base;
    *(uintptr_t *)(h + (e->is_64 ? 0x18 : 0x10)) = file_offset;
    *(uintptr_t *)(h + (e->is_64 ? 0x20 : 0x14)) = (uintptr_t)s->size;    
    *(uint32_t *)(h + (e->is_64 ? 0x28 : 0x18)) = s->link;
    *(uintptr_t *)(h + (e->is_64 ? 0x30 : 0x20)) = (uintptr_t)s->align;
    *(uintptr_t *)(h + (e->is_64 ? 0x38 : 0x24)) = (uintptr_t)s->entsize;
    return e->e_shentsize;
}


static void elf_show(elf_t e)
{
    fprintf(stderr, "ELF:\n");
    fprintf(stderr, "  Total header size: %u\n", elf_total_headers(e));
    {
        struct segment const *s;
        fprintf(stderr, "  Segments (%u):\n", e->n_segments);
        for (s = e->segments; s != NULL; s = s->next) {
            fprintf(stderr, "    %016llx %016llx type=%u",
                (unsigned long long)s->base, (unsigned long long)s->size, s->type);
            if (s->name) {
                fprintf(stderr, " name=\"%s\"@%u", s->name->name, s->name->offset);
            }
            fprintf(stderr, "\n");
        }
    }
    {
        struct string const *s;
        fprintf(stderr, "  Strings (total %u bytes):\n", (unsigned int)e->string_table.size);
        for (s = e->strings; s != NULL; s = s->next) {
            fprintf(stderr, "    \"%s\"@%u\n", s->name, s->offset);
        }
    }
    {
        struct symbol const *s;
        fprintf(stderr, "  Symbols (%u)\n", e->n_symbols);
        for (s = e->symbols; s != NULL; s = s->next) {
            fprintf(stderr, "    %s @ %p size %lu",
                (s->name ? s->name->name : "<no name>"), s->address, s->size);
            if (s->segment) {
                fprintf(stderr, " in #%p", s->segment);
            }
            fprintf(stderr, "\n");
        }
    }
}


/*
 * Generate a contiguous ELF image in memory.
 * The ELF image includes the ELF header and tables, but not the code/data.
 * The tables point to the actual code and data in their current locations.
 * So the total image is not contiguous, rather we're generating a
 * contiguous descriptor for memory areas.
 *
 * We dynamically allocate a buffer, and the caller must free it.
 */
static void *elf_gen_image(elf_t e, int file_offsets)
{
    void *image;
    unsigned char *image_base;
    struct segment *s;
    size_t image_size;
    unsigned int file_offset_for_data;
    unsigned char *p;
    assert(e != NULL);
    assert(e->n_segments > 0);

    e->do_segments = 1;
    e->do_sections = 1;
    if (0) {
        elf_show(e);
    }
    image_size = elf_total_headers(e);
    image = malloc(image_size);
    if (!image) {
        return NULL;
    }
    image_base = (unsigned char *)image;
    p = image_base;
    p += elf_gen_header(e, image);
    file_offset_for_data = elf_total_headers(e);
    for (s = e->segments; s != NULL; s = s->next) {
        s->file_offset = (file_offsets ? file_offset_for_data : 0);
        file_offset_for_data += s->size;
    }
    if (e->do_segments) {
        for (s = e->segments; s != NULL; s = s->next) {        
            unsigned int len = elf_gen_pheader(s, p, s->file_offset);
            p += len;
        }
    }
    if (e->do_sections) {
        struct string *str;
        struct symbol *sym;
        unsigned int section_index = 0;
        /* Section table */
        assert((p - image_base) == e->offset_to_section_table);
        /* Start with section table entries for special (non-segment) sections */
        /* Null entry */
        memset(p, 0, e->e_shentsize);
        p += e->e_shentsize;
        ++section_index;
        /* Entry for the string section. We always expect to generate this,
           if only for the section names. */
        if (e->string_table.size > 1) {   
            //e->string_table.base = image_base + e->offset_to_string_data;
            unsigned int len = elf_gen_sheader(&e->string_table, p, e->offset_to_string_data);
            p += len;
            ++section_index;
        }
        if (e->symbol_table.size > 0) {
            unsigned int len = elf_gen_sheader(&e->symbol_table, p, e->offset_to_symbol_data);
            p += len;
            ++section_index;
        }
        /* Iterate the segments again, but this time as sections */
        for (s = e->segments; s != NULL; s = s->next) {
            s->section_index = section_index;
            unsigned int len = elf_gen_sheader(s, p, s->file_offset);
            p += len;
            ++section_index;
        }
        /* We've completed the section table. */
        assert(section_index == elf_n_sections(e));
        /* Now output the special (metadata) section contents. */
        /* string section */
        assert((p - image_base) == e->offset_to_string_data);
        unsigned int sp = 0;
        /* start with a NUL */
        *p++ = '\0';
        sp += 1;
        for (str = e->strings; str != NULL; str = str->next) {
            unsigned int templen = strnlen(str->name, BUFFER_SIZE);
            strncpy((char *)p, str->name, templen + 1);
            unsigned int len = templen + 1;
            p += len;
            sp += len;
        }
        while ((sp & (STRING_ALIGNMENT-1)) != 0) {
            *p++ = '\0';
            sp += 1;
        }
        /* symbol section */
        for (sym = e->symbols; sym != NULL; sym = sym->next) {
            unsigned char info = 0;   /* type in low nybble, binding in high nybble */
            memset(p, 0, e->symbol_table.entsize);
            *(uint32_t *)(p + 0) = sym->name ? sym->name->offset : 0;
            *(uintptr_t *)(p + (e->is_64 ? 0x08 : 0x04)) = (uintptr_t)sym->address;
            *(uintptr_t *)(p + (e->is_64 ? 0x10 : 0x08)) = sym->size;
            *(unsigned char *)(p + (e->is_64 ? 0x04 : 0x0c)) = info;
            if (sym->segment != NULL) {
                *(uint16_t *)(p + (e->is_64 ? 0x06 : 0x0e)) = sym->segment->section_index;
            }
            p += e->symbol_table.entsize;
        }
    }
    /* check we hit the predicted end of buffer */
    if ((size_t)(p - (unsigned char *)image) != image_size) {
        fprintf(stderr, "Image:\n");
        fprintf(stderr, "  Start:     %p\n", image);
        fprintf(stderr, "  Segment table: %lx\n", (unsigned long)e->offset_to_segment_table);
        fprintf(stderr, "    Size:        %lx\n", (unsigned long)elf_segment_table_size(e));
        fprintf(stderr, "  Section table: %lx\n", (unsigned long)e->offset_to_section_table);
        fprintf(stderr, "    Size:        %lx\n", (unsigned long)elf_section_table_size(e));
        fprintf(stderr, "  Strings:       %lx\n", (unsigned long)e->offset_to_string_data);
        fprintf(stderr, "    Size:        %lx\n", (unsigned long)elf_string_section_size(e));
        fprintf(stderr, "  Symbols:       %lx\n", (unsigned long)e->offset_to_symbol_data);
        fprintf(stderr, "    Size:        %lx\n", (unsigned long)e->symbol_table.size);
        fprintf(stderr, "  Current:       %lx\n", (unsigned long)(p - image_base));
        fprintf(stderr, "  Predicted:     %lx\n", (unsigned long)image_size);
    }
    assert((p - (unsigned char *)image) == image_size);
    e->image_size = image_size;
    return image;
}


/*
Generate the ELF image in memory. Idempotent.
*/
void const *elf_image(elf_t e)
{
    assert(e != NULL);
    if (!e->image) {
        e->image = elf_gen_image(e, 0);
    }
    return e->image;
}


unsigned int elf_image_size(elf_t e)
{
    (void)elf_image(e);
    return e->image_size;
}



int elf_dump(elf_t e, char const *fn)
{
    void *image;
    struct segment *s;
    FILE *fd;

    assert(e != NULL);

    fd = fopen(fn, "wb");
    if (!fd) {
        return -1;
    }
    /* generate and output the contiguous part of the ELF file, same as
       we generate in-memory */
    image = elf_gen_image(e, /*file_offsets=*/1);
    fwrite(image, e->image_size, 1, fd);
    /* last of all, output the data contents from memory to the file */
    for (s = e->segments; s != NULL; s = s->next) {
        assert(s->size > 0);
        fwrite(s->base, s->size, 1, fd);
    }
    fclose(fd);
    return 0;
}


void elf_destroy(elf_t e)
{
    if (e) {
        while (e->segments != NULL) {
            struct segment *s = e->segments;
            e->segments = s->next;
            free(s);
        }
        while (e->strings != NULL) {
            struct string *s = e->strings;
            e->strings = s->next;
            free(s);
        }
        while (e->symbols != NULL) {
            struct symbol *s = e->symbols;
            e->symbols = s->next;
            free(s);
        }
        if (e->image) {
            free(e->image);
        }
        free(e);
    }
}

