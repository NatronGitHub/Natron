#!/bin/bash

# install soxr
# see https://sourceforge.net/projects/soxr/files
SOXR_VERSION=0.1.3
SOXR_TAR="soxr-${SOXR_VERSION}-Source.tar.xz"
SOXR_SITE="https://sourceforge.net/projects/soxr/files"
if download_step; then
    download "$SOXR_SITE" "$SOXR_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/soxr.pc" ] || [ "$(pkg-config --modversion soxr)" != "$SOXR_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$SOXR_TAR"
    pushd "soxr-${SOXR_VERSION}-Source"
    mkdir tmp_build
    pushd tmp_build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBUILD_TESTS=no -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "soxr-${SOXR_VERSION}-Source"
    end_build
fi
