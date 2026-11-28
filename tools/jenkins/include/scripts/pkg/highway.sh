#!/bin/bash

# Install highway (for libjxl)
# see https://github.com/google/highway/releases
HWY_VERSION=1.4.0
HWY_TAR="highway-${HWY_VERSION}.tar.gz"
HWY_SHA256=36f672ab48ddb3c8555e9e89e16fe400cd7d16c6eb455a1a3d0c146a63ababdc
HWY_SITE="https://github.com/google/highway/releases/download/${HWY_VERSION}"
if download_step; then
    download "$HWY_SITE" "$HWY_TAR" "$HWY_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libhwy.pc" ] || [ "$(pkg-config --modversion libhwy)" != "$HWY_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$HWY_TAR"
    pushd "highway-${HWY_VERSION}"
    cmake -S . -B _build -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF \
        -DHWY_ENABLE_TESTS=OFF -DHWY_ENABLE_EXAMPLES=OFF
    cmake --build _build -j${MKJOBS} --target install
    popd
    rm -rf "highway-${HWY_VERSION}"
    end_build
fi
