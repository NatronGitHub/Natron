#!/bin/bash

# Install gperf (used by fontconfig) as well as assemblers (yasm and nasm)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/gperf.html
GPERF_VERSION=3.1
GPERF_TAR="gperf-${GPERF_VERSION}.tar.gz"
GPERF_SITE="http://ftp.gnu.org/pub/gnu/gperf"
if download_step; then
    download "$GPERF_SITE" "$GPERF_TAR"
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
