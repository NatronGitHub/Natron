#!/bin/bash

# Install libde265
# see https://github.com/strukturag/libde265/releases
LIBDE265_VERSION=1.0.4
LIBDE265_TAR="libde265-${LIBDE265_VERSION}.tar.gz"
LIBDE265_SITE="https://github.com/strukturag/libde265/releases/download/v${LIBDE265_VERSION}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libde265.pc" ] || [ "$(pkg-config --modversion libde265)" != "$LIBDE265_VERSION" ]; }; }; then
    start_build
    download "$LIBDE265_SITE" "$LIBDE265_TAR"
    #download_github strukturag libde265 "$LIBDE265_VERSION" v "$LIBDE265_TAR"
    untar "$SRC_PATH/$LIBDE265_TAR"
    pushd "libde265-${LIBDE265_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libde265-${LIBDE265_VERSION}"
    end_build
fi
