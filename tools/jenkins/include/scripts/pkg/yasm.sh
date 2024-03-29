#!/bin/bash

# Install yasm
# see http://www.linuxfromscratch.org/blfs/view/svn/general/yasm.html
YASM_VERSION=1.3.0
YASM_TAR="yasm-${YASM_VERSION}.tar.gz"
YASM_SITE="http://www.tortall.net/projects/yasm/releases"
if download_step; then
    download "$YASM_SITE" "$YASM_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/yasm" ]; }; }; then
    start_build
    untar "$SRC_PATH/$YASM_TAR"
    pushd "yasm-${YASM_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "yasm-${YASM_VERSION}"
    end_build
fi
