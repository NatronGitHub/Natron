#!/bin/bash

# Install webp (for OIIO)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/libwebp.html
WEBP_VERSION=1.0.3
WEBP_TAR="libwebp-${WEBP_VERSION}.tar.gz"
WEBP_SITE="https://storage.googleapis.com/downloads.webmproject.org/releases/webp"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libwebp.pc" ] || [ "$(pkg-config --modversion libwebp)" != "$WEBP_VERSION" ]; }; }; then
    start_build
    download "$WEBP_SITE" "$WEBP_TAR"
    untar "$SRC_PATH/$WEBP_TAR"
    pushd "libwebp-${WEBP_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --enable-libwebpmux --enable-libwebpdemux --enable-libwebpdecoder --enable-libwebpextras --disable-silent-rules --enable-swap-16bit-csp
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libwebp-${WEBP_VERSION}"
    end_build
fi
