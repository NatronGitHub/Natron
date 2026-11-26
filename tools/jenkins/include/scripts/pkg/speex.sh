#!/bin/bash

# Install speex (EOL, use opus)
# see http://www.linuxfromscratch.org/blfs/view/stable/multimedia/speex.html
SPEEX_VERSION=1.2.1
SPEEX_TAR="speex-${SPEEX_VERSION}.tar.gz"
SPEEX_SHA256=4b44d4f2b38a370a2d98a78329fefc56a0cf93d1c1be70029217baae6628feea
SPEEX_SITE="https://downloads.xiph.org/releases/speex"
if download_step; then
    download "$SPEEX_SITE" "$SPEEX_TAR" "$SPEEX_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/speex.pc" ] || [ "$(pkg-config --modversion speex)" != "$SPEEX_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$SPEEX_TAR"
    pushd "speex-$SPEEX_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "speex-$SPEEX_VERSION"
    end_build
fi
