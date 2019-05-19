#!/bin/bash

# Install libvorbis
# http://www.linuxfromscratch.org/blfs/view/cvs/multimedia/libvorbis.html
LIBVORBIS_VERSION=1.3.6
LIBVORBIS_TAR="libvorbis-${LIBVORBIS_VERSION}.tar.gz"
LIBVORBIS_SITE="http://downloads.xiph.org/releases/vorbis"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/vorbis.pc" ] || [ "$(pkg-config --modversion vorbis)" != "$LIBVORBIS_VERSION" ]; }; }; then
    start_build
    download "$LIBVORBIS_SITE" "$LIBVORBIS_TAR"
    untar "$SRC_PATH/$LIBVORBIS_TAR"
    pushd "libvorbis-${LIBVORBIS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libvorbis-${LIBVORBIS_VERSION}"
    end_build
fi
