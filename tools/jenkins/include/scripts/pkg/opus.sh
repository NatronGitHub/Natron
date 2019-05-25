#!/bin/bash

# Install opus
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/opus.html
OPUS_VERSION=1.3.1
OPUS_TAR="opus-${OPUS_VERSION}.tar.gz"
OPUS_SITE="https://archive.mozilla.org/pub/opus"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/opus.pc" ] || [ "$(pkg-config --modversion opus)" != "$OPUS_VERSION" ]; }; }; then
    start_build
    download "$OPUS_SITE" "$OPUS_TAR"
    untar "$SRC_PATH/$OPUS_TAR"
    pushd "opus-$OPUS_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static --enable-custom-modes
    make -j${MKJOBS}
    make install
    popd
    rm -rf "opus-$OPUS_VERSION"
    end_build
fi
