#!/bin/bash

# Install orc
# see https://gstreamer.freedesktop.org/src/orc/
ORC_VERSION=0.4.29 # 0.4.30 uses meson to build
ORC_TAR="orc-${ORC_VERSION}.tar.xz"
ORC_SITE="http://gstreamer.freedesktop.org/src/orc"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/orc-0.4.pc" ] || [ "$(pkg-config --modversion orc-0.4)" != "$ORC_VERSION" ]; }; }; then
    start_build
    download "$ORC_SITE" "$ORC_TAR"
    untar "$SRC_PATH/$ORC_TAR"
    pushd "orc-$ORC_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "orc-$ORC_VERSION"
    end_build
fi
