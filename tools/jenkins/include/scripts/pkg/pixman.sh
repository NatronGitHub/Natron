#!/bin/bash

# Install pixman
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/pixman.html
PIXMAN_VERSION=0.34.0
PIXMAN_TAR="pixman-${PIXMAN_VERSION}.tar.gz"
PIXMAN_SITE="https://www.cairographics.org/releases"
if build_step && { force_build || { [ "${REBUILD_PIXMAN:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/{lib,include}/*pixman* || true
    rm -f "$SDK_HOME"/lib/pkgconfig/*pixman* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/pixman-1.pc" ] || [ "$(pkg-config --modversion pixman-1)" != "$PIXMAN_VERSION" ]; }; }; then
    REBUILD_CAIRO=1
    start_build
    download "$PIXMAN_SITE" "$PIXMAN_TAR"
    untar "$SRC_PATH/$PIXMAN_TAR"
    pushd "pixman-${PIXMAN_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "pixman-${PIXMAN_VERSION}"
    end_build
fi
