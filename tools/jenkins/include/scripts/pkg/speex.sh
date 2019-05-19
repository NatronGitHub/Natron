#!/bin/bash

# Install speex (EOL, use opus)
# see http://www.linuxfromscratch.org/blfs/view/stable/multimedia/speex.html
SPEEX_VERSION=1.2.0
SPEEX_TAR="speex-${SPEEX_VERSION}.tar.gz"
SPEEX_SITE="http://downloads.us.xiph.org/releases/speex"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/speex.pc" ] || [ "$(pkg-config --modversion speex)" != "$SPEEX_VERSION" ]; }; }; then
    start_build
    download "$SPEEX_SITE" "$SPEEX_TAR"
    untar "$SRC_PATH/$SPEEX_TAR"
    pushd "speex-$SPEEX_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "speex-$SPEEX_VERSION"
    end_build
fi
