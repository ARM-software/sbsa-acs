## @file
 # Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 # SPDX-License-Identifier : Apache-2.0
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #  http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 ##

import sys
import os
from os import walk
import warnings

IGNORE_FILE_TYPES = ['', '.inf', '.mk', '.md', '.cmake', 'json', '.dts', '.h']

def compile_check(COMPILED_FILE, ROOT_DIR, TARGET):

    VAL_PATH = os.path.join(ROOT_DIR,'val')
    PLAT_COMMON = os.path.join(ROOT_DIR,'platform', 'pal_baremetal', "common")
    PLAT_PATH = os.path.join(ROOT_DIR,'platform', 'pal_baremetal', TARGET)
    TEST_PATH = os.path.join(ROOT_DIR,'test_pool')

    global_filelist = []

    for val, val_dir, val_files in os.walk(VAL_PATH):
        for file in val_files:
            if (os.path.splitext(file)[1] in IGNORE_FILE_TYPES):
                continue
            else:
                global_filelist.append(os.path.join(val,file))

    for plat, plat_dir, plat_files in os.walk(PLAT_COMMON):
        for file in plat_files:
                if (os.path.splitext(file)[1] in IGNORE_FILE_TYPES):
                        continue
                else:
                        global_filelist.append(os.path.join(plat,file))

    for plat, plat_dir, plat_files in os.walk(PLAT_PATH):
        for file in plat_files:
                if (os.path.splitext(file)[1] in IGNORE_FILE_TYPES):
                        continue
                else:
                        global_filelist.append(os.path.join(plat,file))

    for test, test_dir, test_files in os.walk(TEST_PATH):
        for file in test_files:
                if (os.path.splitext(file)[1] in IGNORE_FILE_TYPES):
                        continue
                else:
                        global_filelist.append(os.path.join(test,file))

    not_compiled_files = [not_compiled for not_compiled in global_filelist
                                       if not_compiled not in COMPILED_FILE]
    for i in not_compiled_files:
        print(i)


if __name__ == "__main__":
    COMPILED_FILE = sys.argv[1]
    ROOT_DIR = sys.argv[2]
    TARGET = sys.argv[3]

    compile_check(COMPILED_FILE, ROOT_DIR, TARGET)
