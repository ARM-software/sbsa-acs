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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define MAX_LENGTH 5

void resetOnly(int *len, char *addr)
{
    if (len == NULL|| addr == NULL)
        return;

    memset(addr, 0, *len);
    *len = 0;

    return;
}

void printAndReset(int *len, char *addr)
{
    int i;

    if (len == NULL|| addr == NULL)
        return;

//    printf("length = %d\n", (*len));
    for (i = 0; i < (*len); i++) {
//         printf("%c ", addr[i]);
    }

//    printf("\n");

    memset(addr, 0, *len);
    *len = 0;

    return;
}

int branch_load_gen(int scale)
{
    int j, i, acronymLength = 0, blockCount;
    char acronym[MAX_LENGTH] = {0};
    char c;
    int length = 100; // string length
    char string[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";

    srand(time(NULL));

    // A: loop not entered 1/LOOP_COUNT times
    for(j = 0; j < scale; j++) {
        //printf("Starting iteration #%d\n", j);
        blockCount = 0;
        c = 0;
        resetOnly(&acronymLength, acronym);
        // B: loop not entered 1/length times
        for(i = 0; i < length; i++) {
            c = string[(rand() % (int)(sizeof(string) - 1))];
            // C: condition true
            // (number_of_block_letters)/(total_characters_in_string) times
            if (c >= 'A' && c <= 'Z') {
                blockCount++;
                // D: condition true up to MAX_LENGTH times consecutively
                if (acronymLength < MAX_LENGTH) {
                    acronym[acronymLength] = c;
                }
                // E: condition true up to MAX_LENGTH+1 times consecutively
                if (acronymLength <= MAX_LENGTH) {
                    acronymLength++;
                }
            }
            else {
                // F: condition true if E was true then C was false
                if (acronymLength > 1 && acronymLength <= MAX_LENGTH) {
                    printAndReset(&acronymLength, acronym);
                }
                // G: condition true if E was false then C was false
                else if (acronymLength != 0) {
                    resetOnly(&acronymLength, acronym);
                }
            }
        }
    }

    return 0;
}
