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
Given a block of machine code, generated elsewhere, prepare it for execution.

Platform-specific functions are used for cache unification etc.
*/

#include "prepcode.h"

#include "arch.h"

#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>       /* for perror */
#include <assert.h>


#ifdef ARCH_ARM
//#if defined(__ARMEL__)
//#include <asm/cachectl.h>
extern void __clear_cache(char *beg, char *end);
//#endif

int cacheflush(char *p, int s, int flag)
{
    __clear_cache(p, p+s);
    return 0;
}
#define BCACHE 3
#endif

#define GDB_JIT
#ifdef GDB_JIT
typedef enum {
    JIT_NOACTION = 0,
    JIT_REGISTER_FN,
    JIT_UNREGISTER_FN
} jit_actions_t;

struct jit_code_entry {
    struct jit_code_entry *next_entry;
    struct jit_code_entry *prev_entry;
    char const *symfile_addr;     /* sic - not an 'unsigned char *' */
    unsigned long long symfile_size;
};

struct jit_descriptor {
    unsigned int version;
    unsigned int action_flag;
    struct jit_code_entry *relevant_entry;
    struct jit_code_entry *first_entry;
};

struct jit_descriptor __jit_debug_descriptor = {1,0,0,0};

void __attribute__((noinline)) __jit_debug_register_code() {}
#endif /* GDB_JIT */


int prepare_code_protection(void const *p, size_t size)
{
    assert(((unsigned long)p & (sysconf(_SC_PAGESIZE)-1)) == 0);
    assert((size & (sysconf(_SC_PAGESIZE)-1)) == 0);
    if (0 > mprotect((void *)p, size, PROT_READ|PROT_EXEC)) {
        perror("mprotect");
        fprintf(stderr, "prepcode: failed to mark %p size %#lx as executable\n",
            p, (unsigned long)size);
        return -1;
    }
    return 0;
}


int prepare_code_coherence(void const *p, size_t size)
{
#ifdef ARCH_ARM
    if (0 > cacheflush((char *)p, size, BCACHE)) {
        perror("cacheflush");
        return -1;
    }
#else
    /* On other platforms, the D-cache and I-cache are coherent */
#endif
    return 0;
}


static struct jit_code_entry the_entry = {NULL, NULL, NULL, 0};


int prepare_code_elf(void const *p, size_t size, unsigned int flags, void const *elf, size_t elf_size)
{
    int rc;
    assert(p != NULL);
    if (flags & PREPCODE_DEBUGGER) {
#ifdef GDB_JIT
        /* TBD - we should support several descriptors active */
        the_entry.symfile_addr = (char *)elf;
        the_entry.symfile_size = elf_size;
        __jit_debug_descriptor.action_flag = JIT_REGISTER_FN;
        __jit_debug_descriptor.relevant_entry = &the_entry;
        __jit_debug_descriptor.first_entry = &the_entry;
        __jit_debug_register_code();
        /* We need to call this again when we've finished with the code buffer */
#endif /* GDB_JIT */
    }
    if (flags & PREPCODE_PROTECT) {
        rc = prepare_code_protection(p, size);
        if (rc) {
            return rc;
        }
    }
    if ((flags & PREPCODE_COHERENCE) && !(flags & PREPCODE_PROTECT)) {
        /* If we've done mprotect() above, we believe we don't have to do
           a userspace cache-unification sequence as well. */
        rc = prepare_code_coherence(p, size);
        if (rc) {
            return rc;
        }
    }
    return 0;
}


int prepare_code(void const *p, size_t size, unsigned int flags)
{
    return prepare_code_elf(p, size, flags, NULL, 0);
}


int unprepare_code(void const *p, size_t size, unsigned int flags)
{
    if (flags & PREPCODE_DEBUGGER) {
#ifdef GDB_JIT
        /* TBD - we might have several descriptors active */
        __jit_debug_descriptor.action_flag = JIT_UNREGISTER_FN;
        __jit_debug_register_code();
#endif /* GDB_JIT */
    }
    return 0;
}


/* end of prepcode.c */

