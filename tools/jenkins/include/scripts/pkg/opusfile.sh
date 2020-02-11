#!/bin/bash

# Install opusfile
# see https://github.com/xiph/opusfile/releases
OPUSFILE_VERSION=0.11
OPUSFILE_TAR="opusfile-${OPUSFILE_VERSION}.tar.gz"
OPUSFILE_SITE="https://archive.mozilla.org/pub/opus"
if download_step; then
    download "$OPUSFILE_SITE" "$OPUSFILE_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/opusfile.pc" ] || [ "$(pkg-config --modversion opusfile)" != "$OPUSFILE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$OPUSFILE_TAR"
    pushd "opusfile-$OPUSFILE_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "opusfile-$OPUSFILE_VERSION"
    end_build
fi
