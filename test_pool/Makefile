## @file
 # Copyright (c) 2016-2018, Arm Limited or its affiliates. All rights reserved.
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

export CROSS_COMPILE ?= aarch64-linux-gnu-
export ARCH ?= arm64

#since we have copied the files locally
ACS_DIR ?= .

TEST_POOL = $(ACS_DIR)/

obj-m += sbsa_acs_test.o
sbsa_acs_test-objs += $(TEST_POOL)/pcie/test_p001.o \
    $(TEST_POOL)/pcie/test_p002.o \
    $(TEST_POOL)/pcie/test_p003.o $(TEST_POOL)/pcie/test_p004.o \
    $(TEST_POOL)/pcie/test_p005.o $(TEST_POOL)/pcie/test_p006.o \
    $(TEST_POOL)/pcie/test_p007.o $(TEST_POOL)/pcie/test_p008.o \
    $(TEST_POOL)/pcie/test_p009.o $(TEST_POOL)/pcie/test_p010.o \
    $(TEST_POOL)/pcie/test_p011.o $(TEST_POOL)/pcie/test_p012.o \
    $(TEST_POOL)/pcie/test_p013.o $(TEST_POOL)/pcie/test_p014.o \
    $(TEST_POOL)/pcie/test_p015.o $(TEST_POOL)/pcie/test_p016.o \
    $(TEST_POOL)/pcie/test_p017.o $(TEST_POOL)/pcie/test_p018.o \
    $(TEST_POOL)/pcie/test_p019.o \
    $(TEST_POOL)/exerciser/test_e001.o $(TEST_POOL)/exerciser/test_e002.o \
    $(TEST_POOL)/exerciser/test_e003.o $(TEST_POOL)/exerciser/test_e004.o \
    $(TEST_POOL)/exerciser/test_e005.o $(TEST_POOL)/exerciser/test_e006.o \
    $(TEST_POOL)/exerciser/test_e007.o

ccflags-y=-I$(PWD)/$(ACS_DIR)/.. -I$(PWD)/$(ACS_DIR)/val/ -I$(PWD)/$(ACS_DIR)/ -DTARGET_LINUX -Wall -Werror

all:
ifeq ($(KERNEL_SRC),)
	echo "	KERNEL_SRC variable should be set to kernel path "
	exit 1
else
	echo "Kernel source is set to $(KERNEL_SRC)"
endif

	make -C $(KERNEL_SRC) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules_install

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean
