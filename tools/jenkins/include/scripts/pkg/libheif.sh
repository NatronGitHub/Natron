#!/bin/bash

# Install libheif
# see https://github.com/strukturag/libheif/releases
LIBHEIF_VERSION=1.6.2
LIBHEIF_TAR="libheif-${LIBHEIF_VERSION}.tar.gz"
LIBHEIF_SITE="https://github.com/strukturag/libheif/releases/download/v${LIBHEIF_VERSION}"
if download_step; then
    download "$LIBHEIF_SITE" "$LIBHEIF_TAR"
    #download_github strukturag libheif "$LIBHEIF_VERSION" v "$LIBHEIF_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libheif.pc" ] || [ "$(pkg-config --modversion libheif)" != "$LIBHEIF_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBHEIF_TAR"
    pushd "libheif-${LIBHEIF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libheif-${LIBHEIF_VERSION}"
    end_build
fi
