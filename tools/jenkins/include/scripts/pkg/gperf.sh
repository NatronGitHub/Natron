#!/bin/bash

# Install gperf (used by fontconfig) as well as assemblers (yasm and nasm)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/gperf.html
GPERF_VERSION=3.3
GPERF_TAR="gperf-${GPERF_VERSION}.tar.gz"
GPERF_SHA256=fd87e0aba7e43ae054837afd6cd4db03a3f2693deb3619085e6ed9d8d9604ad8
GPERF_SITE="https://ftp.gnu.org/pub/gnu/gperf"
if download_step; then
    download "$GPERF_SITE" "$GPERF_TAR" "$GPERF_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/gperf" ]; }; }; then
    start_build
    untar "$SRC_PATH/$GPERF_TAR"
    pushd "gperf-${GPERF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gperf-${GPERF_VERSION}"
    end_build
fi
