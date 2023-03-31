/** @file
 * Copyright (c) 2023 Arm Limited or its affiliates. All rights reserved.
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


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "include/sbsa_app.h"
#include "include/sbsa_drv_intf.h"


extern unsigned int *g_skip_test_num;

/**
  This function calls the SBSA Kernel Module in a loop to execute all the SMMU tests
**/
int
execute_tests_smmu(int num_pe, int level, unsigned int print_level)
{

    int status;

    call_update_skip_list(SBSA_UPDATE_SKIP_LIST, g_skip_test_num);
    call_drv_execute_test(SBSA_SMMU_EXECUTE_TEST, num_pe, level, print_level, 0);
    status  = call_drv_wait_for_completion();

    return status;
}

