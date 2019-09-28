#!/bin/bash

# Install expat
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/expat.html
EXPAT_VERSION=2.2.8
EXPAT_TAR="expat-${EXPAT_VERSION}.tar.bz2"
EXPAT_SITE="https://sourceforge.net/projects/expat/files/expat/${EXPAT_VERSION}"
if build_step && { force_build || { [ "${REBUILD_EXPAT:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/include/expat*.h "$SDK_HOME"/lib/libexpat* "$SDK_HOME"/lib/pkgconfig/expat* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/expat.pc" ] || [ "$(pkg-config --modversion expat)" != "$EXPAT_VERSION" ]; }; }; then
    start_build
    download "$EXPAT_SITE" "$EXPAT_TAR"
    untar "$SRC_PATH/$EXPAT_TAR"
    pushd "expat-${EXPAT_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "expat-${EXPAT_VERSION}"
    end_build
fi
