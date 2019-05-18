#!/bin/bash

# Install libunistring (for gnutls)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libunistring.html
LIBUNISTRING_VERSION=0.9.10
LIBUNISTRING_VERSION_HEX=0x00090A
#LIBUNISTRING_VERSION_SHORT=${LIBUNISTRING_VERSION%.*}
LIBUNISTRING_TAR="libunistring-${LIBUNISTRING_VERSION}.tar.gz"
LIBUNISTRING_SITE="https://ftp.gnu.org/gnu/libunistring"
if build_step && { force_build || { [ ! -s "$SDK_HOME/include/unistring/version.h" ] || ! fgrep "${LIBUNISTRING_VERSION_HEX}" "$SDK_HOME/include/unistring/version.h" &>/dev/null; }; }; then
    start_build
    download "$LIBUNISTRING_SITE" "$LIBUNISTRING_TAR"
    untar "$SRC_PATH/$LIBUNISTRING_TAR"
    pushd "libunistring-${LIBUNISTRING_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libunistring-${LIBUNISTRING_VERSION}"
    end_build
fi
