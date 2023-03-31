 # #!/usr/bin/env bash

 # @file
 # Copyright (c) 2023 Arm Limited or its affiliates. All rights reserved.
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

arch=$(uname -m)
echo $arch
if [[ $arch = "aarch64" ]]
then
    echo "aarch64 native build"
    export CROSS_COMPILE=''
elif [ -v $CROSS_COMPILE ]
then
    echo "CROSS_COMPILE is not set"
    echo "set using export CROSS_COMPILE=<lib_path>/bin/aarch64-none-linux-gnu-"
    return 0
fi

export CC=${CROSS_COMPILE}gcc
echo "Building Linux MTE test..."
echo "CC = $CC"

$CC -o mte_test -O0 -march=armv8.5-a+memtag test_mte.c

unset CC
unset CROSS_COMPILE
