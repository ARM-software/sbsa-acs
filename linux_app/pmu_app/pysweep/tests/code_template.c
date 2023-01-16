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
 * Templates for the kind of code we're going to be dynamically generating.
 * We don't auto-generate the generator from this, it's just for convenience.
 */

#include <math.h>

int *arg_to_return(int *p) { return p; }

int *arg_load_to_return(int **p) { return *p; }

int *arg_load_load_to_return(int ***p) { return **p; }

extern int *giver(void);
extern void takerp(int *p);
extern void takerpp(int *p, int *q);

void arg_load_to_arg(int **p) { takerp(*p); }
void arg_load_to_arg2(int **p) { takerpp(0,*p); }
void arg2_load_to_arg(int **p, int **q) { takerp(*q); }

int *arg_load_indexed(int a, int **b, int c) { return *(int **)((unsigned char *)b + c); }
int *arg_load_indexed2(int a, int b, int **c) { return *(int **)((unsigned char *)c + b); }
void arg_load_indexed_to_arg(int a, int **b, int c) { takerpp(0,*(int **)((unsigned char *)b + c)); }

void arg_store(int *p) { *p = 1; }

void return_to_arg(void) { takerp(giver()); }

extern void taker2(float, float);
extern void taker3(float, float, float);
void float_to_arg2(float x) { taker2(0.0, x); }

int mul32(int x, int y) { return x * y; }
long long mul64(long long x, long long y) { return x * y; }
int div32(int x, int y) { return x / y; }
long long div64(long long x, long long y) { return x / y; }

float add_float00(float x, float y) { return x + x; }
float add_float01(float x, float y) { return x + y; }
float add_float11(float x, float y) { return add_float00(x, y+y); }

void sub_float01to2(float x, float y) { taker3(0.0, 0.0, x - y); }

double neg_double(double x) { return -x; }
double add_double(double x, double y) { return x + y; }
double mul_double(double x, double y) { return x * y; }
double div_double(double x, double y) { return x / y; }
double sqrt_double(double x) { return __builtin_sqrt(x); }
double fma_double(double x, double y, double z) { return x + (y*z); }
#ifdef FP_FAST_FMA
float fma2_float(float x, float y, float z) { return fmaf(y,z,x); }
float fma2_float11(float x, float y, float z) { return fmaf(y,y,x); }
float fma2_float22(float x, float y, float z) { return fmaf(z,z,x); }
double fma2_double(double x, double y, double z) { return fma(y,z,x); }
#endif

double load_double_r0(double *p) { return *p; }
double load_double_r1(double *p, double *q) { return *q; }
float load_float_r0(float *p) { return *p; }
void store_float_r0(float *p, float x) { *p = x; }
void store_float1_r0(float *p, float x, float y) { *p = y; }

#if 1
/*
 * Try to generate a vector instruction by auto-vectorization.
 */
#define GENVEC(name, T, op) \
void vec_##name(T *__restrict a, T const *__restrict b, T const *__restrict c) { \
    int i; for (i = 0; i < 4; ++i) { \
        T cb = b[i] op c[i]; T r = cb + cb; a[i] += r; } }

GENVEC(fadd, float, +)
GENVEC(fmul, float, *)
GENVEC(dadd, double, +)
GENVEC(iadd, int, +)
GENVEC(ieor, int, ^)
GENVEC(imul, int, *)
GENVEC(ladd, long long, +)
#endif

void countdown(int n) {
  do {
    taker2(0.0, 0.0);    
    n--;
  } while (n >= 0);
}

int small_integer(void) { return 12; }
int small_neg_integer(void) { return -12; }
int large_integer(void) { return 12983489; }


void pfetch(void *p) { __builtin_prefetch(p); }
void pfetch2(void *x, void *p) { __builtin_prefetch(p); }
void pfetcha(void *p, int n) { __builtin_prefetch((char *)p + n); }
void pfetcha2(void *p, int a, int n) { __builtin_prefetch((char *)p + n); }
void pfetcha3(void *x, void *p, int n) { __builtin_prefetch((char *)p + n); }

