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

#include "SbsaAvs.h"

    .extern val_main
    .extern vector_table
    .extern val_fixup_symbol_table
    .extern val_inv_dcache_range
    .extern val_inv_icache_range
    .extern g_primary_mpidr

    .cfi_sections .debug_frame
    .globl    acs_host_entry
    .section .text.acs_host_entry, "ax"

.macro    dcache_line_size  reg, tmp
    mrs    \tmp, ctr_el0
    ubfx    \tmp, \tmp, #16, #4
    mov    \reg, #4
    lsl    \reg, \reg, \tmp
.endm

.macro    icache_line_size  reg, tmp
    mrs    \tmp, ctr_el0
    and    \tmp, \tmp, #0xf
    mov    \reg, #4
    lsl    \reg, \reg, \tmp
.endm

.macro do_icache_maintenance_by_mva op
    /* Exit early if size is zero */
    cbz    x1, exit_loop_\op
    icache_line_size x2, x3
    add    x1, x0, x1
    sub    x3, x2, #1
    bic    x0, x0, x3
loop_\op:
    ic    \op, x0
    add    x0, x0, x2
    cmp    x0, x1
    b.lo    loop_\op
    dsb    sy
exit_loop_\op:
    ret
.endm

/*
 * This macro can be used for implementing various data cache operations `op`
 */
.macro do_dcache_maintenance_by_mva op
    /* Exit early if size is zero */
    cbz    x1, exit_loop_\op
    dcache_line_size x2, x3
    add    x1, x0, x1
    sub    x3, x2, #1
    bic    x0, x0, x3
loop_\op:
    dc    \op, x0
    add    x0, x0, x2
    cmp    x0, x1
    b.lo    loop_\op
    dsb    sy
exit_loop_\op:
    ret
.endm

acs_host_entry:
   /* Install vector table */
    adr x0, vector_table
    msr  vbar_el2, x0

   /* Set x19 = 1 for primary cpu
    * Set x19 = 0 for secondary cpu
    */
    adr x18, g_primary_mpidr
    ldr x0, [x18]
    mov   x2, #INVALID_MPIDR
    cmp   x2, x0
    b.eq  primary_cpu_entry

    mrs x2, mpidr_el1
    cmp   x2, x0
    b.ne  secondary_cpu_entry


primary_cpu_entry:
    mov   x19, #1

   /*
     * Invalidate the instr cache for the code region.
     * This prevents re-use of stale data cache entries from
     * prior bootloader stages.
     */
    adr x0, __TEXT_START__
    adr x1, __TEXT_END__
    sub x1, x1, x0
    bl  val_inv_icache_range

    /*
     * Invalidate the data cache for the data regions.
     * This prevents re-use of stale data cache entries from
     * prior bootloader stages.
     */
    adrp x0, __RODATA_START__
    add	 x0, x0, :lo12:__RODATA_START__
    adrp x1, __RODATA_END__
    add	x1, x1, :lo12:__RODATA_END__
    sub x1, x1, x0
    bl  val_inv_dcache_range


    /* Enable I-Cache */
    mrs  x0, sctlr_el2
    orr  x0, x0, #SCTLR_I_BIT
    msr  sctlr_el2, x0
    isb

    /* Save the primary cpu mpidr */
    adr x18, g_primary_mpidr
    mrs x0, mpidr_el1
    str x0, [x18]

   /* Clear BSS */
  adrp x0, __BSS_START__
  add  x0,x0,:lo12:__BSS_START__
  adrp x1, __BSS_END__
  add  x1,x1,:lo12:__BSS_END__
  sub x1, x1, x0
  //bl  val_inv_dcache_range

1:
   stp xzr, xzr, [x0]
   add x0, x0, #16
   sub x1, x1, #16
   cmp xzr, x1
   b.ne 1b

   b  0f

secondary_cpu_entry:
    mov   x19, #0

    /* Enable I-Cache */
    mrs  x0, sctlr_el2
    orr  x0, x0, #SCTLR_I_BIT
    msr  sctlr_el2, x0
    isb

0:
   /* Setup the dummy stack to call val_get_pe_id C fn */
    adrp  x1, dummy_stack_end
    add x1,x1,#:lo12:dummy_stack_end
    mov  sp, x1

    mrs  x0, mpidr_el1
    bl   val_get_pe_id

    /* Now setup the stack pointer with actual stack addr
     * for the logic PE id return by val_get_pe_id
     */
    adrp  x1, stacks_end
    add x1,x1,#:lo12:stacks_end
    mov  x2, #STACK_SIZE
    mul  x2, x0, x2
    sub  sp, x1, x2

    /* And jump to the C entrypoint. */
    mov  x0, x19
    b    ShellAppMainsbsa

val_inv_icache_range:
    do_icache_maintenance_by_mva ivau

val_inv_dcache_range:
    do_dcache_maintenance_by_mva ivac


.section .bss.stacks
  .global stacks_start
  .global stacks_end
    .balign CACHE_WRITEBACK_GRANULE
stacks_start:
    .fill    STACK_SIZE * PLATFORM_CPU_COUNT
stacks_end:
    .fill    STACK_SIZE
dummy_stack_end:
