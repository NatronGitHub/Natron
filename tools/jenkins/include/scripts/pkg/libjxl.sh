#!/bin/bash

# Install libjxl (JPEG XL, for OIIO)
# see https://github.com/libjxl/libjxl/releases
# The release archives have no third_party submodules: brotli and highway come from the SDK
# (brotli.sh, highway.sh), the colour management from its Little CMS (lcms2.sh) instead of skcms.
LIBJXL_VERSION=0.13.0
LIBJXL_TAR="libjxl-${LIBJXL_VERSION}.tar.gz"
LIBJXL_SHA256=24d18c9af2d1f5a17730b3f8d3f70a5c16e5ac303f5d9d5fe9dc00cf7ca82af2
if download_step; then
    download_github libjxl libjxl "$LIBJXL_VERSION" v "$LIBJXL_TAR" "$LIBJXL_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libjxl.pc" ] || [ "$(pkg-config --modversion libjxl)" != "$LIBJXL_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBJXL_TAR"
    pushd "libjxl-${LIBJXL_VERSION}"
    cmake -S . -B _build -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF \
        -DJPEGXL_FORCE_SYSTEM_BROTLI=ON -DJPEGXL_FORCE_SYSTEM_HWY=ON -DJPEGXL_FORCE_SYSTEM_LCMS2=ON -DJPEGXL_ENABLE_SKCMS=OFF \
        -DJPEGXL_ENABLE_TOOLS=OFF -DJPEGXL_ENABLE_DEVTOOLS=OFF -DJPEGXL_ENABLE_BENCHMARK=OFF -DJPEGXL_ENABLE_EXAMPLES=OFF \
        -DJPEGXL_ENABLE_DOXYGEN=OFF -DJPEGXL_ENABLE_MANPAGES=OFF -DJPEGXL_ENABLE_JNI=OFF -DJPEGXL_ENABLE_SJPEG=OFF \
        -DJPEGXL_ENABLE_OPENEXR=OFF -DJPEGXL_ENABLE_PLUGINS=OFF -DJPEGXL_ENABLE_VIEWERS=OFF -DJPEGXL_ENABLE_FUZZERS=OFF \
        -DJPEGXL_ENABLE_TCMALLOC=OFF -DJPEGXL_BUNDLE_LIBPNG=OFF
    cmake --build _build -j${MKJOBS} --target install
    popd
    rm -rf "libjxl-${LIBJXL_VERSION}"
    end_build
fi
