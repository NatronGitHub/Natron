#!/bin/bash

# Install orc
# see https://gstreamer.freedesktop.org/src/orc/
#ORC_VERSION=0.4.29 # last version before meson
ORC_VERSION=0.4.33 # 0.4.30 and later use meson to build
ORC_TAR="orc-${ORC_VERSION}.tar.xz"
ORC_SITE="http://gstreamer.freedesktop.org/src/orc"
if download_step; then
    download "$ORC_SITE" "$ORC_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/orc-0.4.pc" ] || [ "$(pkg-config --modversion orc-0.4)" != "$ORC_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$ORC_TAR"
    pushd "orc-$ORC_VERSION"
    if version_gt "$ORC_VERSION" 0.4.29; then
        mkdir build
        pushd build
        env CFLAGS="$BF" CXXFLAGS="$BF" meson --prefix="$SDK_HOME" --libdir="lib"
        env CFLAGS="$BF" CXXFLAGS="$BF" ninja
        env CFLAGS="$BF" CXXFLAGS="$BF" ninja install
        popd
    else
        env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
        make -j${MKJOBS}
        make install
    fi
    popd
    rm -rf "orc-$ORC_VERSION"
    end_build
fi
