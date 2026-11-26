#!/bin/bash

# Install libvorbis
# http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libvorbis.html
LIBVORBIS_VERSION=1.3.7
LIBVORBIS_TAR="libvorbis-${LIBVORBIS_VERSION}.tar.gz"
LIBVORBIS_SHA256=0e982409a9c3fc82ee06e08205b1355e5c6aa4c36bca58146ef399621b0ce5ab
LIBVORBIS_SITE="https://downloads.xiph.org/releases/vorbis"
if download_step; then
    download "$LIBVORBIS_SITE" "$LIBVORBIS_TAR" "$LIBVORBIS_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/vorbis.pc" ] || [ "$(pkg-config --modversion vorbis)" != "$LIBVORBIS_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBVORBIS_TAR"
    pushd "libvorbis-${LIBVORBIS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libvorbis-${LIBVORBIS_VERSION}"
    end_build
fi
