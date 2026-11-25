#!/bin/bash

# Install libogg
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libogg.html
LIBOGG_VERSION=1.3.6
LIBOGG_TAR="libogg-${LIBOGG_VERSION}.tar.gz"
LIBOGG_SHA256=83e6704730683d004d20e21b8f7f55dcb3383cdf84c0daedf30bde175f774638
LIBOGG_SITE="https://downloads.xiph.org/releases/ogg"
if download_step; then
    download "$LIBOGG_SITE" "$LIBOGG_TAR" "$LIBOGG_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/ogg.pc" ] || [ "$(pkg-config --modversion ogg)" != "$LIBOGG_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBOGG_TAR"
    pushd "libogg-${LIBOGG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libogg-${LIBOGG_VERSION}"
    end_build
fi
