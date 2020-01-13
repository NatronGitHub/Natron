#!/bin/bash

# Install libtheora
# see http://www.linuxfromscratch.org/blfs/view/cvs/multimedia/libtheora.html
LIBTHEORA_VERSION=1.1.1
LIBTHEORA_TAR="libtheora-${LIBTHEORA_VERSION}.tar.bz2"
LIBTHEORA_SITE="http://downloads.xiph.org/releases/theora"
if download_step; then
    download "$LIBTHEORA_SITE" "$LIBTHEORA_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/theora.pc" ] || [ "$(pkg-config --modversion theora)" != "$LIBTHEORA_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBTHEORA_TAR"
    pushd "libtheora-${LIBTHEORA_VERSION}"
    $GSED -i 's/png_\(sizeof\)/\1/g' examples/png2theora.c # fix libpng16
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libtheora-${LIBTHEORA_VERSION}"
    end_build
fi
