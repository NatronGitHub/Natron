#!/bin/bash

# Install m4
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/m4.html
M4_VERSION=1.4.19
M4_TAR="m4-${M4_VERSION}.tar.xz"
M4_SITE="https://ftp.gnu.org/gnu/m4"
if download_step; then
    download "$M4_SITE" "$M4_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/m4" ]; }; }; then
    start_build
    untar "$SRC_PATH/$M4_TAR"
    pushd "m4-${M4_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "m4-${M4_VERSION}"
    end_build
fi
