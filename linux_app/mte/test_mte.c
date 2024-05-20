/** @file
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#define TEST_RULE "B_PE_16"
#define TEST_DESC "Check for MTE support     "

#define HWCAP2_MTE              (1 << 18)
#define HWCAP2_MTE3             (1 << 22)
#define PROT_MTE                 0x20
#define PR_SET_TAGGED_ADDR_CTRL 55
#define PR_TAGGED_ADDR_ENABLE  (1UL << 0)
#define PR_MTE_TCF_SHIFT       1
#define PR_MTE_TCF_NONE        (0UL << PR_MTE_TCF_SHIFT)
#define PR_MTE_TCF_SYNC        (1UL << PR_MTE_TCF_SHIFT)
#define PR_MTE_TCF_ASYNC       (2UL << PR_MTE_TCF_SHIFT)
#define PR_MTE_TCF_MASK        (3UL << PR_MTE_TCF_SHIFT)
#define PR_MTE_TAG_SHIFT       3
#define PR_MTE_TAG_MASK        (0xffffUL << PR_MTE_TAG_SHIFT)
#define SIGSEGV_EXIT_CODE 139
#define EXIT_SKIPPED 2

/* insert a random logical tag into the given pointer */
#define insert_random_tag(ptr) ({                       \
        uint64_t __val;                                 \
        asm("irg %0, %1" : "=r" (__val) : "r" (ptr));   \
        __val;                                          \
})

/* set the allocation tag on the destination address */
#define set_tag(tagged_addr) do {                                      \
        asm volatile("stg %0, [%0]" : : "r" (tagged_addr) : "memory"); \
} while (0)



uint32_t payload(void)
{
    unsigned char *var;
    unsigned long page_sz = sysconf(_SC_PAGESIZE);
    unsigned long hwcap2 = getauxval(AT_HWCAP2);
    uint32_t pid;
    uint32_t childReturnCode;
    uint64_t timeout = 100000;

    /* check if MTE is supported */
    if (!((hwcap2 & HWCAP2_MTE) || (hwcap2 & HWCAP2_MTE3))) {
        perror("\n      Memory tagging extension(MTE2 and MTE3) not supported.");
        return EXIT_SKIPPED;
    }

    /* enable the tagged address ABI, synchronous or asynchronous MTE tag check faults */
    if (prctl(PR_SET_TAGGED_ADDR_CTRL,
                PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC | PR_MTE_TCF_ASYNC |
                (0xfffe << PR_MTE_TAG_SHIFT),
                0, 0, 0)) {
            perror("\n      prctl() failed");
            return EXIT_FAILURE;
    }

    var = mmap(0, page_sz, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (var == MAP_FAILED) {
            perror("\n      mmap() failed");
            return EXIT_FAILURE;
    }

    /* Enable MTE on the above mmap.*/
    if (mprotect(var, page_sz, PROT_READ | PROT_WRITE | PROT_MTE)) {
            perror("\n      mprotect() failed");
            return EXIT_FAILURE;
    }

    /* access with the default tag (0) */
    var[0] = 1;
    var[1] = 10;
    printf("\n      Access with the default tag (0)");
    printf("\n      var[0] = %hhu var[1] = %hhu", var[0], var[1]);
    printf("\n      Default tag(0) pointer : %p", var);

    /* set the logical and allocation tags */
    var = (unsigned char *)insert_random_tag(var);
    set_tag(var);

    /* access with non-zero tag */
    var[0] = 3;
    printf("\n      Access with the non-zero tag");
    printf("\n      var[0] = %hhu var[1] = %hhu", var[0], var[1]);
    printf("\n      Non zero tag pointer : %p", var);

    /* If MTE is enabled correctly the access to next 16 byte granule will cause an exception
        due to mismatch in logical and allocation tags */

    /* creating a child process and accessing var[16] or later to cause exception */
    /*delay for above printf*/
    while (timeout--);
    pid = fork();

    if (pid == 0) {
        /* true for child process */
        printf("\n      Expecting SIGSEGV exception...");
        var[18] = 0xdd;

        while (timeout--);
        exit(0);
    }
    else if (pid < 0) {
        /* fork failure */
        printf("\n      Fork failed.");
        return EXIT_FAILURE;
    }
    else {
        /* parent process wait and check return status */
        waitpid(pid, &childReturnCode, 0);
        printf("\n      Child process returned with exit code: %d", childReturnCode);

        if (childReturnCode == SIGSEGV_EXIT_CODE) {
            return EXIT_SUCCESS;
        }
        else {
            printf("\n      Not received SIGSEGV");
            return EXIT_FAILURE;
        }
    }
    return EXIT_FAILURE;
}

void main(void)
{
    uint32_t status;
    /* test start */
    printf("\n%s: %s:", TEST_RULE, TEST_DESC);
    /* call test payload */
    status = payload();
    /*check test return status*/
    if (status == EXIT_SUCCESS)
        printf("\nResult: PASS\n");
    else if (status == EXIT_SKIPPED)
        printf("\nResult: SKIP\n");
    else
        printf("\nResult: FAIL\n");

    return;
}