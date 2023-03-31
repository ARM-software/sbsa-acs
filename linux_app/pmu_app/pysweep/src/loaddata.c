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

#include "loadgenp.h"

#include "arch.h"

#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


/*
Test the length of a chain of data.  The chain must be completely circular
i.e. we must get back to the beginning.  Given bad data, this function will
crash or loop infinitely.
*/
static unsigned int chain_length(void const *chainp, int offset)
{
    unsigned int n = 0;
    void const *p = chainp;
    do {
        ++n;
        void const * const*load_addr = (void const * const*)((unsigned char *)p + offset);
        p = *((void const **)load_addr);
    } while (p != chainp);
    return n;
}


/*
 * Construct a random maximal cycle, returned as an array of integers,
 * caller to free.
 * We use Sattolo's algorithm (a variant on Fisher-Yates).
 */
static int *random_maximal_cycle(unsigned int n)
{
    unsigned int i;
    int *order = malloc(sizeof(int) * n);   /* This could be problematic if w.s. very large */
    assert(order != NULL);
    assert(n > 0);
    for (i = 0; i < n; ++i) {
        order[i] = i;
    }
    for (i = n-1; i >= 1; --i) {
        unsigned int j = rand() % (i);
        int temp;
        assert(j < n);
        temp = order[j];
        order[j] = order[i];
        order[i] = temp;
    }
    /* order now contains a random maximal cycle. */
    return order;
}


/*
 * WorkingSet object.
 *
 * This object captures post-facto characteristics of a working set.
 * Can be fed a trace and will update the working set properties.
 *
 * Currently this is very simplistic.
 *
 * We don't attempt to characterize how this working set would
 * interact with various cache geometries. For example,
 * a working set which consisted of accesses at n, n+4K, n+8K
 * etc. might have a lot of conflicts due to aliasing.
 *
 * Nor do we characterize how many distinct
 * granules of size N are touched, where interesting values of
 * N would relate to various caching mechanisms such as
 *   - data cache line: 64-bytes or (occasionally) 128-bytes
 *   - TLB entry for 4K page
 *   - TLB entry for huge (512K) page
 * Nor do we characterize the "page walk working set" or the
 * amount of space that would be taken up by caching one or more
 * levels of page table.
 *
 * Other things being equal, for a dense working set we'd expect
 *   - number of 4K pages to be size / 4K
 *   - one TLB entry per page assuming no compression
 * So for an 8M working set with no huge pages we'd have
 *   - 2048 pages
 *   - 2048 last-level page table entries
 *   - 16K of last-level page tables
 */
typedef struct Footprint {
    struct Footprint *next_in_bucket;
    void const *granule_address;
#define FOOTPRINT_GRANULE_BITS (8 * sizeof(unsigned long))
    unsigned long bitmap_touch1;    /* touch1 and touchm form a saturating-to-2 counter */
    unsigned long bitmap_touchm;
    unsigned int n_access;
} Footprint;

typedef struct WorkingSet {
    void const *min_address;        /* lowest address accessed */
    void const *max_access_address; /* max (base) address of access */
    void const *hwm;                /* high water mark allowing for access size */
    unsigned int n_access;          /* number of accesses */
    unsigned int n_unaligned;       /* number of unaligned accesses */
    void const *most_recent_access; /* the most recent access - to detect simple streaming */
    unsigned int n_contig_access;   /* number of accesses on same line or next line */
#define FOOTPRINT_N_BUCKETS 16384
    Footprint *cache_lines_touched[FOOTPRINT_N_BUCKETS];
    unsigned int n_cache_lines_touched;
} WorkingSetCharacteristics;

static void ws_init(WorkingSetCharacteristics *ws)
{
    memset(ws, 0, sizeof *ws);
    ws->n_access = 0;
    ws->n_unaligned = 0;
    ws->min_address = 0;
    ws->max_access_address = 0;
    ws->hwm = 0;
    ws->most_recent_access = 0;
    ws->n_contig_access = 0;
}

/*
Incrementally update working set characteristics, given another access.
Currently we don't distinguish reads and writes.
*/
static void ws_update(WorkingSetCharacteristics *ws, void const *p, size_t access_size)
{
    ws->n_access++;
    void const *pe = (char const *)p + access_size;
    unsigned int const LINE = 64;

    assert(p != 0);
    assert(access_size > 0);
    assert(access_size == (access_size & -access_size));   /* check it's a power of 2 */
    if (ws->n_access == 1) {
        /* This is the first access */
        ws->min_address = p;
        ws->max_access_address = p;
        ws->hwm = pe;
    } else {
        /* Check if this access is contiguous with the previous access. */
        unsigned long pline = (unsigned long)(ws->most_recent_access) & ~(unsigned long)(LINE-1);
        unsigned long line = (unsigned long)p & ~(unsigned long)(LINE-1);
        if (line == pline || line == pline+LINE) {
            ws->n_contig_access++;
        }
        if ((char const *)p < (char const *)ws->min_address) {
            ws->min_address = p;
        }
        if ((char const *)p > (char const *)ws->max_access_address) {
            ws->max_access_address = p;
        }
        if ((char const *)pe > (char const *)ws->hwm) {
            ws->hwm = pe;
        }
    }
    if (((unsigned long)p & (access_size-1)) != 0) {
        ws->n_unaligned += 1;
    }
    ws->most_recent_access = p;
    {
        void const *line_address = (void const *)((unsigned long)p & ~(unsigned long)(LINE-1));
        unsigned long line = (unsigned long)line_address / LINE;
        unsigned int const bytes_per_granule = (LINE * FOOTPRINT_GRANULE_BITS);
        void const *granule_address = (void const *)((unsigned long)p & ~(bytes_per_granule-1));
        unsigned long granule = (unsigned long)granule_address / bytes_per_granule;
        unsigned int const bucket = granule % FOOTPRINT_N_BUCKETS;
        Footprint *fp;
        for (fp = ws->cache_lines_touched[bucket]; fp; fp = fp->next_in_bucket) {
            if (fp->granule_address == granule_address) {
                break;
            }
        }
        if (!fp) {
            fp = (Footprint *)malloc(sizeof *fp);
            fp->granule_address = granule_address;
            fp->n_access = 0;
            fp->bitmap_touch1 = 0;
            fp->bitmap_touchm = 0;
            fp->next_in_bucket = ws->cache_lines_touched[bucket];
            ws->cache_lines_touched[bucket] = fp;
        }
        ++fp->n_access;
        {
            unsigned int offset = line & (FOOTPRINT_GRANULE_BITS-1);
            unsigned long mask = 1ULL << offset;
            if ((fp->bitmap_touch1 & mask) == 0) {
                ++ws->n_cache_lines_touched;
                fp->bitmap_touch1 |= mask;
            } else if ((fp->bitmap_touchm & mask) == 0) {
                fprintf(stderr, "repeat access to %p\n", line_address);
                fp->bitmap_touchm |= mask;
            }
        }    
    }
}


/*
 * Queries on the working set characteristics.
 */
static size_t ws_range(WorkingSetCharacteristics const *ws)
{
    return (char const *)ws->hwm - (char const *)ws->min_address;
}

static void ws_show(WorkingSetCharacteristics const *ws)
{
    printf("Working set (%u accesses):\n", ws->n_access);
    printf("  From:   %p\n", ws->min_address);
    printf("  To:     %p\n", ws->hwm);
    printf("  Range:  %#lx\n", (unsigned long)ws_range(ws));
    printf("  Contig: %u\n", ws->n_contig_access);
    printf("  Lines:  %u\n", ws->n_cache_lines_touched);
    printf("  Unalign:%u\n", ws->n_unaligned);
}

static void ws_free(WorkingSetCharacteristics *ws)
{
    unsigned int b;
    for (b = 0; b < FOOTPRINT_N_BUCKETS; ++b) {
        while (ws->cache_lines_touched[b]) {
            Footprint *fp = ws->cache_lines_touched[b];
            ws->cache_lines_touched[b] = fp->next_in_bucket;
            free(fp);
        }
    }
}


static unsigned int cache_line_length(Character const *c)
{
    static unsigned int line_size = 0;
    if (!line_size) {
        long line = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
        if (line < 0) {
            perror("sysconf(_SC_LEVEL1_DCACHE_LINESIZE)");
            line = 64;    /* Fall back to sensible guess at line size */
        } else if (line == 0) {
            fprintf(stderr, "sysconf(_SC_LEVEL1_DCACHE_LINESIZE) reports line size zero: assume 64\n");
            line = 64;
        }
        line_size = (unsigned int)line;
    }
    return line_size;
}


static unsigned int hash_uint(unsigned int n)
{
    return n * (1024+17);
}


/*
 * Return the placement of the data chain pointer within the cache line or line group.
 * This needs to be a deterministic function if i, so if we want to randomize
 * the placement we've got to use a pseudo-random function of i.
 * We mustn't overspill the end of the line group, as we risk bumping into
 * the next line group or unrelated data.
 * Exceptionally, the first item is always at offset 0, so that the client
 * knows where to start.
 */
static unsigned int line_data_placement(Character const *c, unsigned int i)
{
    unsigned int ix;
    unsigned int const LINE = cache_line_length(c);
    unsigned int const dispersion = (c->data_dispersion >= 1) ? c->data_dispersion : 1;
    unsigned int const chunk = LINE * dispersion;
    unsigned int alignment = c->data_alignment ? c->data_alignment : sizeof(void *);
    unsigned int range = (chunk - sizeof(void *)) / alignment;
    ix = (hash_uint(i) % range) * alignment;
    assert((ix + sizeof(void *)) <= chunk);
    return (i == 0) ? 0 : ix;
}


/* 
Construct a data working set, given some characteristics. The output is a contiguous
area of memory consisting of a granules (generally of cache line size) with a pointer
at the start of each granule. The pointers form a linked chain of pointers, which when
followed, will exhibit the desired working set characteristics.

Return NULL if we fail to allocate the data working set.

The input characteristics are as follows:

  - the total working set span (c->data_working_set)

  - TBD: add working set density e.g. number of cache lines touched

The first word of this working set should be the first link in a circular chain of data.

Layout is randomized in order to avoid hardware prefetching.

When the workload is run, it starts at the first link in the chain, and progressively
steps through the chain.  To fully utilize the working set, we need to process the
whole chain - so the workload must do at least that number of loads before being
reset to the beginning of the chain.  If the workload is a relatively small number of 
instructions it must either be wrapped by a loop or must remember its state from
one run to the next.
*/
void *load_construct_data(Character const *c, struct workload_mem *m)
{
    unsigned int i;
    int debug = workload_verbose;
    unsigned int const LINE = cache_line_length(c);
    unsigned int const dispersion = (c->data_dispersion >= 1) ? c->data_dispersion : 1;
    unsigned int const chunk = LINE * dispersion;
    size_t const size_rounded_to_lines = round_size(c->data_working_set*dispersion, chunk);
    unsigned int const n_lines = (size_rounded_to_lines / chunk);
    int *order;    
    void *data;
    void *adjusted_data;
    unsigned int expected_chain_length = n_lines;

    if (debug >= 1) {
        printf("Constructing data working set: size=%lu rounded=%lu lines=%u\n",
            (unsigned long)c->data_working_set,
            (unsigned long)size_rounded_to_lines, n_lines);
    }
    assert(size_rounded_to_lines >= c->data_working_set); 
    if (size_rounded_to_lines == 0) {
        /* No data working set required - presumably testing compute only */
        assert(c->data_working_set == 0);
        m->size_req = 0;
        m->size = 0;
        m->base = NULL;
        return NULL;
    }
    /*
     * Allocate the actual data area in which we construct the chain of pointers.
     * We're possibly asking for a large amount of space here (it's the data
     * working set) so we should be prepared for allocation to fail.
     */
    memset(m, 0, sizeof(struct workload_mem));
    m->size_req = size_rounded_to_lines;
    m->is_no_hugepage = (c->workload_flags & WL_MEM_NO_HUGEPAGE) != 0;
    m->is_hugepage = (c->workload_flags & WL_MEM_HUGEPAGE) != 0;
    m->is_force_hugepage = (c->workload_flags & WL_MEM_FORCE_HUGEPAGE) != 0;
    data = load_alloc_mem(m);
    if (!data) {
        fprintf(stderr, "loadgen: couldn't allocate %llu bytes for data working set\n",
            (unsigned long long)size_rounded_to_lines);
        return NULL;
    }
    /*
     * Any placement of links within lines relies on the area being at least line-aligned.
     * Check here just to make sure.
     */
    assert(((unsigned long)data % LINE) == 0);
    adjusted_data = (void *)((unsigned char *)data - c->data_pointer_offset);
    if (!(c->workload_flags & WL_MEM_STREAM)) {
        /* Construct a random cycle. */
        order = random_maximal_cycle(n_lines);
        if (debug >= 3) {
            unsigned int i;
            for (i = 0; i < n_lines; ++i) {
                printf(" %d", order[i]);
            }
            printf("\n");
        }
        /* Use the random cycle to build a chain of pointers in the data area. */
        /* Each link in the chain can, in principle, be allocated anywhere in the line,
           or if we're using dispersion, in the group of lines. We can also try to
           use unaligned and cross-line data placement. */
        for (i = 0; i < n_lines; ++i) {
            assert(order[i] < n_lines);
            *(void **)((unsigned char *)data + i*chunk + line_data_placement(c, i)) =
                ((unsigned char *)adjusted_data + order[i]*chunk + line_data_placement(c, order[i]));
        }
        free(order);
    } else {
        /* Construct a sequential cycle. */
        for (i = 0; i < n_lines; ++i) {
            *(void **)((unsigned char *)data + i*chunk) = ((unsigned char *)adjusted_data + ((i+1)%n_lines)*chunk);
        }
    }
    if (debug >= 2) {
        printf("Data working set:\n");
        unsigned int lines_to_show = n_lines;
        if (lines_to_show > 10) {
            lines_to_show = 10;
        }
        for (i = 0; i < lines_to_show; ++i) {
            unsigned int j;
            void **p;
            unsigned int ix = (c->workload_flags & WL_MEM_STREAM) ? 0 : line_data_placement(c, i);
            p = (void **)((unsigned char *)adjusted_data + i*chunk + ix);
            printf("  from %2u: ", i);
            for (j = 0; j < 10; ++j) {
                printf("*(%p+%d) -> ", p, c->data_pointer_offset);
                p = (void **)*(void **)((unsigned char *)p + c->data_pointer_offset);
            }
            printf("...\n");
        }
    }
    if (debug >= 1) {
        /* Do a post-facto check on the data, to check it has the right parameters */
        void *p = adjusted_data;
        WorkingSetCharacteristics ws;
        printf("Collecting data working set characteristics...\n");
        ws_init(&ws);
        do {
            void **load_addr = (void **)((unsigned char *)p + c->data_pointer_offset);
            ws_update(&ws, load_addr, sizeof(void *));
            p = *load_addr;
            if (ws.n_access > expected_chain_length) {
                fprintf(stderr, "working set corrupt\n");
                assert(0);
            }
        } while (p != adjusted_data);
        assert(round_size(ws_range(&ws), chunk) == size_rounded_to_lines);
        if (debug >= 1) {
            ws_show(&ws);
        }
        ws_free(&ws);
    }
    if (1) {
        unsigned int cl = chain_length(adjusted_data, c->data_pointer_offset);
        assert(cl == expected_chain_length);
        if (debug >= 1) {
            printf("Data chain length verified as %lu (%lu-byte footprint in %u-byte lines)\n",
                (unsigned long)cl, ((unsigned long)cl * LINE), LINE);
        }
    }
    if (debug >= 1) {
        printf("Constructed data working set.\n");
    }
    /* Return the initial offsetted pointer that the client should use when
       iterating through the working set. Each load will add the fixed offset to
       the current pointer. The actual memory area is remembered in the
       memory descriptor structure and will be used on free. */
    return adjusted_data;
}

