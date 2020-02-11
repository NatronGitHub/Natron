#!/bin/bash

# Install libmodplug
# see https://sourceforge.net/projects/modplug-xmms/files/libmodplug/
LIBMODPLUG_VERSION=0.8.9.0
LIBMODPLUG_TAR="libmodplug-${LIBMODPLUG_VERSION}.tar.gz"
LIBMODPLUG_SITE="https://sourceforge.net/projects/modplug-xmms/files/libmodplug/${LIBMODPLUG_VERSION}"
if download_step; then
    download "$LIBMODPLUG_SITE" "$LIBMODPLUG_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libmodplug.so" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBMODPLUG_TAR"
    pushd "libmodplug-$LIBMODPLUG_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libmodplug-$LIBMODPLUG_VERSION"
    end_build
fi
