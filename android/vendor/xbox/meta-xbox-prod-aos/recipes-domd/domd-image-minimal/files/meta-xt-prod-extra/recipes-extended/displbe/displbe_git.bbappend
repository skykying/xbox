################################################################################
# Renesas R-Car
################################################################################
SRCREV_rcar = "b5d07a43ba56380f2da8f47d7d106afbde0f2a3e"

SRC_URI_append_rcar = " git://github.com/xen-troops/displ_be.git;protocol=https;branch=master"

DEPENDS += " wayland-ivi-extension wayland-native"

EXTRA_OECMAKE_append_rcar = " -DWITH_DOC=OFF -DWITH_DRM=ON -DWITH_ZCOPY=ON -DWITH_WAYLAND=ON -DWITH_IVI_EXTENSION=ON -DWITH_INPUT=ON"
