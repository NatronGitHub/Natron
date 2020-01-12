#!/bin/bash

# Install libass (for ffmpeg)
# see http://www.linuxfromscratch.org/blfs/view/cvs/multimedia/libass.html
LIBASS_VERSION=0.14.0
LIBASS_TAR="libass-${LIBASS_VERSION}.tar.xz"
LIBASS_SITE="https://github.com/libass/libass/releases/download/${LIBASS_VERSION}"
if download_step; then
    download "$LIBASS_SITE" "$LIBASS_TAR"
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
