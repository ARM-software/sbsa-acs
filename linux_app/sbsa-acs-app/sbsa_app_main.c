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
#include <getopt.h>

int  g_sbsa_level = 3;
int  g_print_level = 3;
int  g_skip_test_num[3] = {10000, 10000, 10000};
unsigned long int  g_exception_ret_addr;

int
initialize_test_environment(unsigned int print_level)
{

    return call_drv_init_test_env(print_level);
}

void
cleanup_test_environment()
{

    call_drv_clean_test_env();
}

void print_help(){
  printf ("\nUsage: Sbsa [-v <n>] | [-l <n>] | [--skip <n>]\n"
         "Options:\n"
         "-v      Verbosity of the Prints\n"
         "        1 shows all prints, 5 shows Errors\n"
         "-l      Level of compliance to be tested for\n"
         "        As per SBSA spec, 0 to 3\n"
         "--skip  Test(s) to be skipped\n"
         "        Refer to section 4 of SBSA_ACS_User_Guide\n"
         "        To skip a module, use Model_ID as mentioned in user guide\n"
         "        To skip a particular test within a module, use the exact testcase number\n"
  );
}

int
main (int argc, char **argv)
{

    int   c = 0,i=0;
    char *endptr, *pt;
    int   status;

    struct option long_opt[] =
    {
      {"skip", required_argument, NULL, 'n'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}
    };

    /* Process Command Line arguments */
    while ((c = getopt_long(argc, argv, "hv:l:", long_opt, NULL)) != -1)
    {
       switch (c)
       {
       case 'v':
         g_print_level = strtol(optarg, &endptr, 10);
         break;
       case 'l':
         g_sbsa_level = strtol(optarg, &endptr, 10);
         break;
       case 'h':
         print_help();
         return 1;
         break;
       case 'n':/*SKIP tests */
         pt = strtok(optarg, ",");
         while((pt!=NULL) && (i<3)){
           int a = atoi(pt);
           g_skip_test_num[i++] = a;
           pt = strtok(NULL, ",");
         }
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
    printf ("                        Version %d.%d  \n", SBSA_APP_VERSION_MAJOR, SBSA_APP_VERSION_MINOR);


    printf ("\n Starting tests for level %2d (Print level is %2d)\n\n", g_sbsa_level, g_print_level);

    printf (" Gathering system information.... \n");
    status = initialize_test_environment(g_print_level);
    if (status) {
        printf ("Cannot initialize test environment. Exiting.... \n");
        return 0;
    }

    printf("\n      *** Starting PCIe tests ***  \n");
    execute_tests_pcie(1, g_sbsa_level, g_print_level);


    printf("\n                    *** SBSA tests complete *** \n\n");

    cleanup_test_environment();

    return 0;
}
