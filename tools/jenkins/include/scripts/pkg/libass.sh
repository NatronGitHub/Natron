#!/bin/bash

# Install libass (for ffmpeg)
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libass.html
LIBASS_VERSION=0.17.5
LIBASS_TAR="libass-${LIBASS_VERSION}.tar.xz"
LIBASS_SHA256=2dca25c0e0c837ddf00b52011b3f82cac1e4ddd3ad018227806b0c2288864acc
LIBASS_SITE="https://github.com/libass/libass/releases/download/${LIBASS_VERSION}"
if download_step; then
    download "$LIBASS_SITE" "$LIBASS_TAR" "$LIBASS_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libass.pc" ] || [ "$(pkg-config --modversion libass)" != "$LIBASS_VERSION" ]; }; }; then
    REBUILD_FFMPEG=1
    start_build
    untar "$SRC_PATH/$LIBASS_TAR"
    pushd "libass-${LIBASS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libass-${LIBASS_VERSION}"
    end_build
fi
