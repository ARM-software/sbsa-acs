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

# yocto-bsp-filename {{=example_recipe_name}}_0.1.bb
#
# This file was derived from the 'Hello World!' example recipe in the
# Yocto Project Development Manual.
#

SUMMARY = "SBSA ACS commandline application"
SECTION = "meta-luv"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "file://sbsa_app_main.c \
           file://sbsa_app_pcie.c \
           file://sbsa_drv_intf.c \
           file://include/sbsa_drv_intf.h \
           file://include/sbsa_app.h \
           file://include/sbsa_avs_common.h \
           "
SRC_URI[md5sum] = "3bff44b2755c130da1c74fbf2a0223d5"

S = "${WORKDIR}"

do_compile() {
	   ${CC} sbsa_app_main.c sbsa_app_pcie.c sbsa_drv_intf.c -Iinclude -o sbsa
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 sbsa ${D}${bindir}
}
