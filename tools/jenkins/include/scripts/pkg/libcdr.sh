#!/bin/bash

# install libcdr
LIBCDR_VERSION=0.1.4
LIBCDR_TAR="libcdr-${LIBCDR_VERSION}.tar.xz"
LIBCDR_SITE="http://dev-www.libreoffice.org/src/libcdr"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libcdr-0.1.pc" ] || [ "$(pkg-config --modversion libcdr-0.1)" != "$LIBCDR_VERSION" ]; }; }; then
    start_build
    download "$LIBCDR_SITE" "$LIBCDR_TAR"
    untar "$SRC_PATH/$LIBCDR_TAR"
    pushd "libcdr-${LIBCDR_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --disable-werror --prefix="$SDK_HOME" --disable-docs --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libcdr-${LIBCDR_VERSION}"
    end_build
fi
