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
 * Generate a runnable workload in memory, based on a set of workload characteristics.
 */

/*
 * Have to define this to get sigaction, MAP_ANONYMOUS etc.
 * Define it now in case our headers pull in any system headers.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "loadgenp.h"

#include "arch.h"
#include "genelf.h"
#include "denormals.h"

#include <sys/mman.h>
#include <unistd.h>
#include <execinfo.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>


int workload_verbose = 0;


static unsigned long round_size_to_pages(unsigned long size)
{
    return round_size(size, sysconf(_SC_PAGESIZE));
}


/*
 * Find out the system's huge page size, so that we can safely use MAP_HUGETLB.
 * MAP_HUGETLB allocations must be rounded up to this size.
 *
 * Return 0 if we can't find the size.
 */
static unsigned long huge_page_size(void)
{
    static unsigned long size = 1;   /* Never valid; initiates discovery */
    if (size == 1) {
        FILE *fd = fopen("/proc/meminfo", "r");
        char buf[100];
        while (fgets(buf, sizeof buf, fd)) {
            if (!memcmp(buf, "Hugepagesize:", 13)) {
                long sizek = 0;
                if (sscanf(buf+13, "%ld", &sizek) == 1) {           
                    size = sizek * 1024;
                    if (size != 0) {
                        assert((size & -size) == size);   /* power of 2 */
                        assert((long)size > sysconf(_SC_PAGESIZE));  /* TBD: always? */
                    }
                    break;
                }
            }
        }
        fclose(fd);
        if (!size) {
            fprintf(stderr, "Couldn't get huge page size\n");
            size = 0;
        }
    }
    return size;
}


static unsigned long total_mmap_size = 0;
static unsigned int total_mmap_count = 0;


/*
 * Allocate some memory, e.g. for data or code working set.
 * The memory is page-aligned, so that we can later change its protection.
 * On return, the workload_mem structure is filled in.
 * TBD: What should we do for size zero?
 */
void *load_alloc_mem(struct workload_mem *m)
{
    void *p;
    unsigned long rsize;
    /* MAP_POPULATE is documented as pre-populating the page tables. */
    int flags = MAP_PRIVATE|MAP_ANONYMOUS;
    assert(m->size_req > 0);
    rsize = round_size_to_pages(m->size_req);
    /* MAP_HUGETLB is only available if the size is a multiple of the
       huge page size. When the user requests HUGETLB for a smaller allocation,
       do they want us to round up the size, or ignore HUGETLB? */
    if ((m->is_hugepage && rsize >= huge_page_size()) ||
        m->is_force_hugepage) {
        /* Is it even worth doing this if /proc/sys/vm/nr_hugepages is 0? */
        flags |= MAP_HUGETLB;
        rsize = round_size(rsize, huge_page_size());
    }
    /* We can't force mmap() to allocate with small pages.
       But we can allocate without population, then madvise(MADV_NOHUGEPAGE),
       then populate. */
    if (!m->is_no_hugepage) {
        flags |= MAP_POPULATE;
    }
    m->size = rsize;
    m->base = NULL;
    if (1) {
        /* _GNU_SOURCE should have ensured we see MAP_ANONYMOUS */
        unsigned int prot = PROT_READ|PROT_WRITE;
        if (m->is_exec) {
            prot |= PROT_EXEC;
        }
        assert(!(flags & MAP_HUGETLB) || (rsize % huge_page_size()) == 0);
        if (workload_verbose) {
            fprintf(stderr, "loadgen: mmap %lu/%#lx bytes, prot=%04x, flags=%04x\n",
                (unsigned long)rsize, (unsigned long)rsize, (unsigned int)prot, (unsigned int)flags);
        }
        p = mmap(NULL, rsize, prot, flags, -1, 0);
        if (p == MAP_FAILED && (flags & MAP_HUGETLB) != 0 && errno == ENOMEM) {
            if (workload_verbose) {
                fprintf(stderr, "loadgen: mmap(MAP_HUGETLB) failed, will retry and use madvise(MADV_HUGEPAGE)\n");
            }
            /* Try again, using madvise() to force huge pages */
            flags = flags & ~MAP_HUGETLB;
            /* We don't want the pages to be allocated until we've done madvise() */
            flags = flags & ~MAP_POPULATE;
            p = mmap(NULL, rsize, prot, flags, -1, 0);
        }
        if (p == MAP_FAILED) {
            perror("mmap");
            fprintf(stderr, "Failed to allocate %lu/%#lx bytes (flags 0x%x, page size %lu): total out %u, %lu bytes\n",
                (unsigned long)rsize, (unsigned long)rsize,
                (unsigned int)flags,
                (unsigned long)sysconf(_SC_PAGESIZE),
                total_mmap_count,
                (unsigned long)total_mmap_size);
            return NULL;
        }
        total_mmap_count += 1;
        total_mmap_size += rsize;
        m->is_mmap = 1;
        /* We don't need to use MADV_HUGEPAGE, as we will have mmap'ed with MAP_HUGETLB */
        if ((m->is_hugepage || m->is_force_hugepage) && !(flags & MAP_HUGETLB)) {
#ifdef MADV_HUGEPAGE
            int rc = madvise(p, rsize, MADV_HUGEPAGE);
            if (rc < 0) {
                perror("madvise");
                m->is_hugepage = 0;
            }
#else
            fprintf(stderr, "MADV_HUGEPAGE not available when this module was built\n");
            m->is_hugepage = 0;   
#endif
        }
        if (m->is_no_hugepage) {
#ifdef MADV_NOHUGEPAGE
            int rc = madvise(p, rsize, MADV_NOHUGEPAGE);
            if (rc < 0) {
                perror("madvise");
                m->is_no_hugepage = 0;
            }
#else
            fprintf(stderr, "MADV_NOHUGEPAGE not available when this module was built\n");
            m->is_no_hugepage = 0;
#endif
        }        
    }
    m->base = p;
    if (workload_verbose) {
        fprintf(stderr, "loadgen: alloc %p size %lu\n", m->base, m->size);
    }
    assert(p != NULL);
    return p;
}


void load_free_mem(struct workload_mem *m)
{
    if (m->base) {
        if (workload_verbose) {
            fprintf(stderr, "loadgen: free %p size %lu\n", m->base, m->size);
        }
        if (m->is_mmap) {
            unsigned long rsize = round_size_to_pages(m->size);
            assert(total_mmap_size >= rsize);
            munmap(m->base, rsize);
            total_mmap_count -= 1;
            total_mmap_size -= rsize;
        } else {
            free(m->base);
        }
        m->base = NULL;
    }
}


/*
This function has the same API as the workload we create, and can be used
as a stub when we're diagnosing crashes with the workload.
*/
static void *dummy_workload_code(void *p, void *offsetp, void *scratch)
{
    unsigned long offset = (unsigned long)offsetp;
    void **actualp = (void **)((unsigned char *)p + offset);
    void *next;
    if (0) {
        fprintf(stderr, "data step: input pointer %p, add %ld, actual address %p:\n",
            p, offset, actualp);
    }
    next = *actualp;
    if (0) {
        fprintf(stderr, "  -> %p\n", next);
    }
    return next;
}


static void *dummy_workload_code_nodata(void *p, void *unused, void *scratch)
{
    return 0;
}


static int workload_code_is_trivial(Character const *c)
{
    if (c->debug_flags & WORKLOAD_DEBUG_DUMMY_CODE) {
        return 1;
    }
    return (c->inst_working_set == 0 && c->fp_intensity == 0);
}


void workload_init(Character *c)
{
    memset(c, 0, sizeof(Character));
    c->fp_value = 1.0;
    c->fp_value2 = 1.0;
    c->inst_target = 50000;
}


/*
 * Construct a new workload.
 * All memory needed for this workload is newly allocated.
 * Return NULL if we can't create the workload.
 */
Workload *workload_create(Character const *c)
{
    void *data;
    Workload *w = (Workload *)malloc(sizeof(Workload));

    if (workload_verbose) {
        fprintf(stderr, "loadgen: creating workload...\n");
    }
    memset(w, 0, sizeof(Workload));
    w->elf_image = elf_create();
    /* Take a copy of the supplied workload characteristics.
       Later changes made by the caller will not take effect. */
    w->c = *c;
    data = load_construct_data(&w->c, &w->data_mem);
    if (c->data_working_set > 0 && !data) {
        /* Data working set was requested but couldn't be constructed */
        free(w);
        if (workload_verbose) {
            fprintf(stderr, "loadgen: couldn't create data working set\n");
        }
        return NULL;    
    }
    if (w->data_mem.base != NULL) {
        elf_add_data(w->elf_image, w->data_mem.base, w->data_mem.size);
    }
    if (workload_code_is_trivial(c)) {
        w->expected.n[COUNT_INST] = 100;    /* Just a guess */
        if (c->data_working_set) {
            w->entry = &dummy_workload_code;
            w->expected.n[COUNT_INST_RD] = 1;
            w->expected.n[COUNT_BYTES_RD] = sizeof(void *);
        } else {
            /* We mustn't try to run a chain pointer step when there's no
               data working set. */
            w->entry = &dummy_workload_code_nodata;
        }
    } else if (load_construct_code(w)) {
        /* We've now dynamically constructed a workload code sequence. */
        assert(w->entry != NULL);
    } else {
        /* Code was requested but couldn't be constructed -
           e.g. maybe self-modifying code has been locked out by the OS.
           If we didn't need a code workload with specific code footprint,
           we might be able to fall back to a predefined function
           that would iterate through the data working set. But we don't
           currently support that. */
        free(w);
        if (workload_verbose) {
            fprintf(stderr, "loadgen: couldn't create code working set\n");
        }
        return NULL;
    }
    w->entry_args[0] = data;
    w->entry_args[1] = (void *)(unsigned long)w->c.data_pointer_offset;
    if (workload_verbose) {
        fprintf(stderr, "loadgen: %p: set up workload entry %p with args [%p, %p]\n",
            w, w->entry,
            w->entry_args[0], w->entry_args[1]);
        fprint_code(stderr, w->entry, 32);
    }

    /* WORKLOAD_KEEP presets the workload reference counter to a high
       number so that the workload isn't deleted even when not in use.
       When we later want to delete the workload, we subtract WORKLOAD_KEEP
       so that when the reference counter drops to zero the workload can
       be deleted. */
#define WORKLOAD_KEEP 100000
    w->references = WORKLOAD_KEEP;        /* Make it stick until deleted */
    if (0) {
        if (workload_verbose) {
            fprintf(stderr, "loadgen: %p: trial run...\n", w);
        }
        /* Run the workload once, just to check */
        workload_run_once(w);
        if (workload_verbose) {
            fprintf(stderr, "loadgen: %p: trial run successful\n", w);
        }
    }
    assert(w->references == WORKLOAD_KEEP);
    return w;
}


/*
 * Create an image file containing the code for the workload.
 * The flags option currently isn't used.
 * The file is generated in ELF format for direct viewing with e.g. "objdump -d".
 * TBD: generate jitdump.
 */
int workload_dump(Workload *w, char const *fn, unsigned int flags)
{
    int rc;
    if (workload_verbose) {
        fprintf(stderr, "loadgen: %p: dumping image: %s\n", w, fn);
    }
    rc = elf_dump(w->elf_image, fn);
    if (workload_verbose) {
        fprintf(stderr, "loadgen: %p: dumped image: %s\n", w, fn);
    }
    return rc;
}


/*
 * Clean up and destroy a workload.
 * The workload must not be currently running.
 * This function must be called once, by only one thread.
 */
static void workload_destroy(Workload *w)
{
    if (workload_verbose) {
        fprintf(stderr, "loadgen: %p: destroy\n", w);
    }
    if (w->references != 0) {
        fprintf(stderr, "** loadgen: %p: workload_destroy called when references=%d\n", w, w->references);
        /* How did this happen? We shouldn't have been called unless and until
           'running' was finally set to zero, and no more iterations should
           have been started after that point. */
    }
    assert(!w->references);
#if 0
    if (w->elf_image != NULL) {
        elf_destroy(w->elf_image);
        w->elf_image = NULL;
    }
#endif
    if (!(w->c.debug_flags & WORKLOAD_DEBUG_NO_FREE)) {
        load_free_mem(&w->data_mem);
        load_free_code(w);
    } else {
        fprintf(stderr, "loadgen: %p: debug request to not free working sets\n", w);
    }
    /* If somehow we've still got a workload running, it's possibly
       crashed by now, as we've released the code and data. */
    assert(!w->references);
    free(w);
    if (workload_verbose) {
        fprintf(stderr, "loadgen: %p (freed): workload destroyed\n", w);
    }
}


/*
 * User request to delete the workload. If the workload is not in use (running),
 * it is deleted right now, otherwise it is marked for deletion when the last
 * runner completes.
 * This function must be called only once per workload.
 */
int workload_free(Workload *w)
{
    int destroyed = 0;
    if (w) {
        int now_running;
        if (workload_verbose) {
            fprintf(stderr, "loadgen: %p: free\n", w);
        }
        now_running = __sync_sub_and_fetch(&w->references, WORKLOAD_KEEP);
        if (!now_running) {
            workload_destroy(w);
            destroyed = 1;
        } else {
            if (workload_verbose) {
                fprintf(stderr, "loadgen: %p: pending delete (%d)\n", w, now_running);
            }
            assert(destroyed == 0);
        }
    }
    return destroyed;
}


void workload_add_reference(Workload *w)
{
    __sync_fetch_and_add(&w->references, 1);
}


void workload_remove_reference(Workload *w)
{
    int now_running = __sync_sub_and_fetch(&w->references, 1);
    assert(now_running >= 0);
    if (__builtin_expect((now_running == 0), 0)) {
        if (workload_verbose) {
            fprintf(stderr, "loadgen: %p: workload was marked for delete, now deleting\n", w);
        }
        workload_destroy(w);
    }
}


/*
This is a dubious way to ensure the FP registers have known values
before we go into the generated workload code. Only the first three
registers need to be set. The workload code will copy them to the
remaining work registers.
*/
__attribute__((noinline,used))
static double fp_regs_clearer_double(double a, double b, double c)
{
    __asm__("");
    return a + b + c;
}

__attribute__((noinline,used))
static float fp_regs_clearer_float(float a, float b, float c)
{
    __asm__("");
    return a + b + c;
}


/*
 * As preparation for running a sequence of generated floating-point instructions,
 * set up the floating-point work registers.
 */
static void fp_regs_clear_double(double workval, double constval)
{
    fp_regs_clearer_double(0.0, workval, constval);
}

static void fp_regs_clear_float(double workval, double constval)
{
    fp_regs_clearer_float(0.0, workval, constval);
}


/*
If we want to see the effect of denormals on
instruction timings, when we're using the output of one instruction as
a data input of the next, we need to ensure that denormals don't quickly
evaporate. So for example MUL of two denormals would go to zero.
A better result is to MUL a denormal with a value close to 1 (but not
exactly 1 as that might take a fast path):
 
   x = DENORMAL;
   x = NEAR1 * x;
   x = NEAR1 * x;

or

   x = DENORMAL;
   x = NEAR1 / x;    // x now large finite (not denormal)
   x = NEAR1 / x;    // x now denormal again

For division, in order that NEAR1/x doesn't overflow to infinity,
we should pick a value of NEAR1 that's somewhat less than 1,
e.g. 1e-7 for float and 1e-15 for double.
*/


static void signal_handler(int sig, siginfo_t *si, void *unused)
{
    unsigned int i;
    if (sig == SIGILL) {
        unsigned char const *p = (unsigned char const *)si->si_addr;
        fprintf(stderr, "loadgen: SIGILL from illegal instruction at %p\n", p);
        fprint_code(stderr, si->si_addr, 32);
    } else if (sig == SIGSEGV) {
        unsigned char const *p = (unsigned char const *)si->si_addr;
        fprintf(stderr, "loadgen: SIGSEGV from illegal address at %p\n", p);
    } else {
        fprintf(stderr, "loadgen: signal %d at %p\n", sig, si->si_addr);
    }
    const int MAX_BACKTRACE = 20;
    void *array[MAX_BACKTRACE];
    size_t size = backtrace(array, MAX_BACKTRACE);
    char **strings = backtrace_symbols(array, size);
    for (i = 0; i < size; ++i) {
        printf("%s\n", strings[i]);
    }
    free(strings);
    exit(EXIT_FAILURE);
}


__attribute__((noinline))
static void *workload_enter(Workload *w, void *data, unsigned int n_iters)
{
    unsigned int i;
    /* Run some iterations of the workload. This may take some time. */
    /* FP operations will use the ambient contents of the FP registers. */
    for (i = 0; i < n_iters; ++i) {
        if (0) {
            fprintf(stderr, "loadgen: %p: run iteration %u with %p\n", w, i, data);
        }
        data = (w->entry)(data, w->entry_args[1], w->scratch);
    }
    return data;
}


/*
Run the next 'chunk' of a workload using some prior state,
perhaps the state returned from last time in argument 0.
Other arguments will be picked up from the work descriptor.

This may be run from multiple threads, so we should avoid updating
any shared state.
*/
void *workload_run(Workload *w, void *data, unsigned int n_iters)
{
    assert(w != NULL);
    if (workload_verbose >= 2) {
        fprintf(stderr, "loadgen: %p: run workload entry %p with args [%p (originally %p), %p], %u iterations\n",
            w, w->entry,
            data, w->entry_args[0], w->entry_args[1],
            n_iters);
    }
    static int sigdone;
    if (!sigdone) {
        int rc;
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        sa.sa_sigaction = signal_handler;
        sigdone = 1;
        rc = sigaction(SIGSEGV, &sa, NULL);
        if (rc != -1) {
            rc = sigaction(SIGILL, &sa, NULL);
        }
        if (rc == -1) {
            perror("sigaction");
            fprintf(stderr, "loadgen: %p: could not set trap handler\n", w);
        }
    }
    //fp_regs_clear(1.0);
    if (!denormals_set_enabled((w->c.fp_flags & FP_FLAG_DENORMAL_FTZ) == 0)) {
        /* TBD we should have done this earlier and returned a failure code */
        fprintf(stderr, "** Could not set required denormal handling mode\n");
        assert(0);
    }
#if defined(__x86_64__)
    /* On x86 with AVX, we can take an ongoing penalty when doing XMM operations,
       on 128-bit values, if the upper 128 bits of 256-bit registers are non-zero.
       To fix this, we need to do a VZEROALL or VZEROUPPER.
       The performance penalty seems to be about 1 cycle per operation.
       It doesn't seem to be well indicated by hardware events. It doesn't
       correspond to an assist, for example.
     */
    __asm__("vzeroupper");
    //_mm256_zeroupper();
#endif
    /* Provide suitable input values.
       The sequence of FP operations modifies the values in registers so
       (the back-to-back chaining is critical to measuring latency) so the
       values we provide here may quickly vanish, diverge etc.  TBD do better.
    */
    if (w->c.fp_precision == FP_PRECISION_DOUBLE) {
        fp_regs_clear_double((w->c.fp_flags & FP_FLAG_DENORMAL_GEN) ? DOUBLE_DENORMAL : w->c.fp_value,
                             (w->c.fp_operation == FP_OP_DIV ? 1e-15 : w->c.fp_value2));
    } else {
        fp_regs_clear_float((w->c.fp_flags & FP_FLAG_DENORMAL_GEN) ? FLOAT_DENORMAL : (float)w->c.fp_value,
                            (w->c.fp_operation == FP_OP_DIV ? 1e-7 : (float)w->c.fp_value2));
    }
    data = workload_enter(w, data, n_iters);
    return data;
}


void workload_run_once(Workload *w)
{
    void *ndata;
    if (workload_verbose) {
        fprintf(stderr, "loadgen: %p: run workload entry %p with args %p %p\n",
            w, w->entry,
            w->entry_args[0], w->entry_args[1]);
    }
    ndata = workload_run(w, w->entry_args[0], 1);
    if (workload_verbose) {
        fprintf(stderr, "loadgen: %p: workload returned %p\n", w, ndata);
    }   
}


/* end of loadgen.c */

