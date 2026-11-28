#!/bin/bash

# Install brotli (for libjxl)
# see https://github.com/google/brotli/releases
BROTLI_VERSION=1.2.1
BROTLI_TAR="brotli-${BROTLI_VERSION}.tar.gz"
BROTLI_SHA256=98d35d11a78dc453aebb7d10c3748f3f73ff78c8267c0dc697b8631f67bf7e8f
if download_step; then
    download_github google brotli "$BROTLI_VERSION" v "$BROTLI_TAR" "$BROTLI_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libbrotlidec.pc" ] || [ "$(pkg-config --modversion libbrotlidec)" != "$BROTLI_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$BROTLI_TAR"
    pushd "brotli-${BROTLI_VERSION}"
    cmake -S . -B _build -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DCMAKE_C_FLAGS="$BF" -DBUILD_SHARED_LIBS=ON -DBROTLI_BUILD_TOOLS=OFF -DBROTLI_DISABLE_TESTS=ON
    cmake --build _build -j${MKJOBS} --target install
    popd
    rm -rf "brotli-${BROTLI_VERSION}"
    end_build
fi
