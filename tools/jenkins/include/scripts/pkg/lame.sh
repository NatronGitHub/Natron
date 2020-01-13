#!/bin/bash

# Install lame
# see http://www.linuxfromscratch.org/blfs/view/cvs/multimedia/lame.html
LAME_VERSION=3.100
#LAME_VERSION_SHORT=${LAME_VERSION%.*}
LAME_TAR="lame-${LAME_VERSION}.tar.gz"
LAME_SITE="https://sourceforge.net/projects/lame/files/lame/${LAME_VERSION}"
if download_step; then
    download "$LAME_SITE" "$LAME_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libmp3lame.so" ] || [ "$("$SDK_HOME/bin/lame" --version |head -1 | sed -e 's/^.*[Vv]ersion \([^ ]*\) .*$/\1/')" != "$LAME_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LAME_TAR"
    pushd "lame-${LAME_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-mp3rtp --enable-shared --disable-static --enable-nasm
    make -j${MKJOBS}
    make install
    popd
    rm -rf "lame-${LAME_VERSION}"
    end_build
fi
