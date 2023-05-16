#!/bin/bash

# Install dav1d AV1 decoder
# see https://code.videolan.org/videolan/dav1d/-/releases
DAV1D_VERSION=1.2.0
DAV1D_TAR="dav1d-${DAV1D_VERSION}.tar.bz2"
DAV1D_SITE="https://code.videolan.org/videolan/dav1d/-/archive/${DAV1D_VERSION}"
if download_step; then
    download "$DAV1D_SITE" "$DAV1D_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/dav1d.pc" ] || [ "$(pkg-config --modversion dav1d)" != "$DAV1D_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$DAV1D_TAR"
    pushd "dav1d-$DAV1D_VERSION"
    mkdir build
    pushd build
    env CFLAGS="$BF" CXXFLAGS="$BF" meson --prefix="$SDK_HOME" --libdir="lib"
    env CFLAGS="$BF" CXXFLAGS="$BF" ninja
    env CFLAGS="$BF" CXXFLAGS="$BF" ninja install
    popd
    popd
    rm -rf "dav1d-$DAV1D_VERSION"
    end_build
fi
