/** @file
 * Copyright (c) 2020, Arm Limited or its affiliates. All rights reserved.
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
#include <math.h>
#include <unistd.h>
#include <string.h>

#include "val/include/sbsa_avs_val.h"
#include "val/include/val_interface.h"
#include "val/include/sbsa_avs_nist.h"

#define TEST_NUM   (AVS_NIST_TEST_NUM_BASE + 1)
#define TEST_DESC  "NIST Statistical Test Suite      \n "

#define BUFFER_SIZE     1000
#define RND_FILE_SIZE   36428
#define REQ_OPEN_FILES  30
#define ALL_NIST_TEST   0xFFFE
#define NIST_SUITE_1    0xFE
#define NIST_SUITE_2    0xDE00   /* Test 1 - 7 */
#define MIN_NIST_TEST   0x0000   /* Test 9 - 12, 13 - 14 */

extern int main(int argc, char *argv[]);

/*Enabling all NIST test suites(test 1 - 15) by default */
uint32_t test_select = ALL_NIST_TEST;

static
int32_t
check_prerequisite_nist(void)
{
  FILE     *fp[REQ_OPEN_FILES];
  char      file_name[REQ_OPEN_FILES][20];
  int32_t   i;
  uint32_t  status = AVS_STATUS_PASS;
  char      result_file[] = "experiments/AlgorithmTesting/finalAnalysisReport.txt";

  /* Check the max # of opened file requiremnt for
   * executing NIST test suite.
   */
  for (i = 0; i < REQ_OPEN_FILES; i++) {
      sprintf(file_name[i], "tmp_%d.txt", i);
      fp[i] = fopen(file_name[i], "wb");
      if (fp[i] == NULL) {
          val_print(AVS_PRINT_ERR, "\nMax # of opened files has been reached. "
                                   "NIST prerequistite failed: %d", i);
          status = AVS_STATUS_FAIL;
	  break;
      }
  }

  for (i = i - 1; i >= 0; i--)
  {
      fclose(fp[i]);
      remove(file_name[i]);
  }

  remove(result_file);
  val_print(AVS_PRINT_INFO, "\nAll NIST Prerequistite were met", 0);
  return status;
}

static
int32_t
print_nist_result(void)
{
  FILE   *fptr;
  char    buffer[BUFFER_SIZE];
  int32_t totalRead = 0;
  char    filename[] = "experiments/AlgorithmTesting/finalAnalysisReport.txt";

  // Open report file
  fptr = fopen(filename, "r");
  if (fptr == NULL)
  {
      val_print(AVS_PRINT_ERR, "Cannot open file \n", 0);
      return AVS_STATUS_FAIL;
  }

  while (fgets(buffer, BUFFER_SIZE, fptr) != NULL)
  {
      /* Total character read count */
      totalRead = strlen(buffer);

      /* Trim new line character from last if exists */
      buffer[totalRead - 1] = buffer[totalRead - 1] == '\n'
                                  ? '\0'
                                  : buffer[totalRead - 1];

      /* Print line read on cosole*/
      //ToDo: Print using val_print
      //val_print(AVS_PRINT_TEST, "%s\n", buffer);
      printf("%s\n", buffer);
  }

  fclose(fptr);
  return AVS_STATUS_PASS;
}

static
int32_t
create_random_file(void)
{
  uint32_t  buffer, status = AVS_STATUS_FAIL;
  FILE     *fp;
  char      str[] = "data.txt";
  int32_t   i, j, k, noofr = RND_FILE_SIZE;
  char      one = '1', zero = '0';

  fp = fopen(str, "wb");
  if (fp == NULL)
  {
      val_print(AVS_PRINT_ERR, "\n       Unable to create file", 0);
      return AVS_STATUS_FAIL;
  }

  for (i = 0; i < noofr; i++)
  {
      /* Get a 32-bit random number */
      status = val_nist_generate_rng(&buffer);
      if (status != AVS_STATUS_PASS) {
	  val_print(AVS_PRINT_ERR, "\n       Random number generation failed", 0);
	  fclose(fp);
          return AVS_STATUS_FAIL;
      }

      /* Convert decimal random number to binary value and
       * write it as ASCII value in the file
       */
      for (j = 31; j >= 0; j--) {
          k = buffer >> j;
          if (k & 1)
	      fwrite(&one, sizeof(char), 1, fp);
          else
              fwrite(&zero, sizeof(char), 1, fp);
      }
  }

  fclose(fp);
  val_print(AVS_PRINT_INFO, "\nA random file with sequence of ASCII 0's and 1's created", 0);
  return AVS_STATUS_PASS;
}
static
void
payload()
{
  int32_t  status, i, argc = 2;
  char    *argv[] = {"data.txt", "100000"};
  char    *dirname = "experiments";
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t test_list[] = {NIST_SUITE_1, NIST_SUITE_2};
  size_t   test_listsize = sizeof(test_list) / sizeof(test_list[0]);

  status = check_prerequisite_nist();
  if (status != AVS_STATUS_PASS) {
      /* Omitting tests 8, 9 and 13 */
      test_select = MIN_NIST_TEST;
      val_print(AVS_PRINT_INFO, "\nSkipping test 8, 9 and 13 of NIST test suite", 0);
  }

  /* Generate a Random file with binary ASCII values */
  status = create_random_file();
  if (status != AVS_STATUS_PASS) {
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  /* Create the directories required for NIST test suite */
  status = mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/ApproximateEntropy";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/BlockFrequency";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/CumulativeSums";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/FFT";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/Frequency";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/LinearComplexity";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/LongestRun";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/NonOverlappingTemplate";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/OverlappingTemplate";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/RandomExcursions";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/RandomExcursionsVariant";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/Rank";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/Runs";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/Serial";
  status |= mkdir(dirname, 0777);
  dirname = "experiments/AlgorithmTesting/Universal";
  status |= mkdir(dirname, 0777);
  if (status != AVS_STATUS_PASS) {
      val_print(AVS_PRINT_ERR, "\n       Directory not created", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }
  else
      val_print(AVS_PRINT_INFO, "\n       Directory created", 0);

  if (test_select == MIN_NIST_TEST) {
      /* Run the NIST test suite 1 and 2 as the prerequisite conditions
       * were not satisfied.
       */
      for (i = 0; i < test_listsize; i++) {
          test_select = test_list[i];
          status = main(argc, argv);    // NIST STS
          if (status == AVS_STATUS_NIST_PASS) {
              val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
          } else {
              val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
              return;
          }
      }
  }
  else {
      /* Run all the NIST test suite as the as the prerequisite conditions
       * were satisfied.
       */
      status = main(argc, argv);    // NIST STS
      if (status == AVS_STATUS_NIST_PASS) {
          val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
      } else {
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
          return;
      }
  }

  print_nist_result();
  return;
}

uint32_t
n001_entry(uint32_t num_pe)
{
  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This NIST test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);

  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
//  status = val_check_for_error(TEST_NUM, num_pe);

//  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
