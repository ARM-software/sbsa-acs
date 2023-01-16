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
 * Enable/disable support for denormal FP operations.
 */

#include "denormals.h"

#include "arch.h"

#include <stdio.h>
#include <assert.h>
#if defined(__x86_64__)
#include <xmmintrin.h>
#endif


#if defined(__x86_64__)

#define FPCR_FZ 0x8040
#define get_fpcr _mm_getcsr
#define set_fpcr _mm_setcsr

#elif defined(ARCH_A64)

#define FPCR_FZ 0x01000000

static uint32_t get_fpcr(void)
{
    uint32_t r;
    __asm__ __volatile__("mrs %0,fpcr":"=r"(r)::);
    return r;
}

static void set_fpcr(uint32_t x)
{
    __asm__ __volatile__("msr fpcr,%0"::"r"(x):);
}

#else
/* On other architectures, we don't know how to get the FPCR */
#endif


/*
 * Interrogate the floating-point control word to see if denormals are enabled.
 */
static int __attribute__((unused)) get_denormals_enabled_hw(void)
{
#ifdef FPCR_FZ
    return (get_fpcr() & FPCR_FZ) == 0;
#else
    fprintf(stderr, "** Can't get denormal status from hardware on this architecture\n");
    assert(0);
    return 0;
#endif
}


/*
 * Discover if denormals are enabled, empirically
 */
static int denormals_are_enabled_empirical(void)
{
    double volatile x = DOUBLE_DENORMAL;
    x += DOUBLE_DENORMAL;
    return (x != 0.0);
}


/*
 * Discover if denormals are enabled, empirically and (if possible) by interrogating
 * the floating-point control word. If these tests don't correspond, raise an error.
 */
int denormals_are_enabled(void)
{
    int enabled = denormals_are_enabled_empirical();
#ifdef FPCR_FZ
    assert(enabled == get_denormals_enabled_hw());
#endif
    return enabled;
}


int denormals_set_enabled(int enable)
{
    if (enable) {
        /* Clear DAZ and FTZ - denormals participate in arithmetic */
#ifdef FPCR_FZ
        set_fpcr(get_fpcr() & ~FPCR_FZ);
#else
        /* Can't control - succeed if denormals are already enabled */
#endif
    } else {
        /* Set DAZ and FTZ - denormals will be eliminated */
#ifdef FPCR_FZ
        set_fpcr(get_fpcr() | FPCR_FZ);
#else
        /* Can't control - succeed if denormals are already disabled */
#endif
    }
    /* If we can't control, we succeed iff denormals are already in
       the enablement state we wanted. */
    return enable == denormals_are_enabled();
}


/*
 * Get the raw representation of floating-point numbers.
 * Simplest is
 *   *(unsigned int *)&x;
 * but that will be faulted (at -O2) by -Werror=strict-aliasing
 */
static unsigned int float_as_int(float x)
{
    union {
        float f;
        unsigned int i;
    } u;
    u.f = x;
    return u.i;
}


static unsigned long long double_as_int(double x)
{
    union {
        double f;
        unsigned long long i;
    } u;
    u.f = x;
    return u.i;
}


void test_denormals(void)
{
    float volatile xf;
    double volatile xd;
    int was_enabled = denormals_are_enabled();
    printf("Denormals test: ");
#ifdef __cplusplus
    printf("C++");
#else
    printf("C");
#endif
    printf("\n");
    printf("No-denormals:\n");
    denormals_set_enabled(0);
    xd = DOUBLE_DENORMAL;
    printf("  %016llx (%.20g)\n", double_as_int(xd), xd);
    xf = FLOAT_DENORMAL;
    /* The float-to-double conversion will be flushed-to-zero on input */
    printf("  %08x (%.10g) - will print as zero\n", float_as_int(xf), xf);
    assert(xd == 0.0);     /* It will compare equal to zero, even though it's not */
    xd += DOUBLE_DENORMAL;
    printf("  %016llx CSR=0x%04x\n", double_as_int(xd), (unsigned int)get_fpcr());
    assert(xd == 0.0);
    printf("Denormals:\n");
    denormals_set_enabled(1);
    xd = DOUBLE_DENORMAL;
    printf("  %016llx (%g)\n", double_as_int(xd), xd);
    assert(xd != 0.0);
    xf = FLOAT_DENORMAL;
    printf("  %08x (%g)\n", float_as_int(xf), xf);
    xd += DOUBLE_DENORMAL;
    printf("  %016llx CSR=0x%04x\n", double_as_int(xd), (unsigned int)get_fpcr());
    assert(xd != 0.0);
    /* Restore original state */
    denormals_set_enabled(was_enabled);
}


#ifdef COMPILE_DENORMALS_AS_MAIN
int main(void)
{
    test_denormals();
    return 0;
}
#endif
