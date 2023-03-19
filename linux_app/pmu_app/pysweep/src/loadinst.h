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
 * Generate instructions in memory.
 *
 * This is a set of general-purpose routines suitable for JITting code.
 */

#ifndef __included_loadinst_h
#define __included_loadinst_h

#include "arch.h"

#include <stddef.h>
#include <stdint.h>


typedef struct codestream CS;


/* Code stream management */

void codestream_show(CS const *);

struct inst_counters;

CS *codestream_init(struct inst_counters *m, void *base, size_t size, unsigned int line_size);

void *codestream_addr(CS const *);

void codestream_use_alternate(CS *);

void codestream_set_multiplier(CS *, int);

/* Push and pop a multiple, e.g. when looping by a fixed amount */
int codestream_push_multiplier(CS *, int);
int codestream_pop_multiplier(CS *, int);

int codestream_errors(CS const *);

void codestream_free(CS *);

/*
 * Check there are enough consecutive bytes left in the current line
 * for the instruction we're about to generate. If so, do nothing.
 * If not, generate a branch to the next line if possible, otherwise
 * report failure.
 */
int codestream_reserve(CS *, unsigned int bytes);


/* Instruction generators */

typedef unsigned int ireg_t;
/* Integer argument registers */
#define IR0 0
#define IR1 1
#define IR2 2
#define IR3 3
#define IR4 4
#define IR5 5

typedef unsigned int freg_t;
#define NR 0xFF     /* no register - placeholder for instructions with less than max no. of regs */

/*
 * We define various 'flavors' of FP/SIMD operation.
 */
typedef unsigned int flavor_t;
#define F16   0x01
#define F32   0x02
#define F64   0x03
#define S64   0x08
#define S128  0x10   /* 16 bytes */
#define S256  0x20   /* 32 bytes */
#define S512  0x40   /* 64 bytes */
#define S1024 0x80   /* 128 bytes */

#define FLOAT_BITS(t) (8U << ((t) & 0x03))
#define SIMD_SIZE(t)  ((t) & 0xff8)
#define IS_SIMD(t)    (((t) & 0xff8) != 0)


/*
 * Move an immediate value into a register, or return 0 if not possible in one instruction.
 */
int codestream_gen_movi32(CS *, ireg_t Rd, uint32_t n);

/*
 * Decrement an integer register and set the Z flag.
 */
int codestream_gen_decs(CS *, ireg_t Rd);

/*
 * Integer operation
 */
#define CS_IOP_ADD 0
#define CS_IOP_SUB 1
int codestream_gen_iopk(CS *, unsigned int iop, ireg_t Rd, ireg_t Rn, int k);

/*
 * Floating-point (or vector) operation on floating-point/vector registers.
 */
int codestream_gen_op(CS *, unsigned int op, flavor_t flavor, freg_t Rd, freg_t Rx, freg_t Ry, freg_t Ra);

/*
 * Generate a load:
 *   Rt = *(Rn + Radd + offset).
 * Radd may be NR.
 */
#define CS_LOAD_DEFAULT      0x00
#define CS_LOAD_NONTEMPORAL  0x01   /* Use a non-temporal (streaming) load */
#define CS_LOAD_PAIR         0x02   /* Use a load-pair instruction */
#define CS_LOAD_PREFETCH     0x04   /* Generate a prefetch instead of a load (Rt==NR) */
#define CS_LOAD_ACQUIRE      0x08   /* Use load-acquire if available */
#define CS_LOAD_ATOMIC       0x10   /* Use a load-atomic */
int codestream_gen_load(CS *, ireg_t Rt, ireg_t Rn, ireg_t Radd, int offset, unsigned int flags);

int codestream_gen_fp_load(CS *, flavor_t flavor, freg_t Rt, ireg_t Rn, int offset, unsigned int flags);


/*
 * Generate a store. This has similar constraints as load.
 */
#define CS_STORE_DEFAULT     0x00
#define CS_STORE_NONTEMPORAL 0x01
#define CS_STORE_RELEASE     0x08
int codestream_gen_store(CS *, ireg_t Rt, ireg_t Rn, ireg_t Radd, int offset, unsigned int flags);

int codestream_gen_fp_store(CS *, flavor_t flavor, freg_t Rt, ireg_t Rn, int offset, unsigned int flags);

/* Generate an explicit barrier/fence instruction */
#define CS_FENCE_LOAD        0x01
#define CS_FENCE_STORE       0x02
#define CS_FENCE_SYSTEM      0x04   /* Domain: on Arm, use SY instead of ISH */
#define CS_FENCE_SYNC        0x08   /* Synchronize: on Arm, use DSB instead of DMB */
int codestream_gen_fence(CS *, unsigned int flags);

int codestream_gen_nop(CS *);

int codestream_gen_call(CS *, void *dest);

int codestream_gen_ret(CS *);

int codestream_gen_ret_abi(CS *);

/*
 * Generate an instruction or part of an instruction directly. Specialist use only.
 */
void codestream_gen_direct(CS *, unsigned int x);


/*
 * Condition encodings. Several abstract conditions might have the same encoding,
 * but this is different for different ISAs. E.g. "unsigned less than"
 * is the same as "carry set" for x86, but "carry clear" for ARM.
 *
 * N.b. tables in loadinst.c are indexed by this enum. Keep in sync.
 */
typedef enum {
    CC_AL,
    CC_EQ,
    CC_NE,
    CC_UGT,
    CC_ULE,
    CC_UGE,
    CC_ULT,
    CC_SGT,
    CC_SLE,
    CC_SGE,
    CC_SLT,
    CC_NEG,
    CC_NNG,
    CC_CS,
    CC_CC,
    CC_VS,
    CC_VC,
    CC_MAX
} cc_t;

int codestream_gen_branch(CS *, void *dest, cc_t cc);



#endif /* included */

/* end of loadinst.h */
