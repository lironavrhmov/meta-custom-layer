SUMMARY = "Universal Boot Loader for embedded devices"
HOMEPAGE = "http://www.denx.de/wiki/U-Boot/WebHome"
SECTION = "bootloaders"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=2ca5f251d9c9f74394da28b659a39ca5"
require recipes-bsp/u-boot/u-boot-common.inc
require recipes-bsp/u-boot/u-boot.inc
PV = "2022.04"
SRC_URI = "https://ftp.denx.de/pub/u-boot/u-boot-${PV}.tar.bz2"
SRC_URI[sha256sum] = "68e065413926778e276ec3abd28bb32fa82abaa4a6898d570c1f48fbdb08bcd0"
S = "${WORKDIR}/u-boot-${PV}"
DEPENDS += "python3-setuptools-native"