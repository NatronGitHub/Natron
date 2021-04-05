#!/bin/bash

# install libtool
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/libtool.html
LIBTOOL_VERSION=2.4.6
LIBTOOL_TAR="libtool-${LIBTOOL_VERSION}.tar.gz"
LIBTOOL_SITE="http://ftp.gnu.org/pub/gnu/libtool"
if download_step; then
    download "$LIBTOOL_SITE" "$LIBTOOL_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/libtool" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBTOOL_TAR"
    pushd "libtool-${LIBTOOL_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libtool-${LIBTOOL_VERSION}"
    end_build
fi
