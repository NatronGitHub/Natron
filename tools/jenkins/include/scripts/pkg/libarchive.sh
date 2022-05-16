#!/bin/bash

# Install libarchive (for cmake)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libarchive.html
LIBARCHIVE_VERSION=3.6.1
LIBARCHIVE_TAR="libarchive-${LIBARCHIVE_VERSION}.tar.gz"
#LIBARCHIVE_SITE="http://www.libarchive.org/downloads" # up to 3.3.3
LIBARCHIVE_SITE="https://github.com/libarchive/libarchive/releases/download/v${LIBARCHIVE_VERSION}"
if download_step; then
    download "$LIBARCHIVE_SITE" "$LIBARCHIVE_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libarchive.pc" ] || [ "$(pkg-config --modversion libarchive)" != "$LIBARCHIVE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBARCHIVE_TAR"
    pushd "libarchive-${LIBARCHIVE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libarchive-${LIBARCHIVE_VERSION}"
    end_build
fi
