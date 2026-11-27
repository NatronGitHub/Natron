#!/bin/bash

# Install googletest (Google Test only, no Google Mock), for Natron's Tests program
# (Tests/CMakeLists.txt: find_package(GTest 1.14 CONFIG), required since #1910).
# A static library: Tests links it in, so build-Linux-installer.sh has no libgtest.so to bundle.
# PIC, since Tests may be linked as a position-independent executable.
# see https://github.com/google/googletest/releases
GOOGLETEST_VERSION=1.18.0
GOOGLETEST_TAR="googletest-${GOOGLETEST_VERSION}.tar.gz"
GOOGLETEST_SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
GOOGLETEST_SITE="https://github.com/google/googletest/releases/download/v${GOOGLETEST_VERSION}"
if download_step; then
    download "$GOOGLETEST_SITE" "$GOOGLETEST_TAR" "$GOOGLETEST_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/gtest.pc" ] || [ "$(pkg-config --modversion gtest)" != "$GOOGLETEST_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$GOOGLETEST_TAR"
    pushd "googletest-${GOOGLETEST_VERSION}"
    mkdir build
    pushd build
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" \
          -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
          -DBUILD_GMOCK=OFF -DINSTALL_GTEST=ON
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "googletest-${GOOGLETEST_VERSION}"
    end_build
fi
