/** @file
 * Copyright (c) 2020, 2022-2023, Arm Limited or its affiliates. All rights reserved.
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

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_nist.h"
#include "include/sbsa_avs_common.h"
#include <math.h>

/**
  @brief   This API executes all the PCIe tests sequentially
  @param   level  - level of compliance being tested for.
  @param   num_pe - the number of PE to run these tests on.

  @return  Consolidated status of all the tests run.
**/
uint32_t
val_nist_execute_tests(uint32_t level, uint32_t num_pe)
{
  uint32_t status, i;
  (void) level;

  for (i = 0; i < g_num_skip; i++) {
      if (g_skip_test_num[i] == AVS_NIST_TEST_NUM_BASE) {
          val_print(AVS_PRINT_INFO, "      USER Override - Skipping all NIST tests \n", 0);
          return AVS_STATUS_SKIP;
      }
  }

  /* Check if there are any tests to be executed in current module with user override options*/
  status = val_check_skip_module(AVS_NIST_TEST_NUM_BASE);
  if (status) {
      val_print(AVS_PRINT_INFO, "\n USER Override - Skipping all NIST tests \n", 0);
      return AVS_STATUS_SKIP;
  }

  val_print_test_start("NIST");
  status = n001_entry(num_pe);

  val_print_test_end(status, "NIST");

  return status;
}

/**
  @brief   This API generates a 32 bit random number.
  @param   rng_buffer    - Pointer to store the random data.

  @return  success/failure.
**/
uint32_t
val_nist_generate_rng(uint32_t *rng_buffer)
{
  uint32_t status;

  status = pal_nist_generate_rng(rng_buffer);
  return status;
}

double
erf(double x)
{
    // constants
    double t, y;
    double a1 =  0.254829592;
    double a2 = -0.284496736;
    double a3 =  1.421413741;
    double a4 = -1.453152027;
    double a5 =  1.061405429;
    double p  =  0.3275911;
    // Save the sign of x
    int sign = 1;
    if (x < 0)
        sign = -1;
    x = fabs(x);
    // A&S formula 7.1.26

    t = 1.0/(1.0 + p*x);
    y = 1.0 - (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);
    return sign*y;
}

double
erfc(double x)
{
    double t, z, ans;
    z = fabs(x);
    t = 1.0 / (1.0 + 0.5 * z);

    ans = t*exp(-z*z-1.26551223+t*(1.00002368+t*(0.37409196+t*(0.09678418+
        t*(-0.18628806+t*(0.27886807+t*(-1.13520398+t*(1.48851587+
        t*(-0.82215223+t*0.17087277)))))))));

    return x >= 0.0 ? ans : 2.0-ans;
}
