#!/bin/bash

# Install libuv (for cmake)
# http://www.linuxfromscratch.org/blfs/view/svn/general/libuv.html
LIBUV_VERSION=1.38.0
LIBUV_TAR="libuv-${LIBUV_VERSION}.tar.gz"
#LIBUV_SITE="http://www.libuv.org/downloads"
LIBUV_SITE="https://dist.libuv.org/dist/v${LIBUV_VERSION}"
if download_step; then
    download_github libuv libuv "${LIBUV_VERSION}" v "${LIBUV_TAR}"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libuv.pc" ] || [ "$(pkg-config --modversion libuv)" != "$LIBUV_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBUV_TAR"
    pushd "libuv-${LIBUV_VERSION}"
    ./autogen.sh
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libuv-${LIBUV_VERSION}"
    end_build
fi
