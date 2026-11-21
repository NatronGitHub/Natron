#!/bin/bash

# Install zlib
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/zlib.html
ZLIB_VERSION=1.3.2
ZLIB_TAR="zlib-${ZLIB_VERSION}.tar.gz"
ZLIB_SHA256=bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16
ZLIB_SITE="https://github.com/madler/zlib/releases/download/v${ZLIB_VERSION}"
if download_step; then
    download "$ZLIB_SITE" "$ZLIB_TAR" "$ZLIB_SHA256"
fi
if build_step && { force_build || { [ "${REBUILD_ZLIB:-}" = "1" ]; }; }; then
    rm -rf $SDK_HOME/lib/libz.* || true
    rm -f $SDK_HOME/lib/pkgconfig/zlib.pc || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libz.so.1" ]; }; }; then
    start_build
    untar "$SRC_PATH/$ZLIB_TAR"
    pushd "zlib-${ZLIB_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "zlib-${ZLIB_VERSION}"
    end_build
fi

