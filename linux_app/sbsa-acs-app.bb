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
S = "${WORKDIR}"

do_compile() {
	     ${CC} sbsa_app_main.c sbsa_app_pcie.c sbsa_drv_intf.c -Iinclude -o sbsa
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 sbsa ${D}${bindir}
}
