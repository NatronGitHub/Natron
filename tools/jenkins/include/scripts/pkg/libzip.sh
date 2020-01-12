#!/bin/bash

# Install libzip (requires cmake)
# see https://libzip.org/download/
ZIP_VERSION=1.5.2
ZIP_TAR="libzip-${ZIP_VERSION}.tar.xz"
ZIP_SITE="https://libzip.org/download"
if download_step; then
    download "$ZIP_SITE" "$ZIP_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libzip.pc" ] || [ "$(pkg-config --modversion libzip)" != "$ZIP_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$ZIP_TAR"
    pushd "libzip-${ZIP_VERSION}"
    mkdir build-natron
    pushd build-natron
    #env CFLAGS="$BF" CXXFLAGS="$BF" ../configure --prefix="$SDK_HOME" --disable-static --enable-shared
    # libzip adds -I$SDK_HOME/include before its own includes, and thus includes the installed zip.h
    if [ -f "$SDK_HOME/include/zip.h" ]; then
        rm  "$SDK_HOME/include/zip.h"
    fi
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "libzip-${ZIP_VERSION}"
    end_build
fi
