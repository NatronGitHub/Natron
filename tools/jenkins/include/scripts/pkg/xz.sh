#!/bin/bash

# Install xz (required to uncompress source tarballs)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/xz.html
XZ_VERSION=5.2.4
XZ_TAR="xz-${XZ_VERSION}.tar.bz2"
XZ_SITE="https://tukaani.org/xz"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/liblzma.pc" ] || [ "$(pkg-config --modversion liblzma)" != "$XZ_VERSION" ]; }; }; then
    start_build
    download "$XZ_SITE" "$XZ_TAR"
    untar "$SRC_PATH/$XZ_TAR"
    pushd "xz-$XZ_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "xz-$XZ_VERSION"
    end_build
fi

