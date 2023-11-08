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

  .section .text.sysreg, "ax"

    .global val_mair_write
val_mair_write:
    cmp        x1, #2
    b.ne    mair_el1_write
    msr     mair_el2, x0
    ret

mair_el1_write:
    msr     mair_el1, x0
    ret

    .global val_tcr_write
val_tcr_write:
    cmp        x1, #2
    b.ne    tcr_el1_write
    msr     tcr_el2, x0
    tlbi    alle2
    dsb     sy
    isb
    ret
tcr_el1_write:
    msr     tcr_el1, x0
    tlbi     vmalle1
    dsb      sy
    isb
    ret

    .global val_ttbr0_write
val_ttbr0_write:
    cmp        x1, #2
    b.ne    ttbr0_el1_write
    msr     ttbr0_el2, x0
    ret

ttbr0_el1_write:
    msr     ttbr0_el1, x0
    ret

    .global val_ttbr0_read
val_ttbr0_read:
    cmp        x0, #2
    b.ne    ttbr0_el1_read
    mrs     x0, ttbr0_el2
    ret
ttbr0_el1_read:
    mrs     x0, ttbr0_el1
    ret

    .global val_sctlr_read
val_sctlr_read:
    cmp        x0, #2
    b.ne    sctlr_el1_read
    mrs     x0, sctlr_el2
    isb
    ret
sctlr_el1_read:
    mrs     x0, sctlr_el1
    isb
    ret

    .global val_sctlr_write
val_sctlr_write:
    cmp        x1, #2
    b.ne    sctlr_el1_write
    msr     sctlr_el2, x0
    isb
    ret
sctlr_el1_write:
    msr     sctlr_el1, x0
    isb
    ret

    .global val_esr_el1_read
val_esr_el1_read:
    mrs     x0, esr_el1
    ret

    .global val_elr_el1_read
val_elr_el1_read:
    mrs     x0, elr_el1
    ret

    .global val_elr_el1_write
val_elr_el1_write:
    msr     elr_el1, x0
    ret

    .global val_far_el1_read
val_far_el1_read:
    mrs     x0, far_el1
    ret

    .global val_hpfar_el2_read
val_hpfar_el2_read:
    mrs     x0, hpfar_el2
    ret

    .global val_id_aa64mmfr0_el1_read
val_id_aa64mmfr0_el1_read:
    mrs     x0, id_aa64mmfr0_el1
    ret

    .global val_id_aa64mmfr1_el1_read
val_id_aa64mmfr1_el1_read:
        mrs x0, ID_AA64MMFR1_EL1
        ret

    .global val_esr_el2_read
val_esr_el2_read:
    mrs     x0, esr_el2
    ret

    .global val_elr_el2_read
val_elr_el2_read:
    mrs     x0, elr_el2
    ret

    .global val_read_mpidr
val_read_mpidr:
    mrs     x0, mpidr_el1
    ret

    .global val_read_current_el
val_read_current_el:
    mrs     x0, CurrentEL
    ret

    .global val_dataCacheCleanInvalidateVA
val_dataCacheCleanInvalidateVA:
    dc  civac, x0
    dsb sy
    isb
    ret

    .global val_dataCacheCleanVA
val_dataCacheCleanVA:
    dc  cvac, x0
    dsb ish
    isb
    ret

    .global val_dataCacheInvalidateVA
val_dataCacheInvalidateVA:
    dc  ivac, x0
    dsb ish
    isb
    ret

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

    /* ------------------------------------------
     * Invalidate from base address till
     * size. 'x0' = addr, 'x1' = size
     * ------------------------------------------
     */
    .global val_inv_dcache_range
val_inv_dcache_range:
    do_dcache_maintenance_by_mva ivac

     /* ------------------------------------------
     * Invalidate from base address till
     * size. 'x0' = addr, 'x1' = size
     * ------------------------------------------
     */
    .global val_inv_icache_range
val_inv_icache_range:
    do_icache_maintenance_by_mva ivau

