#!/bin/bash

# install libcdr
# see https://dev-www.libreoffice.org/src/libcdr/
LIBCDR_VERSION=0.1.8
LIBCDR_TAR="libcdr-${LIBCDR_VERSION}.tar.xz"
LIBCDR_SHA256=ced677c8300b29c91d3004bb1dddf0b99761bf5544991c26c2ee8f427e87193c
LIBCDR_SITE="https://dev-www.libreoffice.org/src/libcdr"
if download_step; then
    download "$LIBCDR_SITE" "$LIBCDR_TAR" "$LIBCDR_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libcdr-0.1.pc" ] || [ "$(pkg-config --modversion libcdr-0.1)" != "$LIBCDR_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBCDR_TAR"
    pushd "libcdr-${LIBCDR_VERSION}"
    if [ "${ICU_VERSION:-}" = 68.1 ]; then
        $GSED -i 's/ TRUE/ true/' src/lib/libcdr_utils.cpp
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --disable-werror --prefix="$SDK_HOME" --disable-docs --enable-shared
    if [ "${ICU_VERSION:-}" = 68.1 ] || [ "${ICU_VERSION:-}" = 68.2 ]; then
        $GSED -i 's/ TRUE/ true/' src/lib/libcdr_utils.cpp
    fi
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libcdr-${LIBCDR_VERSION}"
    end_build
fi
