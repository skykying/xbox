FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

BRANCH = "master"
BRANCH_cetibox = "v4.14/rcar-3.7-ctc"
SRCREV = "${AUTOREV}"
LINUX_VERSION = "4.14.75"

SRC_URI = " \
    git://github.com/xen-troops/linux.git;branch=${BRANCH} \
    file://defconfig \
  "

do_deploy_append () {
    find ${D}/boot -iname "vmlinux*" -exec tar -cJvf ${STAGING_KERNEL_BUILDDIR}/vmlinux.tar.xz {} \;
}
