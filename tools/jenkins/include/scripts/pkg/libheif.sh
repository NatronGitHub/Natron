#!/bin/bash

# Install libheif
# see https://github.com/strukturag/libheif/releases
LIBHEIF_VERSION=1.16.1
LIBHEIF_TAR="libheif-${LIBHEIF_VERSION}.tar.gz"
LIBHEIF_SITE="https://github.com/strukturag/libheif/releases/download/v${LIBHEIF_VERSION}"
if download_step; then
    download "$LIBHEIF_SITE" "$LIBHEIF_TAR"
    #download_github strukturag libheif "$LIBHEIF_VERSION" v "$LIBHEIF_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libheif.pc" ] || [ "$(pkg-config --modversion libheif)" != "$LIBHEIF_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBHEIF_TAR"
    pushd "libheif-${LIBHEIF_VERSION}"
    mkdir _build
    pushd _build
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  \
     -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DWITH_GDK_PIXBUF=OFF -DWITH_EXAMPLES=OFF -DBUILD_SHARED_LIBS=on
    #env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "libheif-${LIBHEIF_VERSION}"
    end_build
fi
