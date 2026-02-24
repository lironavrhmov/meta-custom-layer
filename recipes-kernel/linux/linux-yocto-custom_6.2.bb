SUMMARY = "Custom Linux Kernel 6.2"
SECTION = "kernel"
LICENSE = "CLOSED"

inherit kernel kernel-yocto

SRC_URI = "git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git;protocol=https;branch=linux-6.2.y \
           file://defconfig \
           file://cma.cfg"

SRCREV = "${AUTOREV}"

LINUX_VERSION = "6.2"
LINUX_VERSION_EXTENSION = "-custom"

PV = "${LINUX_VERSION}+git${SRCPV}"

COMPATIBLE_MACHINE = "(qemux86-64|qemuarm64)"

KCONFIG_MODE = "--alldefconfig"

S = "${WORKDIR}/git"

DEPENDS += "elfutils-native openssl-native util-linux-native"

KERNEL_VERSION_SANITY_SKIP = "1"
