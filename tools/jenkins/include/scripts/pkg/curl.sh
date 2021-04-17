#!/bin/bash

# Install curl
# see http://www.linuxfromscratch.org/blfs/view/cvs/basicnet/curl.html
CURL_VERSION=7.76.0
CURL_TAR="curl-${CURL_VERSION}.tar.bz2"
CURL_SITE="https://curl.haxx.se/download"
if download_step; then
    download "$CURL_SITE" "$CURL_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libcurl.pc" ] || [ "$(pkg-config --modversion libcurl)" != "$CURL_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$CURL_TAR"
    pushd "curl-${CURL_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --enable-threaded-resolver --prefix="$SDK_HOME" --with-ssl="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "curl-${CURL_VERSION}"
    end_build
fi
