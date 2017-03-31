/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

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

#include <stdint.h>
#include "include/sbsa_drv_intf.h"

#define SIZE 1
#define NUMELEM 5

typedef
struct __SBSA_DRV_PARMS__
{
    unsigned int    api_num;
    unsigned int    num_pe;
    unsigned int    level;
    unsigned long   arg0;
    unsigned long   arg1;
    unsigned long   arg2;
}sbsa_drv_parms_t;


int
call_drv_get_status(unsigned long int *arg0, unsigned long int *arg1, unsigned long int *arg2)
{

    FILE  *fd = NULL;
    sbsa_drv_parms_t test_params;


    fd = fopen("/proc/sbsa", "r");

    fseek(fd, 0, SEEK_SET);

    if (NULL == fd)
    {
        printf("fopen failed \n");
        return 1;
    }
  fread(&test_params,1,sizeof(test_params),fd);

  //printf("read back value is %x %lx \n", test_params.api_num, test_params.arg1);

  fclose(fd);

  *arg0 = test_params.arg0;
  *arg1 = test_params.arg1;
  *arg2 = test_params.arg2;

  return test_params.api_num;
}

int
call_drv_wait_for_completion()
{
  unsigned long int arg0, arg1, arg2;

  arg0 = DRV_STATUS_PENDING;

  while (arg0 == DRV_STATUS_PENDING)
    call_drv_get_status(&arg0, &arg1, &arg2);

  return arg1;
}


int
call_drv_init_test_env()
{
    FILE             *fd = NULL;
    sbsa_drv_parms_t test_params;

    fd = fopen("/proc/sbsa", "rw+");

    if (NULL == fd)
    {
        printf("fopen failed \n");
        return 1;
    }

    test_params.api_num  = SBSA_CREATE_INFO_TABLES;
    test_params.arg1     = 0;
    test_params.arg2     = 0;

    fwrite(&test_params,1,sizeof(test_params),fd);

    fclose(fd);

    call_drv_wait_for_completion();

}

int
call_drv_clean_test_env()
{
    FILE             *fd = NULL;
    sbsa_drv_parms_t test_params;

    fd = fopen("/proc/sbsa", "rw+");

    if (NULL == fd)
    {
        printf("fopen failed \n");
        return 1;
    }

    test_params.api_num  = SBSA_FREE_INFO_TABLES;
    test_params.arg1     = 0;
    test_params.arg2     = 0;

    fwrite(&test_params,1,sizeof(test_params),fd);

    fclose(fd);

    call_drv_wait_for_completion();
}

int
call_drv_execute_test(unsigned int test_num, unsigned int num_pe,
  unsigned int level, unsigned long int test_input)
{
    FILE             *fd = NULL;
    sbsa_drv_parms_t test_params;

    fd = fopen("/proc/sbsa", "rw+");

    if (NULL == fd)
    {
        printf("fopen failed \n");
        return 1;
    }

    test_params.api_num  = SBSA_EXECUTE_TEST;
    test_params.num_pe   = num_pe;
    test_params.level    = level;
    test_params.arg0     = test_num;
    test_params.arg1     = test_input;
    test_params.arg2     = 0;

    fwrite(&test_params,1,sizeof(test_params),fd);

    fclose(fd);

}


int
pal_os_driver_release()
{



}

