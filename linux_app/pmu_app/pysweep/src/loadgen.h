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
Synthetic workload generator / executor.

This module constructs and executes synthetic workloads based on a set of
generic input parameters (characteristics) describing aspects such as

   - code working set
   - data working set
   - branch predictability
   - use of various instruction groups

The resulting workload, when run, should consume CPU and system resources
corresponding to its characteristics.
*/


#ifndef __included_loadgen_h
#define __included_loadgen_h

#include "genelf.h"

#include <stdint.h>

/*
Workload characteristics structure.

This is filled in by the client, to specify the workload.
*/
typedef struct {
    /* Data working set in bytes. Currently assumed to be flat
       (i.e. randomly distributed, not clumped). */
    unsigned long data_working_set;
    /* The data pointer offset can be set non-zero to try and defeat
       linked-list prefetchers. The offset will be applied to pointers
       stored in the data working set, and adjusted for in the loads. */
    unsigned int data_pointer_offset;
    /* How sparse is the data? This multiplier is applied to the
       addresses within the data. (A default of 0 has the effect of 1.) */
    unsigned int data_dispersion;
    /* Alignment of pointers in the data working set - e.g. 1 for
       byte alignment. Set to 0 for natural alignment. */
    unsigned int data_alignment;
    /* Instruction working set in bytes. */
    unsigned long inst_working_set;
    unsigned int inst_mispredict_rate;
#define WL_MEM_BW           0x01    /* Measure for memory bandwidth not latency */
#define WL_MEM_NONTEMPORAL  0x02    /* Use non-temporal loads where possible */
#define WL_MEM_LOAD_EXTRA   0x04    /* Generate an additional (unused) load */
#define WL_MEM_LOAD_PAIR    0x08    /* Use a non-temporal load */
#define WL_MEM_PREFETCH     0x10    /* Issue a software prefetch before loading */
#define WL_MEM_STREAM       0x20    /* Use stream rather than random workload */
#define WL_MEM_NO_HUGEPAGE  0x40    /* Request no huge-pages */
#define WL_MEM_HUGEPAGE     0x80    /* Request huge-pages */
#define WL_MEM_FORCE_HUGEPAGE  0x100    /* Request huge-pages even when small */
#define WL_MEM_BARRIER     0x200    /* Generate a load barrier after each load */
#define WL_MEM_ACQUIRE     0x400    /* Use load-acquire instruction */
#define WL_MEM_NOP         0x800    /* Generate a NOP after each load */
#define WL_MEM_STORE      0x1000    /* Generate stores as well as loads */
#define WL_MEM_RELEASE    0x2000    /* Use store-release instruction */
#define WL_MEM_ATOMIC     0x4000    /* Use atomic instruction */
#define WL_DEPEND         0x8000    /* Force total dependency chain */
#define WL_MEM_BARRIER_SYSTEM 0x10000   /* e.g. DMB SY */
#define WL_MEM_BARRIER_SYNC   0x20000   /* serializing wrt instructions: DSB instead of DMB */
    unsigned int workload_flags;   /* WL_xxx flags */
    /* Floating-point intensity - FP ops per memory reference. */
    unsigned int fp_intensity;
    /* Arithmetic precision */
#define FP_PRECISION_FP16    1 
#define FP_PRECISION_SINGLE  2
#define FP_PRECISION_DOUBLE  3
    unsigned int fp_precision;   /* 1: FP16, 2: SP, 3: DP */
    unsigned int fp_simd;        /* 0: scalar, N: N-way SIMD */
#define FP_OP_MOV 0
#define FP_OP_IADD 1
#define FP_OP_IXOR 2
#define FP_OP_NEG 3
#define FP_OP_ADD 4
#define FP_OP_MUL 5
#define FP_OP_DIV 6
#define FP_OP_SQRT 7
#define FP_OP_FMA 8     /* up to here, we must match arrays in loadinst.c */
#define FP_OP_FMAA 9
#define FP_OP_MULADD 10
#define FP_OP_DOT2 11
#define FP_OP_DOT4 12
#define FP_OP_DIST2 13
    unsigned int fp_operation;
    unsigned int fp_concurrency; /* Number of concurrent ops: 1 = back-to-back */
    double fp_value;             /* Floating-point data value initializer */
    double fp_value2;            /* Floating-point data value corrector */

    /* Floating-point flags can modify both the target configuration and
       our generated code. */
#define FP_FLAG_DENORMAL_GEN   0x01   /* Code generation: use denormal inputs */
#define FP_FLAG_DENORMAL_FTZI  0x02   /* Target mode: flush to zero on input */
#define FP_FLAG_DENORMAL_FTZO  0x04   /* Target mode: flush to zero on output */
#define FP_FLAG_DENORMAL_FTZ   (FP_FLAG_DENORMAL_FTZI|FP_FLAG_DENORMAL_FTZO)
#define FP_FLAG_ALTERNATE      0x08   /* Use alternate instruction form (whatever it is) */
#define FP_FLAG_SIMPLE_VAL     0x10   /* Use simple (possibly fast) value */
#define FP_FLAG_CONVERGE       0x20   /* For DIV, converge to result of 1.0 */
#define FP_FLAG_LOAD_CONST     0x40   /* Load constants from memory */
    unsigned int fp_flags;

    /* Debugging/diagnostic flags for workload generation. */
#define WORKLOAD_DEBUG_NO_MPROTECT     1   /* don't do mprotect() even if we need to */
#define WORKLOAD_DEBUG_NO_UNIFICATION  2   /* don't do cache unification even if we need to */
#define WORKLOAD_DEBUG_DUMMY_CODE      4   /* never generate code */
#define WORKLOAD_DEBUG_NO_WX           8   /* avoid write+execute */
#define WORKLOAD_DEBUG_NO_FREE      0x10   /* don't free any memory - in case race */
#define WORKLOAD_DEBUG_TRIAL_RUN    0x20   /* check workload runs, immediately after construction */
    unsigned int debug_flags;
    unsigned long inst_target;        /* Target no. of insts for one execution of workload */
} Character;


void workload_init(Character *);


/*
What the entry point for a workload looks like.
There may also be implicit floating-point arguments (TBD improve).
*/
typedef void *(* dummy_fn_t)(void *, void *, void *);


/*
When generating the workload code, we keep track of how
many instructions of these different categories we expect
to execute. This can then be calibrated against observed
performance events.
*/
typedef enum InstCounter {
    /* The following correspond to hardware counters. */
    COUNT_INST,          /* Total instructions */
    COUNT_BRANCH,        /* Any kind of branch or transfer of control */
    COUNT_FLOP_HALF,     /* Half-precision floating-point operations */
    COUNT_FLOP_SP,       /* SP floating-point operations: FMA counts 2, SIMD counts N */
    COUNT_FLOP_DP,       /* DP floating-point operations: FMA counts 2, SIMD counts N */
    COUNT_MOVE,          /* Register moves (any kind) */
    COUNT_INST_RD,       /* Memory read instructions */
    COUNT_BYTES_RD,      /* Memory read bytes */
    COUNT_INST_WR,       /* Memory write instructions */
    COUNT_BYTES_WR,      /* Memory write bytes */
    COUNT_FENCE,         /* Fences/barriers */
#define COUNT_MEM_PREFETCH COUNT_INST   /* Don't count prefetches as reads */
    /* The following are more arbitrary measures, when we are generating
       sequences of instructions (e.g. dot-product). */
    COUNT_UNIT,
    /* Number of counter types, for sizing counter arrays */
    COUNT_MAX
} inst_counter_t;


struct inst_counters {
    unsigned int n[COUNT_MAX];
};


/* Properties of an allocated memory block - useful for when we want to free */
struct workload_mem {
    /* Input parameters */
    unsigned long size_req;  /* Size actually wanted */
    int is_exec:1;           /* Allocate as executable */
    int is_no_hugepage:1;    /* Forbid allocation as huge pages */
    int is_hugepage:1;       /* Request opportunistic promotion to huge pages if large enough */
    int is_force_hugepage:1; /* Request promotion to huge pages even for small allocations */
    /* Output */
    void *base;              /* Base virtual address */
    unsigned long size;      /* Size obtained - maybe rounded up to pages etc. */
    int is_mmap:1;           /* Obtained by mmap (not malloc) */
};

/*
Details of a workload created to implement the workload characteristics
requested by a client.
*/
typedef struct {
    /* Data passed in by client */
    Character c;         /* Copy of workload characteristics as specified by client */

    /* Data about the generated workload code */
    struct inst_counters expected;  /* Count values per entry call */
    unsigned int n_chain_steps;  /* number of data steps per iteration */
    elf_t elf_image;     /* Internal descriptor for ELF generation */

    /* Data required to run the workload */
    dummy_fn_t entry;    /* Code entry point */
    void *entry_args[2]; /* Arguments for entry point */

    /* Following are internal details - shouldn't really be exposed here */
    struct workload_mem code_mem;
    struct workload_mem data_mem;

    /* Current status of the workload */
    volatile unsigned int references;   /* Number of threads running this workload */

    /* Anything else needed by the workload */
    uint64_t scratch[16]; /* Scratch space for spills etc. */
} Workload;


/*
 * Builds a workload with the given characteristics.
 * Returns a workload object.
 */
Workload *workload_create(Character const *);

/*
 * Increment the reference count on a workload.
 */
void workload_add_reference(Workload *);

/*
 * Decrement the reference count and possibly destroy the workload.
 */
void workload_remove_reference(Workload *);

/*
 * Destroy a workload object.
 * Return 1 if the workload was actually freed, 0 if deferred.
 */
int workload_free(Workload *);

/*
 * Run the first iteration of a workload in the current thread, and then stop.
 * Multiple threads can concurrently run the same workload.
 */
void workload_run_once(Workload *);

/*
 * Run N iterations of a workload in the current thread.
 */
void *workload_run(Workload *, void *, unsigned int);

/*
 * Dump workload to an ELF file.
 */
int workload_dump(Workload *, char const *fn, unsigned int flags);

#endif /* included */

