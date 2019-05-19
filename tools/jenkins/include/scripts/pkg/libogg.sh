#!/bin/bash

# Install libogg
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libogg.html
LIBOGG_VERSION=1.3.3
LIBOGG_TAR="libogg-${LIBOGG_VERSION}.tar.gz"
LIBOGG_SITE="http://downloads.xiph.org/releases/ogg"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/ogg.pc" ] || [ "$(pkg-config --modversion ogg)" != "$LIBOGG_VERSION" ]; }; }; then
    start_build
    download "$LIBOGG_SITE" "$LIBOGG_TAR"
    untar "$SRC_PATH/$LIBOGG_TAR"
    pushd "libogg-${LIBOGG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libogg-${LIBOGG_VERSION}"
    end_build
fi
