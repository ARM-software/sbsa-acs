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
 * Generate code for a runnable workload in memory, based on a set of workload characteristics.
 *
 * Much of this unit is about generating instructions for x86 and ARM.
 * A future direction would be for this function to be performed via some
 * existing tool such as LLVM or DynamoRIO.
 */

#include "loadgenp.h"

#include "loadinst.h"
#include "prepcode.h"
#include "arch.h"
#include "genelf.h"

#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


/*
Make a data buffer pointer into a function pointer.
This is trivial except in ARM32 where we have to create an interworking pointer
(LSB set to indicate Thumb).  We assume - locally to this program - that the
data buffer has been created with code in the same mode as we're currently executing.
*/
static dummy_fn_t make_fn(void const *p)
{
#ifdef __thumb__
    return (dummy_fn_t)((unsigned char const *)p + 1);
#else
    return (dummy_fn_t)p;
#endif
}


void fprint_mem(FILE *fd, void const *pv, size_t size)
{
    unsigned char const *p = (unsigned char const *)pv;
    unsigned int i;
    for (i = 0; i < size; ++i) {
        if ((i % 32) == 0) {
           fprintf(fd, "  %p ", (p + i));
        }
        fprintf(fd, " %02x", p[i]);
        if (((i+1) % 32) == 0 || (i+1) == size) {
            fprintf(fd, "\n");
        }
    }
}


void fprint_code(FILE *fd, void const *pv, size_t size)
{
    fprint_mem(fd, pv, size);
}


static void character_print(Character const *c)
{
    static char const *const fp_prec_names[] = {"?", "half", "single", "double"};
    static char const *const fp_op_names[] = {
        "mov",
        "iadd", "ixor",
        "fneg", "fadd", "fmul", "fdiv", "fsqrt",
        "fma", "fma(acc)", "dot2", "dot3", "dot4",
    };
    printf("  page size:        %lu\n", (unsigned long)sysconf(_SC_PAGESIZE));
    printf("  inst working set: %lu\n", (unsigned long)c->inst_working_set);
    printf("  data working set: %lu\n", (unsigned long)c->data_working_set);
    if (c->data_pointer_offset != 0) {
        printf("    data pointer offset: %d\n", c->data_pointer_offset);
    }
    if (c->data_dispersion > 1) {
        printf("    data dispersion:     %d\n", c->data_dispersion);
    }
    if (c->data_alignment != 0) {
        printf("    data alignment:      %d\n", c->data_alignment);
    }
    printf("  flags:            %#x\n", (unsigned int)c->workload_flags);
    printf("  FP intensity:     %lu\n", (unsigned long)c->fp_intensity);
    if (c->fp_intensity > 0) {
        printf("    Precision:      %s\n", fp_prec_names[c->fp_precision]);
        printf("    Operation:      %s\n", fp_op_names[c->fp_operation]);
        printf("    Concurrency:    %u\n", (unsigned int)c->fp_concurrency);
        printf("    SIMD:           %u-way\n", (unsigned int)c->fp_simd);
        printf("    Flags:          %#x\n", (unsigned int)c->fp_flags);
    }
    if (c->debug_flags != 0) {
        printf("  debug flags:      %#x\n", (unsigned int)c->debug_flags);
    }
}


static flavor_t character_flavor(Character const *c)
{
    flavor_t flavor = c->fp_precision == FP_PRECISION_DOUBLE ? F64 :
                      c->fp_precision == FP_PRECISION_SINGLE ? F32 :
                      c->fp_precision == FP_PRECISION_FP16 ? F16 : 0;
    return flavor;
}


static unsigned int load_prepcode_flags(Character const *c)
{
    unsigned int pflags = PREPCODE_ALL;
    int allow_write_and_exec = !(c->debug_flags & WORKLOAD_DEBUG_NO_WX);
    if (c->debug_flags & WORKLOAD_DEBUG_NO_UNIFICATION) {
        pflags &= ~PREPCODE_COHERENCE;
    }
    if (allow_write_and_exec ||
        (c->debug_flags & WORKLOAD_DEBUG_NO_MPROTECT)) {
        pflags &= ~PREPCODE_PROTECT;
    }
    return pflags;
}


static int gen_fp_move(CS *cs, Character const *c, freg_t Rd, freg_t Rn)
{
    int ok;
    flavor_t const flavor = character_flavor(c);
    if (!(c->fp_flags & FP_FLAG_LOAD_CONST)) {
        if (Rd == Rn) {
            ok = 1;
        } else {
            ok = codestream_gen_op(cs, FP_OP_MOV, flavor, Rd, Rn, NR, NR);
        }
    } else {
        /* On some cores, internal result caches have separate slots
           for the results of loads, and it can be advantageous
           for loop-invariant data to be in these slots. */
        /* FSTR <constval_source>,[Rscratch,#0] */
        ok = codestream_gen_fp_store(cs, flavor, Rn, 2, 0, 0);
        /* FLDR <reg_first_const+i>,[Rscratch,#0] */
        ok = codestream_gen_fp_load(cs, flavor, Rd, 2, 0, 0);
    }
    return ok;
}


/*
Construct some code, representing a workload with given code
characteristics, and traversing the data structure we've constructed.
*/
void *load_construct_code(Workload *w)
{
    Character const *c = &w->c;
    struct workload_mem *m = &w->code_mem;
    void *code_entry;
    CS *cs;
    unsigned int LINE = 64;
    size_t const size_rounded_to_lines = round_size(c->inst_working_set, LINE);
    size_t const size = size_rounded_to_lines;
    dummy_fn_t fp;
    void *code_area;

    /* If allow_write_and_exec is set, we allocate code memory with
       both PROT_WRITE and PROT_EXEC set. We can then build the code
       and execute it immediately (Intel) or after a userspace cache
       unification (ARM). */
    int allow_write_and_exec = !(c->debug_flags & WORKLOAD_DEBUG_NO_WX);

    void **dummy_data;
    dummy_data = (void **)&dummy_data;    /* Dummy circular chain */

    assert(size_rounded_to_lines > 0);
    assert(size_rounded_to_lines % LINE == 0);

    if (workload_verbose) {
        printf("loadgen: %p: creating workload code\n", w);
        character_print(c);
    }

    /* Allocate a page-aligned block of memory to be used for the generated code. */
    m->size_req = size_rounded_to_lines;
    if (allow_write_and_exec) {
        m->is_exec = 1;
    }
    code_area = load_alloc_mem(m);
    if (!code_area) {
        return NULL;
    }
    if (workload_verbose) {
        printf("  constructing branch code at %p, size 0x%lx\n",
            code_area, (unsigned long)size);
    }
    assert(code_area == m->base);
    assert(m->size >= size);
    elf_add_code(w->elf_image, m->base, m->size);
#if defined(ARCH_A32) || defined(ARCH_T32) || defined(ARCH_A64)
    /* Add a mapping symbol for the whole code area */
    {        
#ifdef ARCH_A64
        char const *ms = "$x";
#elif defined(ARCH_A32)
        char const *ms = "$a";
#else
        char const *ms = "$t";
#endif
        elf_add_symbol(w->elf_image, ms, code_area, 0);
    }
#endif

    /* For the roofline model, we want to be able to generate workloads
       with a given "arithmetic intensity". The intensity is a number N 
       where the ratio of bytes to ops is 1:N. There seems to be an
       implicit assumption that data is dense, so when we bring in a
       cache line, we count the entire line as used for the purposes of
       calculating "arithmetic intensity". */
    unsigned int const fpop_per_mem = c->fp_intensity;
    int const any_data = (c->data_working_set > 0);

    cs = codestream_init(&w->expected, code_area, size, LINE);
    if (c->fp_flags & FP_FLAG_ALTERNATE) {
        codestream_use_alternate(cs);
    }
    if (workload_verbose) {
        codestream_show(cs);
    }

    /* Construct code until we fill up the code working set. */
    code_entry = codestream_addr(cs);
   
    flavor_t flavor = character_flavor(c);
    unsigned int const ewidth = FLOAT_BITS(flavor) / 8;
    if (c->fp_simd*ewidth == 8) { 
        flavor |= S64;
    } else if (c->fp_simd*ewidth == 16) {
        flavor |= S128;
    } else if (c->fp_simd*ewidth == 32) {
        flavor |= S256;
    } else if (c->fp_simd*ewidth == 64) {
        flavor |= S512;
    } else if (c->fp_simd*ewidth == 128) {
        flavor |= S1024;
    } else if (c->fp_simd > 1) {
        /* Invalid number of lanes */
        load_free_mem(m);
        return NULL;
    }

    /* Set up the register pool */

/* How many floating-point registers are we allowed to clobber? */
#if defined(ARCH_T32) || defined(ARCH_A32)
#define FP_REGS_AVAIL 16 
#elif defined(ARCH_A64)
#define FP_REGS_AVAIL 32 
#elif defined(__x86_64__)
/* According to the System V ABI for x86_64, a callee can use %xmm0 to %xmm15 */
#define FP_REGS_AVAIL 16 
#else
#error Unknown architecture
#endif
    /* Current state, for generating the instruction mix */
    int op_needs_2_regs = (c->fp_operation >= FP_OP_DOT2);
    unsigned int op_regs_used = (op_needs_2_regs ? 2 : 1);
    unsigned int fp_regs_cycle = c->fp_concurrency * op_regs_used;

    unsigned int fp_regs_const = 1;
    if (fp_regs_cycle > (FP_REGS_AVAIL - fp_regs_const)) {
        fp_regs_cycle = FP_REGS_AVAIL - fp_regs_const;
    } else if (fp_regs_cycle == 0) {
        fp_regs_cycle = 1;
    }
    unsigned int const reg_first_const = (fp_regs_const > 0) ? fp_regs_cycle : NR;
    assert(fp_regs_cycle > 0);

    if (workload_verbose) {
        printf("  total regs available: %u\n", FP_REGS_AVAIL);
        printf("  regs in recirculation cycle: %u\n", fp_regs_cycle);
        printf("  constant regs: %u\n", fp_regs_const);
    }
 
    /* First copy from FP register 0 into the other registers to ensure
       none of them contain a NaN, denormal etc.  Assume we've got
       enough space for this. Assume the workload runner has set up
       FP register 0 with a suitable value, e.g. 1.0.
       On ARM we could use FCONST instruction to construct suitable
       values directly here, but that's harder on x86.
       If we request an invalid SIMD size, we'll fail here.
    */
    if (fpop_per_mem > 0) {
        unsigned int i;
        int ok;
        unsigned int constval_source = 2;    /* i.e. D2, or S2 */
        unsigned int workval_source = 1;     /* i.e. D1, or S1 */
        /* Save the const value from R2 before we clobber it. */        
        if (reg_first_const != NR && reg_first_const > constval_source) {
            for (i = 0; i < fp_regs_const; ++i) {
                ok = gen_fp_move(cs, c, reg_first_const+i, constval_source);
                if (!ok) {
                    break;
                }
            }
        }
        for (i = 0; i < fp_regs_cycle; ++i) {
            if (i != workval_source) {
                ok = codestream_gen_op(cs, FP_OP_MOV, flavor, i, workval_source, NR, NR);
                if (!ok) {
                    break;
                }
            }
            if (!codestream_reserve(cs, 8)) {
                assert(0);
            }
        }
        if (reg_first_const != NR && reg_first_const <= constval_source) {
            gen_fp_move(cs, c, reg_first_const, constval_source);
        }
        /* We now have all work (recirculation) regs with the initial work
           value from R1, and the FIRST_CONST reg with the value from R2. */
        if (codestream_errors(cs) > 0) {
            goto generation_failed;
        }
    }

    /* For small instruction-working-set workloads, we create an inner
       loop round the workload to reduce the effect of overhead.
       Essentially we aim to execute some reasonably large number
       (say 100000 or 50000) 4-byte instructions per outer call
       Roughly estimate number of instructions (size/4) and then iterate
       that for as many times as we need to hit the target. We don't
       need to be super accurate, as the actual instruction counts will
       be calculated from the outer iteration count (which is accurate
       apart from a possible iteration in progress) and the instruction
       counts we record when generating the code.
     */
    unsigned int n_iters = (c->inst_target / (size / 4));
    if (n_iters == 0) {
        n_iters = 1;
    }
    void *work_kernel = NULL;
#define IRBASE    IR0      /* Memory chain pointer */
#define IROFFSET  IR1      /* Offset for chain pointer */
#define IRSCRATCH IR2
#define IRLOOP    IR3      /* Top-level loop count */
    assert(n_iters >= 1);
    if (n_iters > 1) {
        /* Set up a fixed-count loop within the workload. */
        codestream_gen_movi32(cs, IRLOOP, n_iters);
        work_kernel = codestream_addr(cs);
        /* Instructions generated within the loop add an appropriately scaled
           count to the overall workload instruction metrics. */
        codestream_push_multiplier(cs, n_iters);
    }
    elf_add_symbol(w->elf_image, "kernel", codestream_addr(cs), 0);

    unsigned int fp_reg = 0;     /* Cycle through available FP regs */

    w->n_chain_steps = 0;

    /* Fill the code buffer with instructions implementing the requested code mix.
       This is a sequence of operations cycling through the available FP registers.
       We might bail out mid-way through an operation. */
    unsigned int load_flags = 0;
    if (c->workload_flags & WL_MEM_NONTEMPORAL) {
        load_flags |= CS_LOAD_NONTEMPORAL;
    }
    if (c->workload_flags & WL_MEM_ACQUIRE) {
        load_flags |= CS_LOAD_ACQUIRE;
    }
    if (c->workload_flags & WL_MEM_ATOMIC) {
        load_flags |= CS_LOAD_ATOMIC;
    }
    if (c->workload_flags & WL_MEM_LOAD_PAIR) {
        load_flags |= CS_LOAD_PAIR;
    }
#ifdef ARCH_A64
    if (c->fp_flags & FP_FLAG_ALTERNATE) {
        codestream_gen_direct(cs, 0x2520e020);    /* pseudo SVE instruction to ask ArmIE to start trace */
    }
#endif
    while (codestream_reserve(cs, 12)) {
        unsigned int j;
        if (any_data) {
            /* Generate a load to follow the chain in the data working set.
               This will count as a load instruction in our general code metrics
               accumulator, but we also count it specifically as a chain step. */
            w->n_chain_steps += 1;
            if (c->workload_flags & WL_MEM_PREFETCH) {
                codestream_gen_load(cs, NR, IRBASE, NR, 0, CS_LOAD_PREFETCH);
            }
            if (c->data_pointer_offset != 0) {
                /* Load from the current data-pointer (R0) indexed by the constant offset register (R1) */
                codestream_gen_load(cs, IRBASE, IRBASE, IROFFSET, 0, load_flags);
            } else {
                if (c->workload_flags & WL_MEM_LOAD_EXTRA) {
                    codestream_gen_load(cs, IRSCRATCH, IRBASE, NR, 8, load_flags);    /* TBD do this better */
                }
                codestream_gen_load(cs, IRBASE, IRBASE, NR, 0, load_flags);
            }            
            if (c->workload_flags & WL_MEM_STORE) {
                if (!codestream_reserve(cs, 12)) {
                    break;
                }
                unsigned int store_flags = CS_STORE_DEFAULT;
                if (c->workload_flags & WL_MEM_RELEASE) {
                    store_flags |= CS_STORE_RELEASE;
                }
#ifdef ARCH_A64
                if (c->workload_flags & WL_MEM_RELEASE) {
                    /* ADD <scratch>,<IR0>,#8 */
                    codestream_gen_iopk(cs, CS_IOP_ADD, IRSCRATCH, IRBASE, 8);
                    /* STLR <IR1>,[<scratch>,#0]  - IR1 used as an available value to store */
                    codestream_gen_store(cs, IR1, IRSCRATCH, NR, 0, store_flags);
                } else
                /* ** Careful: next line is the 'else' clause */
#endif
                codestream_gen_store(cs, IR1, IRBASE, NR, 8, store_flags);
            }
            if (c->workload_flags & WL_MEM_BARRIER) {
                unsigned int fence_flags = (c->workload_flags & WL_MEM_STORE) ? CS_FENCE_STORE : CS_FENCE_LOAD; 
                if (c->workload_flags & WL_MEM_BARRIER_SYSTEM) fence_flags |= CS_FENCE_SYSTEM;
                if (c->workload_flags & WL_MEM_BARRIER_SYNC) fence_flags |= CS_FENCE_SYNC;
                codestream_gen_fence(cs, fence_flags);
            }
            if (c->workload_flags & WL_MEM_NOP) {
                codestream_gen_nop(cs);
            }
        } else if (fpop_per_mem == 0) {
            /* No data or FP accesses - just generate NOPs to force a code working set */
            codestream_gen_nop(cs);
        }
        for (j = 0; (j < fpop_per_mem) && codestream_reserve(cs, 8); ++j) {
            unsigned int k;
            freg_t R1 = fp_reg;
            freg_t R2 = (op_regs_used == 2) ? ((fp_reg + 1) % fp_regs_cycle) : NR;
            if (c->workload_flags & WL_MEM_NOP) {
                codestream_gen_nop(cs);
            }
            switch (c->fp_operation) {
            case FP_OP_MOV:
            case FP_OP_NEG:
                codestream_gen_op(cs, c->fp_operation, flavor, R1, R1, NR, NR);
                break;            
            case FP_OP_SQRT:
                /* x := sqrt(x) converges to 1.0 very rapidly, and might then
                   take a fast path. So we insert an ADD into the path to
                   restore the value to something harder. */
                if (!(c->fp_flags & FP_FLAG_CONVERGE)) {
                    codestream_gen_op(cs, FP_OP_ADD, flavor, R1, R1, reg_first_const, NR);
                    if (!codestream_reserve(cs, 8)) {
                        break;
                    }
                }
                codestream_gen_op(cs, c->fp_operation, flavor, R1, R1, NR, NR);
                break;
            case FP_OP_ADD:
            case FP_OP_MUL:           
            case FP_OP_DIV:
            case FP_OP_IADD:
            case FP_OP_IXOR:
                if (c->fp_flags & FP_FLAG_CONVERGE) {
                    /* x := x/x converges immediately to 1.0, which may then
                       take a fast path. */
                    codestream_gen_op(cs, c->fp_operation, flavor, R1, R1, R1, NR);
                } else {
                    /* With a constant numerator, the result of the divide (and denominator
                       for the next divide) will oscillate between two values.
                       We also hope the numerator and denominator will be different.
                       Data-dependency chain is through the denominator. */
                    codestream_gen_op(cs, c->fp_operation, flavor, R1, reg_first_const, R1, NR);
                }
                break;
            case FP_OP_MULADD:
                codestream_gen_op(cs, FP_OP_MUL, flavor, R1, R1, R1, NR);
                if (!codestream_reserve(cs, 8)) goto end_of_loop;
                codestream_gen_op(cs, FP_OP_ADD, flavor, R1, R1, R1, NR);
                break;
            case FP_OP_FMA:
                codestream_gen_op(cs, c->fp_operation, flavor, R1, R1, R1, R1);
                break;
            case FP_OP_FMAA:
                /* Accumulator latency (FMAA). Writes back to its accumulator operand,
                   but the multiplier operands are not written. */ 
                codestream_gen_op(cs, FP_OP_FMA, flavor, R1, reg_first_const, reg_first_const, R1);
                break;
            case FP_OP_DOT2:
                /* 2-term dot product (DOT2) */
                codestream_gen_op(cs, FP_OP_MUL, flavor, R1, R1, R1, NR);
                if (!codestream_reserve(cs, 8)) goto end_of_loop;
                codestream_gen_op(cs, FP_OP_FMA, flavor, R1, R2, R2, R1);
                break;
            case FP_OP_DOT4:
                /* 4-term dot product */
                codestream_gen_op(cs, FP_OP_MUL, flavor, R1, R1, R1, NR);
                if (!codestream_reserve(cs, 8)) goto end_of_loop;
                codestream_gen_op(cs, FP_OP_FMA, flavor, R1, R2, R2, R1);
                if (!codestream_reserve(cs, 8)) goto end_of_loop;
                codestream_gen_op(cs, FP_OP_FMA, flavor, R1, R2, R2, R1);
                if (!codestream_reserve(cs, 8)) goto end_of_loop;
                codestream_gen_op(cs, FP_OP_FMA, flavor, R1, R2, R2, R1);
                break;
            case FP_OP_DIST2:
                /* 2-term distance */
                codestream_gen_op(cs, FP_OP_MUL, flavor, R1, R1, R1, NR);
                if (!codestream_reserve(cs, 8)) goto end_of_loop;
                codestream_gen_op(cs, FP_OP_FMA, flavor, R1, R2, R2, R1);
                if (!codestream_reserve(cs, 8)) goto end_of_loop;
                codestream_gen_op(cs, FP_OP_SQRT, flavor, R1, R1, NR, NR);
                break;
            default:
                assert(0);
            }
            if (c->workload_flags & WL_DEPEND) {
               /* The destination register (R1) is also the first input register,
                  so there is already a loop-carried dependency on R1. If there are
                  other iput registers, copy R1 to them to make them all
                  loop-carried dependencies. */
                for (k = 1; k < op_regs_used; ++k) {
                    if (!codestream_reserve(cs, 4)) {
                        break;
                    }
                    codestream_gen_op(cs, FP_OP_MOV, flavor, ((fp_reg + k) % fp_regs_cycle), R1, NR, NR);
                }
            }
            w->expected.n[COUNT_UNIT] += n_iters;
            /* Set up for the next iteration */
            fp_reg = (fp_reg + op_regs_used) % fp_regs_cycle;
        }        
    }
end_of_loop:;

    if (n_iters > 1) {
        codestream_gen_decs(cs, IRLOOP);
        codestream_gen_branch(cs, work_kernel, CC_NE);
        codestream_pop_multiplier(cs, n_iters);
    }

#ifdef ARCH_A64
    if (c->fp_flags & FP_FLAG_ALTERNATE) {
        codestream_gen_direct(cs, 0x2520e040);    /* pseudo SVE instruction to ask ArmIE to stop trace */
    }
#endif

    /* Add the return in the last instruction block */
    codestream_gen_ret_abi(cs);

    if (codestream_errors(cs) > 0) {
generation_failed:
        if (workload_verbose) {
            printf("  workload generation failed\n");
        }
        codestream_free(cs);
        load_free_mem(m);
        return NULL;
    }

    codestream_free(cs);

    if (workload_verbose) {
        fprint_code(stderr, code_area, 200);
    }
    {
        w->entry = make_fn(code_entry);
        elf_set_entry(w->elf_image, w->entry);
        elf_add_symbol(w->elf_image, "payload", w->entry, 0);
    }
    /* Prepare the code for execution. This might include:
     *   - changing the memory protection if not already done
     *   - cache unification (e.g. on ARM)
     *   - calling any debugger hooks for JIT
     */
    {
        int rc;
        rc = prepare_code_elf(m->base, m->size, load_prepcode_flags(c),
                              elf_image(w->elf_image), elf_image_size(w->elf_image));
        if (rc) {
            /* Generated code, but failed to mark it executable */
            load_free_mem(m);
            return NULL;
        }
    }
    if (c->debug_flags & WORKLOAD_DEBUG_TRIAL_RUN) {
        /* test it - probably segfault if not right */
        printf("Testing generated branches at %p... 1 of 2\n", code_area);
        fprint_mem(stdout, code_area, 32);
        fp = make_fn(code_area);
        printf("  function pointer: %p\n", fp);
        fp(dummy_data, w->entry_args[1], w->scratch);
        printf("  returned ok\n");
        printf("Testing generated branches... 2 of 2\n");
        fp = make_fn(code_area + LINE*2);
        printf("  function pointer: %p\n", fp);
        fp(dummy_data, w->entry_args[1], w->scratch);
        printf("  branches ok\n");
    }
    return code_area;
}


void load_free_code(Workload *w)
{
    (void)unprepare_code(w->code_mem.base, w->code_mem.size, load_prepcode_flags(&w->c));
    if (w->elf_image) {
        elf_destroy(w->elf_image);
        w->elf_image = NULL;
    }
    load_free_mem(&w->code_mem);
}

