#!/bin/bash

# install librevenge
# see https://sourceforge.net/projects/libwpd/files/librevenge/
REVENGE_VERSION=0.0.5
REVENGE_TAR="librevenge-${REVENGE_VERSION}.tar.xz"
REVENGE_SITE="https://sourceforge.net/projects/libwpd/files/librevenge/librevenge-${REVENGE_VERSION}"
if download_step; then
    download "$REVENGE_SITE" "$REVENGE_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/librevenge-0.0.pc" ] || [ "$(pkg-config --modversion librevenge-0.0)" != "$REVENGE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$REVENGE_TAR"
    pushd "librevenge-${REVENGE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --disable-werror --prefix="$SDK_HOME" --disable-docs --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "librevenge-${REVENGE_VERSION}"
    end_build
fi
