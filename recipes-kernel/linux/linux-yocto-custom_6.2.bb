SUMMARY = "Custom Linux Kernel 6.2"
SECTION = "kernel"
LICENSE = "CLOSED"

inherit kernel

SRC_URI = "git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git;protocol=https;branch=linux-6.2.y \
           file://defconfig"
SRCREV = "${AUTOREV}"

LINUX_VERSION = "6.2"
LINUX_VERSION_EXTENSION = "-custom"

PV = "${LINUX_VERSION}+git${SRCPV}"

COMPATIBLE_MACHINE = "(qemux86-64|qemuarm64)"

# Set the source directory explicitly
S = "${WORKDIR}/git"

# Add dependency for objtool
DEPENDS += "elfutils-native"

# Add other common dependencies for modern kernels
DEPENDS += "openssl-native util-linux-native"