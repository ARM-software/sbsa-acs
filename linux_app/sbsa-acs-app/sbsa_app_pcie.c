/** @file
 * Copyright (c) 2017, ARM Limited or its affiliates. All rights reserved.

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

#include "sbsa_avs_common.h"

/**
  This function calls the SBSA Kernel Module in a loop to execute all the PCIe tests
**/
int
execute_tests_pcie(int num_pe, int level)
{

    int status;
    int test_num;

    for (test_num = AVS_PCIE_TEST_NUM_BASE + 1; test_num <= AVS_PCIE_TEST_NUM_BASE + 7; test_num++)
    {
        /* Execute PCIe tests one bys one */
        printf("Executing Test %d :  ", test_num);
        call_drv_execute_test(test_num, num_pe, level, 0);
        status  = call_drv_wait_for_completion();
        if (IS_TEST_PASS(status))
            printf("RESULT: PASS \n");
        else {
            if (IS_TEST_SKIP(status))
                printf("RESULT: SKIP \n");
            else
                printf("RESULT: FAIL \n");
            if (test_num == AVS_PCIE_TEST_NUM_BASE + 1) {
                printf("\n No ECAM, No point continuing with PCIe tests \n");
                break;
            }
        }
    }
}
