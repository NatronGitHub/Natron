#!/bin/bash

# Install intltool
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/intltool.html
INTLTOOL_VERSION=0.51.0
INTLTOOL_TAR="intltool-${INTLTOOL_VERSION}.tar.gz"
INTLTOOL_SHA256=67c74d94196b153b774ab9f89b2fa6c6ba79352407037c8c14d5aeb334e959cd
INTLTOOL_SITE="https://launchpad.net/intltool/trunk/${INTLTOOL_VERSION}/+download"
if download_step; then
    download "$INTLTOOL_SITE" "$INTLTOOL_TAR" "$INTLTOOL_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/intltoolize" ]; }; }; then
    start_build
    untar "$SRC_PATH/$INTLTOOL_TAR"
    pushd "intltool-${INTLTOOL_VERSION}"
    patch -p1 -i "$INC_PATH/patches/intltool-0.51.0-perl-5.22-compatibility.patch"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "intltool-${INTLTOOL_VERSION}"
    end_build
fi
