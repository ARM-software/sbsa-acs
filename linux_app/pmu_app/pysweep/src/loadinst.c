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

#include "loadinst.h"

#include "loadgenp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>


/*
Define a type corresponding to a code address.

Note that for ARM/Thumb we assume that we're generating code according to the
instruction set with which this translation unit is compiled.
*/
#ifdef ARCH_T32_
typedef unsigned short code_t;
#elif defined(ARCH_A32)
typedef unsigned int code_t;
#elif defined(ARCH_A64)
typedef unsigned int code_t;
#elif defined(__x86_64__)
typedef unsigned char code_t;
#else
#error "Unexpected architecture"
#endif


/*
 * This structure maintains a pointer into the instruction stream
 * we're building, and allows the code builder to generate branches
 * when needed to move to the next cache line.
 */
struct codestream {
    struct inst_counters *metrics;   /* For counting instructions of different types */
    unsigned int multiplier;
    int use_alternate;
    unsigned char *base;       /* Base of the whole area */
    size_t size;               /* Size of the whole area */
    unsigned int line_size;    /* Line size e.g. 64 */
    /* The following fields are updated as we generate code */
    unsigned int line_reserve;
    unsigned char *line;
    unsigned char *line_end;
    code_t *p;                 /* running code pointer */
    int ran_out_of_space;
    int error;
};


static void expect_ops(CS *cs, inst_counter_t type, unsigned int n)
{
    cs->metrics->n[type] += n * cs->multiplier;
}


static void expect_op(CS *cs, inst_counter_t type)
{
    expect_ops(cs, type, 1);
}


static void expect_inst(CS *cs, inst_counter_t type)
{
    expect_op(cs, type);
    if (type != COUNT_INST) {
        expect_op(cs, COUNT_INST);
    }
}


void codestream_show(CS const *cs)
{
    fprintf(stderr, "code stream [%p..%p size %#zx] at %p in %p..%p\n",
        cs->base, cs->base+cs->size, cs->size,
        cs->p, cs->line, cs->line_end);
}


static void codestream_start_line(CS *cs, unsigned char *line)
{
    cs->line = line;
    cs->line_end = cs->line + cs->line_size;
    cs->p = (code_t *)cs->line;
#ifdef __x86_64__
    memset(cs->p, 0xCC, cs->line_size);
#endif
}


/*
 * Initialize a code writing stream in a code buffer.
 * In order to get some diversity in code locations, we arrange the
 * buffer as a sequence of code lines.
 * The stream actually goes backwards, starting with the last line.
 * As we reach the end of a line we hop backwards to the previous line.
 */
CS *codestream_init(struct inst_counters *counters,
                    void *base, size_t size, unsigned int line_size)
{
    CS *cs = (CS *)malloc(sizeof(CS));
    if (!cs) {
        return cs;
    }
    memset(cs, 0, sizeof *cs);
    cs->metrics = counters;
    cs->multiplier = 1;
    /* line_size must be a suitable cache line size */
    assert(line_size > 0 && (line_size % 32) == 0);
    /* base is the code area we dynamically allocated */
    assert((unsigned long)base % line_size == 0);
    cs->base = (unsigned char *)base;
    /* The user-provided buffer size doesn't have to be a multiple
       of the line size. We'll have rounded it up when allocating. */
    cs->size = (size + line_size - 1) & (~(line_size-1));
    assert(cs->size >= size);
    assert(cs->size < (size + line_size));
    assert((cs->size % line_size) == 0);
    cs->line_size = line_size;
    cs->ran_out_of_space = 0;
    cs->error = 0;
    codestream_start_line(cs, base + (cs->size - cs->line_size));
    cs->line_reserve = 4;     /* Enough for a short branch */
    return cs;
}

void codestream_use_alternate(CS *cs)
{
    cs->use_alternate = 1;
}

void codestream_set_multiplier(CS *cs, int m)
{
    assert(m >= 0);
    cs->multiplier = m;
}

int codestream_push_multiplier(CS *cs, int m)
{
    assert(m >= 0);
    cs->multiplier *= m;
    return cs->multiplier;
}

int codestream_pop_multiplier(CS *cs, int m)
{
    assert(m >= 0);
    assert((cs->multiplier % m) == 0);
    cs->multiplier /= m;
    return cs->multiplier;
}


void codestream_free(CS *cs)
{
    free(cs);
}

void *codestream_addr(CS const *cs)
{
    return cs->p;
}


/*
 * Set the error flag, which we'll test later
 */
static void codestream_error(CS *cs, char const *reason, ...)
{
    va_list args;
    va_start(args, reason);
    fprintf(stderr, "codegen error: ");
    vfprintf(stderr, reason, args);
    fprintf(stderr, "\n");
    va_end(args);
    cs->error += 1;
}

int codestream_errors(CS const *cs)
{
    return cs->error;
}

static void codestream_gen(CS *cs, code_t data)
{
    *(cs->p++) = data;    
}

void codestream_gen_direct(CS *cs, unsigned int data)
{
    codestream_gen(cs, (code_t)data);
}

static void __attribute__((unused)) codestream_gen2(CS *cs, code_t a, code_t b)
{
    codestream_gen(cs, a);
    codestream_gen(cs, b);
}

static void __attribute__((unused)) codestream_gen3(CS *cs, code_t a, code_t b, code_t c)
{
    codestream_gen(cs, a);
    codestream_gen(cs, b);
    codestream_gen(cs, c);
}

static void __attribute__((unused)) codestream_gen4(CS *cs, code_t a, code_t b, code_t c, code_t d)
{
    codestream_gen(cs, a);
    codestream_gen(cs, b);
    codestream_gen(cs, c);
    codestream_gen(cs, d);
}

#ifdef __x86_64__
static void codestream_gen32(CS *cs, int data)
{
    *(signed int *)(cs->p) = data;
    cs->p += 4;
}
#endif


/*
 * See how many contiguous bytes are left in the current code line.
 * If the next instruction needs more than this we'll need to first
 * branch to a new code line.
 */
static unsigned int codestream_bytes_left(CS const *cs)
{
    int bytes_left_s = (cs->line_end - (unsigned char *)cs->p) - cs->line_reserve;
    if (bytes_left_s < 0) {
        fprintf(stderr, "overran buffer (reserving %u): p=%p, end=%p\n",
            cs->line_reserve, cs->p, cs->line_end);
        /* we hope the caller will assert on -ve result */
        bytes_left_s = 0;
    }
    return (unsigned int)bytes_left_s;
}


int codestream_gen_call(CS *cs, void *dest)
{
    int disp;
#ifdef ARCH_T32
#error TBD
#elif defined(ARCH_A32)
#error TBD
#elif defined(ARCH_A64)
    disp = ((code_t *)dest - cs->p);
    codestream_gen(cs, 0x97000000 | (disp & 0xffffff));
#elif defined(__x86_64__)
    disp = ((code_t *)dest - (cs->p + 5));
    codestream_gen(cs, 0xE8);      /* Relative call */
    codestream_gen32(cs, disp);
#endif
    expect_inst(cs, COUNT_BRANCH);
    return 1;
}


int codestream_gen_ret(CS *cs)
{
#ifdef ARCH_T32
    codestream_gen(cs, 0x4770);
#elif defined(ARCH_A32)
    codestream_gen(cs, 0xe12fff1e);
#elif defined(ARCH_A64)
    codestream_gen(cs, 0xd65f03c0);
#elif defined(__x86_64__)
    codestream_gen(cs, 0xC3);      /* retq */
#endif
    expect_inst(cs, COUNT_BRANCH);
    return 1;
}


int codestream_gen_ret_abi(CS *cs)
{
#if defined(__x86_64__)
    /* Argument in RDI, return in RAX */
    if (cs->metrics->n[COUNT_FLOP_DP] || cs->metrics->n[COUNT_FLOP_SP]) {
        codestream_gen2(cs, 0x0F, 0x77);    /* EMMS after using MMX instructions */
        expect_inst(cs, COUNT_INST);
    }
    codestream_gen3(cs, 0x48, 0x89, 0xF8);  /* mov %rdi,%rax */
    expect_inst(cs, COUNT_MOVE);
#endif
    return codestream_gen_ret(cs);
}



#if defined(ARCH_T32) || defined(ARCH_A32) || defined(ARCH_A64)
static unsigned char const arm_cc[] = {
    0xe, /* AL */
    0x0, /* EQ */
    0x1, /* NE */
    0x8, /* HI */
    0x9, /* LS */
    0x2, /* HS */
    0x3, /* LO */
    0xc, /* GT */
    0xd, /* LE */
    0xa, /* GE */
    0xb, /* LT */
    0x4, /* MI */
    0x5, /* PL: positive or zero */
    0x2, /* CS */
    0x3, /* CC */
    0x6, /* VS */
    0x7  /* VC */
};
typedef int cc_size_check[1/(sizeof arm_cc == CC_MAX)];
#elif defined(__x86_64__)
static unsigned char const x86_cc[] = {
    0xEB, /* j */
    0x74, /* je */
    0x75, /* jne */
    0x77, /* ja (above) / jnbe */
    0x76, /* jbe / jna */
    0x73, /* jae / jnb */
    0x72, /* jb (below) / jnae */
    0x7F, /* jg (greater) */
    0x7E, /* jle */
    0x7D, /* jge */
    0x7C, /* jl */
    0x78, /* js (sign) */
    0x79, /* jns (not sign) */
    0x72, /* jc (carry) */
    0x73, /* jnc (not carry) */
    0x70, /* jo */
    0x71  /* jno */
};
typedef int cc_size_check[1/(sizeof x86_cc == CC_MAX)];
#else
#error Unsupported architecture
#endif

int codestream_gen_branch(CS *cs, void *dest, cc_t cc)
{
    int disp;
    /* Note that the pointer subtractions rely on code_t having the right size. */
#ifdef ARCH_T32
    disp = ((code_t *)dest - (cs->p + 2));
    if (cc == CC_AL) {
        // TBD should use Thumb2 long branch if displacement too large
        codestream_gen(cs, 0xe000 + ((disp>>1) & 0x7ff));
    } else {
        assert(0);   // TBD
    }
#elif defined(ARCH_A32)
    disp = ((code_t *)dest - (cs->p + 2));
    codestream_gen(cs, 0x0a000000 + (arm_cc[cc] << 28) + (disp & 0x00ffffff));
#elif defined(ARCH_A64)
    disp = ((code_t *)dest - cs->p);
    if (cc == CC_AL) {
        codestream_gen(cs, 0x17000000 + (disp & 0x00ffffff));
    } else {
        codestream_gen(cs, 0x54000000 + ((disp & 0x0007ffff) << 5) + arm_cc[cc]);
    }
#elif defined(__x86_64__)
    disp = ((code_t *)dest - (cs->p + 2));
    if (disp == (signed char)disp) {
        unsigned char op = x86_cc[cc];
        assert(op != 0);
        codestream_gen(cs, op);     /* Short jump */
        codestream_gen(cs, (signed char)disp);
    } else {
        if (cc == CC_AL) {
            disp -= 3;       /* Adjust for instruction size difference */
            codestream_gen(cs, 0xE9);     /* Near relative jump */
        } else {
            disp -= 4;
            codestream_gen(cs, 0x0F);     /* Prefix */
            codestream_gen(cs, x86_cc[cc] + 0x10);  /* 0x7x --> 0x8x */
        }
        codestream_gen32(cs, disp);
    }
#endif
    expect_inst(cs, COUNT_BRANCH);
    return 1;
}


/*
 * Check there are enough consecutive bytes left in the current line
 * for the instruction we're about to generate. If so, do nothing.
 * If not, generate a branch to the next line if possible, otherwise
 * report failure.
 */
int codestream_reserve(CS *cs, unsigned int bytes)
{
    unsigned int bytes_left = codestream_bytes_left(cs);
    if (cs->error > 0) {
        /* If we've previously reported an error on a stream,
           indicate that we can't generate any more code.
           This avoids us repeatedly trying to generate an
           unavailable instruction. */
        return 0;
    } else if (bytes_left >= bytes) {
        return 1;
    } else if (cs->line == cs->base) {
        /* This was the last line */
        cs->ran_out_of_space = 1;
        return 0;
    } else {
        /* Generate a branch to the previous line */
        unsigned char *dest = (cs->line - cs->line_size);
        codestream_gen_branch(cs, dest, CC_AL);
        codestream_start_line(cs, dest);
        if (cs->line == cs->base) {
            /* The last line might need a return epilogue - allow space */
            cs->line_reserve = 20;
        }
        assert(codestream_bytes_left(cs) >= bytes);
        return 1;
    }
}


/*
 * Generate a data-processing operation, with up to four register operands.
 *
 * Return 0 if the operation cannot be done on this target, e.g. SIMD size unsupported.
 * Assert if the request is badly formed on any target, e.g. too many registers for operation.
 */
int codestream_gen_op(CS *cs, unsigned int op, flavor_t flavor, freg_t Rd, freg_t Rx, freg_t Ry, freg_t Ra)
{
    unsigned int const esize_bits = FLOAT_BITS(flavor);
    unsigned int const esize_bytes = esize_bits / 8;
    int const is_simd = IS_SIMD(flavor);
    unsigned int const simd_bytes = is_simd ? SIMD_SIZE(flavor) : 0;
    assert(!is_simd || ((simd_bytes*8) >= esize_bits));
    unsigned int const simd_lanes = is_simd ? (simd_bytes / esize_bytes) : 1;
    assert(esize_bits == 16 || esize_bits == 32 || esize_bits == 64);

    //fprintf(stderr, "op=%u flav=%u Rd=%d Rx=%d Ry=%d Ra=%d\n", op, flavor, Rd, Rx, Ry, Ra);
    
    assert(Rd != NR);
    assert(Rx != NR);
    if (op == FP_OP_MOV || op == FP_OP_NEG || op == FP_OP_SQRT) {
        assert(Ry == NR);
    } else {
        assert(Ry != NR);
        if (op == FP_OP_FMA) {
            assert(Ra != NR);
        } else {
            assert(Ra == NR);
        }
    }
    /* n.b. we don't reserve space in the code buffer here. */
#if defined(ARCH_A64)
    int is_bitwise_simd = (is_simd && (op == FP_OP_MOV || op == FP_OP_IXOR));
    unsigned int inst = 0xffffffff;
    if (is_simd && cs->use_alternate) {
        /* SVE instructions */
        static unsigned int const vinsts[] = {
            0x04603000,   /* FMOV */
            0x04200000,   /* ADD */
            0x04a03000,   /* EOR */
            0x041da000,   /* FNEG (predicated) */
            0x65000000,   /* FADD */
            0x65000800,   /* FMUL */
            0x650d8000,   /* FDIV (predicated) */
            0x650da000,   /* FSQRT (predicated) */
            0x65200000,   /* FMLA (predicated) with Rd==Ra */
        };
        assert(op < (sizeof vinsts / sizeof vinsts[0]));
        inst = vinsts[op];
        if (!inst) {
            codestream_error(cs, "arm64: no SVE instruction for op %u", op);
            return 0;
        }
        if (!is_bitwise_simd) { 
            if (esize_bits == 64) {
                inst |= 0x00c00000;
            } else if (esize_bits == 32) {
                inst |= 0x00800000;
            }
        }
        goto a64regs;
    }
    /* For NEON, we support 64-bit and 128-bit operations */
    if (is_simd && !(simd_bytes == 8 || simd_bytes == 16)) {
        codestream_error(cs, "arm64: invalid SIMD size %u bytes", simd_bytes);
        return 0;
    }
    /* AArch64 instruction templates */
    static unsigned int const insts[] = {
        0x1e204000,   /* FMOV */
        0,
        0,
        0x1e214000,   /* FNEG */
        0x1e202800,   /* FADD */
        0x1e200800,   /* FMUL */
        0x1e201800,   /* FDIV */
        0x1e21c000,   /* FSQRT */
        0x1f000000,   /* FFMA */
    };
    static unsigned int const vinsts[] = {
        0x0ea01c00,   /* FMOV (vector): alias of ORR */
        0x0e208400,   /* ADD (vector) */
        0x2e201c00,   /* EOR (vector) */
        0x2ea0f800,   /* FNEG (vector) */
        0x0e20d400,   /* FADD (vector) */
        0x2e20dc00,   /* FMUL (vector) */
        0,            /* no vector FDIV */
        0,            /* no vector FSQRT */
        0x0e20cc00,   /* FMLA (vector) */
    };
    assert(op < (sizeof insts / sizeof insts[0]));
    inst = (is_simd ? vinsts[op] : insts[op]);
    if (inst == 0) {
        /* Something we can't do - e.g. vector DIV on ARM */
        codestream_error(cs, "no encoding for operation %u", op);
        return 0;
    }
    if (is_simd && (esize_bytes == simd_bytes)) {
        /* Single-element double-precision isn't really SIMD */
        codestream_error(cs, "can't do single-element SIMD");
        return 0;
    }
    if (simd_bytes == 16) {
        inst |= 0x40000000;    /* Set Q [30] */
    }
    if (esize_bits == 64 && !is_bitwise_simd) {
        inst |= 0x00400000;    /* Set sz */
    } else if (esize_bits == 16 && !is_bitwise_simd) {
        /* ARMv8.2 half-precision */
        inst ^= 0x0020c000;
    }
a64regs:
    /* Now add in the registers */
    inst |= (Rd << 0);         /* Rd */ 
    switch (op) {
    case FP_OP_ADD:
    case FP_OP_MUL:
    case FP_OP_DIV:
    case FP_OP_IADD:
    case FP_OP_IXOR:
        /* for add, mul, div but not sqrt, there's a third operand */
        inst |= (Rx << 5) | (Ry << 16);
        break;
    case FP_OP_MOV:
        if (is_simd) {
            /* AArch64 MOV (vector) is an alias of ORR */
            inst |= (Rx << 16);
        }
        /* fall through */
    case FP_OP_NEG:
    case FP_OP_SQRT:
        inst |= (Rx << 5);    /* SQRT has only one source operand */
        break;
    case FP_OP_FMA:
        /* for FMA scalar, there are four operands */
        if (is_simd && Rd != Ra) {
            assert(Rd != Rx);
            assert(Rd != Ry);
            codestream_gen_op(cs, FP_OP_MOV, flavor, Rd, Ra, NR, NR);
        }
        inst |= (Rx << 5) | (Ry << 16);   /* multiplier */       
        if (!is_simd) {
            inst |= (Ra << 10);               /* accumulator input */
        }
        break;
    default:
        assert(0);
    }
    codestream_gen(cs, inst);
#elif defined(__x86_64__)
    if (esize_bits == 16) {
        codestream_error(cs, "x86: can't do FP16");
        return 0;
    }
    int const is_dp = (esize_bits == 64);
    int is_evex = (simd_bytes == 64);    /* i.e. AVX512 */
    if (is_simd && !(simd_bytes == 16 || simd_bytes == 32 || simd_bytes == 64)) {
        codestream_error(cs, "x86: invalid SIMD size %u bytes", simd_bytes);
        return 0;
    }
    static unsigned char const inst[] = {
        0x28,  /* mov */
        0xfe,  /* iadd */
        0xef,  /* ixor */ 
        0x57,  /* neg - actually xor */
        0x58,  /* add */
        0x59,  /* mul */
        0x5e,  /* div */
        0x51,  /* sqrt */                
        0xb9,  /* fma */
    };
    assert(op < (sizeof inst / sizeof inst[0]));
    if (is_evex) {
        /* e.g. AVX512 */
        int const single_input = (op == FP_OP_SQRT || op == FP_OP_MOV);
        unsigned char P1 = 0x01, P2 = 0x04, P3 = 0x00;
        unsigned char opcode = inst[op];
        if (op == FP_OP_FMA && Rd != Ra) {
            assert(Rd != Rx);
            assert(Rd != Ry);
            codestream_gen_op(cs, FP_OP_MOV, flavor, Rd, Ra, NR, NR);
        }
        P1 |= 0x70;
        if (single_input) {
            assert(Ry == NR);
            Ry = Rx;
            P2 |= 0x78;
            P3 |= 0x08;
        } else {
            unsigned char nRx = Rx ^ 0x1f;
            P2 |= ((nRx & 15) << 3);
            P3 |= (((nRx & 0x10) >> 4) << 3);
        }
        if (op == FP_OP_FMA) {
            P1 ^= 0x03;
            P2 &= 0xfc;    /* .66 */
            P2 |= 0x01;
            if (is_dp) {
                P2 |= 0x80;        /* .W1 */
            }
            opcode = 0x98;
        } else {
            if (!is_simd) {
                P2 |= 0x02;
            }
            if (is_dp) {
                P2 |= 0x81;
            }
        }
        if (simd_bytes == 64) {
            P3 |= 0x40;
        } else if (simd_bytes == 32) {
            P3 |= 0x20;
        }
        if (!single_input && (Ry & 8)) {
            P1 &= 0xdf;
        }	
        if (Rd & 8) {
            P1 &= 0x7f;
        } else {
            P1 |= 0x80;
        }
        codestream_gen4(cs, 0x62, P1, P2, P3);
        codestream_gen2(cs, opcode, (0xc0 | ((Rd & 7)<<3) | (Ry & 7)));
    } else if (op == FP_OP_MOV || op == FP_OP_IADD || op == FP_OP_IXOR) {
        /* copy Rx to Rd */
        if (esize_bits == 64 || op != FP_OP_MOV) {
            codestream_gen(cs, 0x66);
        }
        if ((Rd & 8) || (Rx & 8)) {
            unsigned char rex = 0x40;
            if (Rd & 8) {
                rex |= 0x04;
            }
            if (Rx & 8) {
                rex |= 0x01;
            }
            codestream_gen(cs, rex);
        }
        codestream_gen3(cs, 0x0f, inst[op], (0xc0 | ((Rd & 7)<<3) | (Rx & 7)));
    } else if (op == FP_OP_FMA) {
        /* Intel processors only support FMA3 instruction set with Rd=Ra */
        unsigned char vex1, vex2, opcode;
        freg_t Rda = Rd;
        if (Rd != Ra) {
            assert(Rd != Rx);
            assert(Rd != Ry);
            codestream_gen_op(cs, FP_OP_MOV, flavor, Rd, Ra, NR, NR);
        }
        codestream_gen(cs, 0xc4);  /* 3-byte VEX prefix */
        vex1 = (Rda & 8) ? 0x62 : 0xe2;
        if (Ry & 8) {
            /* first operand */
            vex1 ^= 0x20;
        }
        codestream_gen(cs, vex1);
        vex2 = (is_dp ? 0xf8 : 0x78);
        vex2 ^= ((Rx & 0xf) << 3);
        if (SIMD_SIZE(flavor) == 32) {
            vex2 |= 0x04;
        }
        vex2 |= 0x01;        
        opcode = inst[op];
        if (is_simd) {
            opcode &= 0xFE;
        }
        codestream_gen3(cs, vex2, opcode, (0xc0 | ((Rda & 7)<<3) | (Ry & 7)));
    } else if (cs->use_alternate) {
        /* old 2-operand style: addss, addsd, addpd etc. */
        if (Rd != Rx) {
            codestream_gen_op(cs, FP_OP_MOV, flavor, Rd, Rx, NR, NR);
        }
        /* Add REX prefix if necessary */
        unsigned char rex = 0x40;
        unsigned char pfx;
        if (!is_simd) {
            pfx = (is_dp ? 0xf2 : 0xf3);
        } else {
            pfx = (is_dp ? 0x66 : 0x00);
        }
        if (pfx != 0x00) {
            codestream_gen(cs, pfx);
        }
        if (Rd & 8) {
            rex |= 0x04;   /* REX.R */
        }
        if (Ry & 8) {
            rex |= 0x01;   /* REX.B */
        }
        if (rex != 0x40) {
            codestream_gen(cs, rex);
        }
        codestream_gen3(cs, 0x0f, inst[op], (0xc0 | ((Rd & 7)<<3) | (Rx & 7)));
    } else {
        /* new 3-operand style: vaddss, vaddsd etc. */
        /* Floating-point negation is not primitive in x86 */
        /* Compilers do it by XORing with -0.0 */
        /* Another way would be subtracting from 0.0 */
        /* For now, we just generate the VXORPS/D, which won't calculate the
           right result, but gets the right performance if we had preloaded
           constant -0.0 into a register. In case there's a special optimization
           for XOR of a register with itself, we force XOR with another register. */
        unsigned char vex;
        int const is_vex3 = ((Ry & 8) != 0);
        int const single_input = (op == FP_OP_SQRT);
        if (op == FP_OP_NEG) {
            Ry = Rx ^ 1;    /* see comment about defeating XOR optimization */
        }
        if (single_input) {
            Ry = Rx;
            Rx = 0xCC;      /* not to be used */
        }
        codestream_gen(cs, (is_vex3 ? 0xc4 : 0xc5));  /* VEX prefix length */
        /* 2nd byte of 2-byte VEX prefix */
        /* 0x80: inverted REX.R */
        vex = 0;                
        if (!(Rd & 8)) {
            vex |= 0x80;    /* VEX.R = ~REX.R */
        }            
        if (is_vex3) {                    
            vex |= 0x01;    /* leading 0F, same as 2-byte VEX */
            vex |= 0x40;    /* VEX.X = ~REX.X = 0 */
            if ((Ry & 8) == 0) {
                vex |= 0x20;    /* VEX.B = 1 */
            }
            codestream_gen(cs, vex);
            vex = 0;
        }
        /* Last or only byte of VEX prefix */
        /* Set VEX.L */
        if (SIMD_SIZE(flavor) == 32) {             
            vex |= 0x04;    /* 256-bit vectors */
        }
        /* Set VEX.pp */ 
        if (!(is_simd || op == FP_OP_NEG)) {
            vex |= (is_dp ? 0x03 : 0x02);  /* Implied prefix: DP:F2 vs. SP:F3 */
        } else {
            vex |= (is_dp ? 0x01 : 0x00);  /* Implied prefix: DP:66 vs. SP:none */
        }       
        /* register specifier in 1s-complement form */                
        if (!single_input) {
            vex |= ((Rx & 15) ^ 0xf) << 3;
        } else {
            /* single-input */
            vex |= (0xf << 3);
        }
        codestream_gen3(cs, vex, inst[op], (0xc0 | ((Rd & 7)<<3) | (Ry & 7)));
    }
#else
#error Unsupported architecture
#endif
    if (op == FP_OP_MOV) {
        /* A register move doesn't count as a floating-point operation */
        expect_inst(cs, COUNT_MOVE);
    } else {
        unsigned int n = simd_lanes;
        if (op == FP_OP_FMA) {
            /* FMA counts as two FP operations */
            n *= 2;
        }
        expect_inst(cs, COUNT_INST);
        expect_ops(cs, (esize_bits == 64) ? COUNT_FLOP_DP : (esize_bits == 16) ? COUNT_FLOP_HALF : COUNT_FLOP_SP, n);        
    }
    return 1;
}


int codestream_gen_nop(CS *cs)
{
#if defined(ARCH_A64)
    codestream_gen(cs, 0xd503201f);
#elif defined(__x86_64__)
    codestream_gen(cs, 0x90);
#else
#error Unsupported architecture
#endif
    expect_inst(cs, COUNT_INST);
    return 1;
}


#ifdef __x86_64__
/* Map logical (argument index) reg no. to actual reg no. */
static unsigned char reg_map(ireg_t r)
{
    /* Argument registers are RDI, RSI, RDX, RCX, R8, R9 */
    /* To use R8, R9 we'd need to use a prefix e.g. 0x41 */
    static unsigned char const reg_map_a[] = {
        0x7,   /* RDI */
        0x6,   /* RSI */
        0x2,   /* RDX */
        0x1,   /* RCX */
        /* EAX is 0 */
        /* EBX is 3 */
        /* EBP is 5 */
    };
    assert(r < sizeof reg_map_a);
    return reg_map_a[r];
}
#endif


/*
 * Decrement a register by 1 and set the Z flag.
 */
int codestream_gen_decs(CS *cs, ireg_t Rd)
{
    unsigned char k = 1;   /* The constant */
#if defined(ARCH_A64)
    codestream_gen(cs, (0x71000000 | (k << 10) | (Rd << 5) | (Rd)));
#elif defined(__x86_64__)
    if (Rd < 4) {
        codestream_gen3(cs, 0x83, (0xe8 | reg_map(Rd)), k);
    } else {
        codestream_gen4(cs, 0x41, 0x83, (0xe8 | (Rd-4)), k);
    }
#else
#error Unsupported architecture
#endif
    expect_inst(cs, COUNT_INST);
    return 1;
}


int codestream_gen_iopk(CS *cs, unsigned int iop, ireg_t Rd, ireg_t Rn, int k)
{
#if defined(ARCH_A64)
    unsigned int opcode = 0xBAD;
    if (iop == CS_IOP_ADD) {
        opcode = 0x91000000;
    } else if (iop == CS_IOP_SUB) {
        opcode = 0xd1000000;
    } else {
        assert(0);
    }
    codestream_gen(cs, (opcode | (k << 10) | (Rn << 5) | (Rd)));
#elif defined(__x86_64__)
    assert(0);
#else
#error Unsupported architecture
#endif
    expect_inst(cs, COUNT_INST);
    return 1;
}


/*
 * Load a register with an immediate value.
 */
int codestream_gen_movi32(CS *cs, ireg_t Rd, uint32_t n)
{
#if defined(ARCH_A64)
    /* On AArch64, constants must be done 16 bits at a time */
    codestream_gen(cs, 0xd2800000 | ((n & 0xffff) << 5) | Rd);
    if (n >= 0x10000) {
        n >>= 16;
        /* MOVK into bits 31:16 */
        codestream_gen(cs, 0xf2a00000 | ((n & 0xffff) << 5) | Rd);
    }
#elif defined(__x86_64__)
    codestream_gen(cs, 0xb8 | reg_map(Rd));
    codestream_gen32(cs, n);
#else
#error Unsupported architecture
#endif
    expect_inst(cs, COUNT_INST);
    return 1;
}


/*
 * Check if an immediate fits in a given signed/unsigned bit width.
 */
static int __attribute__((unused)) fits_simm(long x, unsigned int n_bits)
{
    return x >= (-1U << (n_bits-1)) && x < (1U << (n_bits-1));
}

static int __attribute__((unused)) fits_uimm(long x, unsigned int n_bits)
{
    return x >= 0 && x < (1U << n_bits);
}


/* To share code between load and store, we add an internal-only flag. */
#define _internal_STORE 0x80000000

/*
 * Generate a single load instruction.
 * Will return zero if the requested combination of index register and offset can't be done.
 */
int codestream_gen_load(CS *cs, ireg_t Rt, ireg_t Rn, ireg_t Radd, int offset, unsigned int flags)
{
    assert((Rt == NR) == ((flags & CS_LOAD_PREFETCH) != 0));
#if defined(ARCH_A64)
    /* flags[0] set to 1 for STRM */
    /* flags[2:1] set to 00 for L1, 01 for L2, 10 for L3 */
    /* flags[4:3] set to 00 for LD, 01 for LI, 10 for ST */
    unsigned int prefetch_flags = 0;   /* LOAD KEEP L1 */
    unsigned int flavor_flags = (flags & _internal_STORE) ? 0x00000000 : 0x00400000;
    if (flags & CS_LOAD_NONTEMPORAL) {
        prefetch_flags |= 0x01;
    }
    if (flags & _internal_STORE) {
        prefetch_flags |= 0x10;
    }
    if ((flags & CS_LOAD_NONTEMPORAL) && !(flags & CS_LOAD_PREFETCH) && offset != 0) {
        codestream_error(cs, "load offset invalid with non-temporal demand-load");
        return 0;
    }
    if ((flags & CS_LOAD_ATOMIC) && (offset != 0)) {
        codestream_error(cs, "load offset invalid with atomic load");
        return 0;
    }
    if ((flags & CS_LOAD_ATOMIC) && (flags & CS_LOAD_NONTEMPORAL)) {
        codestream_error(cs, "unsupported combination of atomic and non-temporal");
        return 0;
    }
    if ((flags & CS_LOAD_ACQUIRE) && (Radd != NR || offset != 0)) {
        codestream_error(cs, "offset or index invalid with load-acquire/store-release");
        return 0;
    }
    if (Radd == NR) {                
        unsigned int opcode = 0x00000000;
        if ((offset & 7) != 0 || !fits_uimm(offset>>3, 12)) {
            codestream_error(cs, "load offset %ld is invalid", (long)offset);
            return 0;
        }
        if (flags & CS_LOAD_PREFETCH) {
            /* Store and non-temporality in prefetch_flags */
            opcode = (0xf9800000 | (Rn << 5) | ((offset>>3) << 10) | prefetch_flags); /* PRFM Rt,[Rn,#offset] */
        } else if (flags & CS_LOAD_NONTEMPORAL) {
            if (flags & _internal_STORE) {
                codestream_error(cs, "can't do non-temporal single-word store");
                return 0;
            }
            opcode = (0xa8400000 | (31 << 10) | (Rn << 5) | ((offset>>3) << 15) | Rt); /* LDNP Rt,xzr,[Rn,#offset] */
        } else if (flags & CS_LOAD_PAIR) {
            if (flags & CS_LOAD_ACQUIRE) {
                codestream_error(cs, "can't do pair load-acquire");
                return 0;
            }
            opcode = (0xa8c00000 | (31 << 10) | (Rn << 5) | ((offset>>3) << 15) | Rt); /* LDP Rt,xzr,[Rn,#offset] */
        } else if (flags & CS_LOAD_ATOMIC) {
            opcode = (0xf8202000 | (0x1f << 16) | (Rn << 5) | Rt);      /* LDEOR xzr,Rt,[Rn] */
            if (flags & CS_LOAD_ACQUIRE) {
                opcode |= 0x00800000;
            }
        } else if (flags & CS_LOAD_ACQUIRE) {
            /* Also CS_STORE_RELEASE: store done in flavor_flags */
            opcode = (0xc89ffc00 | flavor_flags | (Rn << 5) | Rt);     /* LDAR Rt,[Rn] */
        } else {
            opcode = (0xf9000000 | flavor_flags | (Rn << 5) | ((offset>>3) << 10) | Rt); /* LDR Rt,[Rn,#offset] */
        }
        codestream_gen(cs, opcode);
    } else {
        if (offset != 0) {
            codestream_error(cs, "cannot combine register and immediate offset");
            return 0;
        }
        if (flags & CS_LOAD_PREFETCH) {
            /* Store and non-temporality in prefetch_flags */
            codestream_gen(cs, (0xf8a0c800 | (Rn << 5) | (Radd << 16) | prefetch_flags));  /* PRFM Rt,[Rn,Radd,sxtw] */
        } else {
            codestream_gen(cs, (0xf860c800 | flavor_flags | (Rn << 5) | (Radd << 16) | Rt));  /* LDR Rt,[Rn,Radd,sxtw] */
        }
    }
#elif defined(__x86_64__)
    /* Argument registers are RDI, RSI, RDX, RCX, R8, R9 */
    assert(Rt == NR || Rt < sizeof reg_map);
    assert(Rn < sizeof reg_map);
    if (offset != 0) {
        codestream_error(cs, "x86 load immediate offset TBD");
        return 0;   /* TBD */
    }
    if (Radd == NR) {
	if (flags & CS_LOAD_PREFETCH) {
	    codestream_gen3(cs, 0x0f, 0x18, (0x08 | (reg_map(Rn))));
        } else {
            codestream_gen3(cs, 0x48, 0x8b, ((reg_map(Rt) << 3) | (reg_map(Rn) << 0)));   /* e.g. (%rdi),%rdi */
	}
    } else {
        /* e.g. mov %rdi,(%rdi+%rsi*1) */
        if (flags & CS_LOAD_PREFETCH) {
	    codestream_gen4(cs, 0x0f, 0x18, 0x0c, ((reg_map(Rn) << 3) | (reg_map(Radd) << 0)));
	} else { 
            codestream_gen4(cs, 0x48, 0x8b, ((reg_map(Rt) << 3) | 0x04), ((reg_map(Rn) << 3) | (reg_map(Radd) << 0)));
	}
    }
#else
#error Unsupported architecture
#endif
    expect_inst(cs, ((flags & CS_LOAD_PREFETCH) ? COUNT_MEM_PREFETCH : (flags & _internal_STORE) ? COUNT_INST_WR : COUNT_INST_RD));
    expect_ops(cs, ((flags & _internal_STORE) ? COUNT_BYTES_WR : COUNT_BYTES_RD), sizeof(void *));
    return 1;
}


int codestream_gen_store(CS *cs, ireg_t Rt, ireg_t Rn, ireg_t Radd, int offset, unsigned int flags)
{
    return codestream_gen_load(cs, Rt, Rn, Radd, offset, flags|_internal_STORE);
}


int codestream_gen_fp_load(CS *cs, flavor_t flavor, freg_t Rt, ireg_t Rn, int offset, unsigned int flags)
{
    assert(!(flags & CS_LOAD_PREFETCH));
#if defined(ARCH_A64)
    unsigned int xflags = (flags & _internal_STORE) ? 0x00000000 : 0x00400000;
    uint32_t opcode = 0xbd000000 | xflags | (offset << 10) | (Rn << 5) | (Rt << 0);
    unsigned int const esize_bits = FLOAT_BITS(flavor);
    if (esize_bits == 64) {
        opcode |= 0x40000000;   /* 0xbd...... -> 0xfd...... */
    }
    assert(offset >= 0 && offset <= 64);
    codestream_gen(cs, opcode);
#elif defined(__x86_64__)
    fprintf(stderr, "x86: FP load/store not implemented\n");
    return 0;
#else
#error Unsupported architecture
#endif
    expect_inst(cs, ((flags & _internal_STORE) ? COUNT_INST_WR : COUNT_INST_RD));
    expect_ops(cs, ((flags & _internal_STORE) ? COUNT_BYTES_WR : COUNT_BYTES_RD), esize_bits / 8);
    return 1;
}


int codestream_gen_fp_store(CS *cs, flavor_t flavor, freg_t Rt, ireg_t Rn, int offset, unsigned int flags)
{
    return codestream_gen_fp_load(cs, flavor, Rt, Rn, offset, flags|_internal_STORE);
}


int codestream_gen_fence(CS *cs, unsigned int flags)
{
    assert((flags & (CS_FENCE_STORE|CS_FENCE_LOAD)) != 0);
#if defined(ARCH_A64)
    unsigned int opcode = 0xd50338bf;   /* DMB ISH */
    if (flags & CS_FENCE_LOAD) {
        opcode |= 0x00000100;
    }
    if (flags & CS_FENCE_STORE) {
        opcode |= 0x00000200;
    }
    if (flags & CS_FENCE_SYSTEM) {
        opcode |= 0x00000400;           /* change ISH to SY */
    }
    if (flags & CS_FENCE_SYNC) {
        opcode &= ~0x00000020;          /* change DMB to DSB */
    }
    codestream_gen(cs, opcode);
#elif defined(__x86_64__)
    unsigned int const lsflags = flags & (CS_FENCE_STORE|CS_FENCE_LOAD);
    /* TBD: not sure I understand x86 fences. Useful exposition at
       https://hadibrais.wordpress.com/2018/05/14/the-significance-of-the-x86-lfence-instruction/ */
    if (lsflags == CS_FENCE_LOAD) {
        codestream_gen3(cs, 0x0F, 0xAE, 0xE8);   /* LFENCE */
    } else if (flags == CS_FENCE_STORE) {
        codestream_gen3(cs, 0x0F, 0xAE, 0xF8);   /* SFENCE */
    } else if (lsflags == (CS_FENCE_STORE|CS_FENCE_LOAD)) {
        codestream_gen3(cs, 0x0F, 0xAE, 0xF0);   /* MFENCE */
    }
#endif
    expect_inst(cs, COUNT_FENCE);
    return 1;
}

