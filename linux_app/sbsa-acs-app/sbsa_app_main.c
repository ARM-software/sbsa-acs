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
#include "include/sbsa_app.h"

int  g_sbsa_level = 3;
int  g_print_level = 3;
int  g_skip_test_num[3];

int
initialize_test_environment()
{

    return call_drv_init_test_env();
}

void
cleanup_test_environment()
{

    call_drv_clean_test_env();
}

int
main (int argc, char **argv)
{

    int   c = 0;
    char *endptr;
    int   status;

    /* Process Command Line arguments */
    while ((c = getopt (argc, argv, "v:l:")) != -1)
    {
       switch (c)
       {
       case 'v':
         g_print_level = strtol(optarg, &endptr, 10);
         break;
       case 'l':
         g_sbsa_level = strtol(optarg, &endptr, 10);
         break;
       case '?':
         if (isprint (optopt))
           fprintf (stderr, "Unknown option `-%c'.\n", optopt);
         else
           fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
         return 1;
       default:
         abort ();
       }
    }


    printf ("\n ************ SBSA Architecture Compliance Suite ********* \n");
    printf ("                version %d.%d  \n", SBSA_APP_VERSION_MAJOR, SBSA_APP_VERSION_MINOR);


    printf ("\n Starting Compliance verification for Level %2d (Print level is %2d)\n\n", g_sbsa_level, g_print_level);

    printf (" Gathering System information.... \n");
    status = initialize_test_environment();
    if (status) {
        printf ("Cannot Initialize test environment. Exiting.... \n");
        return 0;
    }

    printf("\n      *** Starting PCIe tests ***  \n");
    execute_tests_pcie(1, g_sbsa_level);


    printf("\n      *** SBSA Compliance Test Complete.. *** \n\n");

    cleanup_test_environment();

    return 0;
}
