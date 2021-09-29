/** @file
 * Copyright (c) 2021, Arm Limited or its affiliates. All rights reserved.
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

#include "include/sbsa_avs_pe.h"
#include "sys_arch_src/gic/sbsa_exception.h"

//
// Private worker functions for ASM_PFX()
//
#define _CONCATENATE(a, b)  __CONCATENATE(a, b)
#define __CONCATENATE(a, b) a ## b

#define __USER_LABEL_PREFIX__
//
// The __USER_LABEL_PREFIX__ macro predefined by GNUC represents the prefix
// on symbols in assembly language.
//
#define ASM_PFX(name) _CONCATENATE (__USER_LABEL_PREFIX__, name)

#define GCC_ASM_EXPORT(func__)  \
       .global  _CONCATENATE (__USER_LABEL_PREFIX__, func__)    ;\
       .type ASM_PFX(func__), %function

GCC_ASM_IMPORT(common_exception_handler)
GCC_ASM_EXPORT(sbsa_gic_set_el2_vector_table)
GCC_ASM_EXPORT(sbsa_gic_update_elr)
GCC_ASM_EXPORT(sbsa_gic_get_far)
GCC_ASM_EXPORT(sbsa_gic_get_esr)
GCC_ASM_EXPORT(sbsa_gic_get_elr)


#define CHECK_EL_1_OR_2_OR_3(REG) \
        mrs    REG, CurrentEL ;\
        cmp    REG, #0x8      ;\
        b.gt   3f             ;\
        b.eq   2f             ;\
        cbnz   REG, 1f        ;\
        b      .              ;// We should never get here

/**
 * This is a generic handler for exceptions taken at the current EL while using
 * SPx. It saves volatile registers, calls the C handler, restores volatile
 * registers, then returns.
 *
 * Saving state and jumping to C handler takes 15 instructions. We add an extra
 * branch to a common exit path. So each handler takes up one unique cache line
 * and one shared cache line (assuming 16-byte cache lines).
 */
.macro current_exception_spx elx:req handler:req
    save_volatile_to_stack \elx
    bl \handler
    b restore_from_stack_and_return
.endm

.macro current_el_with_spx elx:req val:req
    save_volatile_to_stack \elx
    mov x0, #\val
    bl  ASM_PFX(common_exception_handler)
    b restore_from_stack_and_return
.endm

/**
 * Saves the volatile registers onto the stack. This currently takes 14
 * instructions, so it can be used in exception handlers with 18 instructions
 * left, 2 of which in the same cache line (assuming a 16-byte cache line).
 *
 * On return, x0 and x1 are initialised to elr_el2 and spsr_el2 respectively,
 * which can be used as the first and second arguments of a subsequent call.
 */
.macro save_volatile_to_stack elx:req
    /* Reserve stack space and save registers x0-x18, x29 & x30. */
    stp x0, x1, [sp, #-(8 * 24)]!
    stp x2, x3, [sp, #8 * 2]
    stp x4, x5, [sp, #8 * 4]
    stp x6, x7, [sp, #8 * 6]
    stp x8, x9, [sp, #8 * 8]
    stp x10, x11, [sp, #8 * 10]
    stp x12, x13, [sp, #8 * 12]
    stp x14, x15, [sp, #8 * 14]
    stp x16, x17, [sp, #8 * 16]
    str x18, [sp, #8 * 18]
    stp x29, x30, [sp, #8 * 20]

    /*
     * Save elr_elx & spsr_elx. This such that we can take nested exception
     * and still be able to unwind.
     */
    CHECK_EL_1_OR_2_OR_3(x0)
1:  mrs   x0, elr_el1
    mrs   x1, spsr_el1
    b     4f
2:  mrs   x0, elr_el2
    mrs   x1, spsr_el2
    b     4f
3:  mrs   x0, elr_el3
    mrs   x1, spsr_el3
4:  stp x0, x1, [sp, #8 * 22]
.endm

  .section .text.vtable, "ax"
  .align 12

.global vector_table
vector_table:

// ------------------------------------------------------------
// Current EL with SP0
// ------------------------------------------------------------
  .balign 128
sync_current_el_sp0:
  current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
irq_current_el_sp0:
  current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
fiq_current_el_sp0:
  current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
serror_current_el_sp0:
  current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

// ------------------------------------------------------------
// Current EL with SPx
// ------------------------------------------------------------

  .balign 128
sync_current_el_spx:
  current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
irq_current_el_spx:
  current_el_with_spx el2 EXCEPT_AARCH64_IRQ

  .balign 128
fiq_current_el_spx:
  current_el_with_spx el2 EXCEPT_AARCH64_FIQ

  .balign 128
serror_current_el_spx:
  current_el_with_spx el2 EXCEPT_AARCH64_SERROR

// ------------------------------------------------------------
// Lower EL using AArch64
// ------------------------------------------------------------

  .balign 128
sync_lower_el_aarch64:
  B        .                   //       Synchronous exception
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
irq_lower_el_aarch64:
  B        .                    //        IRQ
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
fiq_lower_el_aarch64:
  B        .                    //        FIQ
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
serror_lower_el_aarch64:
  B        .                    //        SError
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

// ------------------------------------------------------------
// Lower EL using AArch32
// ------------------------------------------------------------

  .balign 128
sync_lower_el_aarch32:
  B        .
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
irq_lower_el_aarch32:
  B        .                    //        IRQ
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
fiq_lower_el_aarch32:
  B        .                    //        FIQ
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
serror_lower_el_aarch32:
  B        .                    //        SError
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

.balign 0x40
/**
 * Restores the volatile registers from the stack.

 * Register x0: if false restores elr_el2, if true retains the value of elr_el2.
 * This enables exception handlers to indicate whether they have changed the
 * value of elr_el2 (e.g., to skip the faulting instruction).
 */
restore_from_stack_and_return:
    /* Restore registers x2-x18, x29 & x30. */
    ldp x2, x3, [sp, #8 * 2]
    ldp x4, x5, [sp, #8 * 4]
    ldp x6, x7, [sp, #8 * 6]
    ldp x8, x9, [sp, #8 * 8]
    ldp x10, x11, [sp, #8 * 10]
    ldp x12, x13, [sp, #8 * 12]
    ldp x14, x15, [sp, #8 * 14]
    ldp x16, x17, [sp, #8 * 16]
    ldr x18, [sp, #8 * 18]
    ldp x29, x30, [sp, #8 * 20]

    cbnz x0, skip_elr

    /* Restore register elr_el2 using x1 as scratch. */
    ldr x1, [sp, #8 * 22]
    CHECK_EL_1_OR_2_OR_3(x0)
1:  msr   elr_el1, x1
    b     4f
2:  msr   elr_el2, x1
    b     4f
3:  msr   elr_el3, x1
4:  isb

skip_elr:
    /* Restore register spsr_el2 using x1 as scratch. */
    ldr x1, [sp, #8 * 23]
    CHECK_EL_1_OR_2_OR_3(x0)
1:  msr   spsr_el1, x1
    b     4f
2:  msr   spsr_el2, x1
    b     4f
3:  msr   spsr_el3, x1
4:  isb

    /* Restore x0 & x1, and release stack space. */
    ldp x0, x1, [sp], #8 * 24

    eret


// ------------------------------------------------------------
// Set Vector Table
// ------------------------------------------------------------

ASM_PFX(sbsa_gic_set_el2_vector_table):
    adr x0, vector_table
    CHECK_EL_1_OR_2_OR_3(x1)
1:  msr   vbar_el1, x0
    b     4f
2:  msr   vbar_el2, x0
    b     4f
3:  msr   vbar_el3, x0
4:  isb
    ret

ASM_PFX(sbsa_gic_update_elr):
    CHECK_EL_1_OR_2_OR_3(x1)
1:  msr   elr_el1, x0
    b     4f
2:  msr   elr_el2, x0
    b     4f
3:  msr   elr_el3, x0
4:  isb
    ret

ASM_PFX(sbsa_gic_get_elr):
    CHECK_EL_1_OR_2_OR_3(x1)
1:  mrs   x0, elr_el1
    b     4f
2:  mrs   x0, elr_el2
    b     4f
3:  mrs   x0, elr_el3
4:  ret

ASM_PFX(sbsa_gic_get_esr):
    CHECK_EL_1_OR_2_OR_3(x1)
1:  mrs   x0, esr_el1
    b     4f
2:  mrs   x0, esr_el2
    b     4f
3:  mrs   x0, esr_el3
4:  ret

ASM_PFX(sbsa_gic_get_far):
    CHECK_EL_1_OR_2_OR_3(x1)
1:  mrs   x0, far_el1
    b     4f
2:  mrs   x0, far_el2
    b     4f
3:  mrs   x0, far_el3
4:  ret
