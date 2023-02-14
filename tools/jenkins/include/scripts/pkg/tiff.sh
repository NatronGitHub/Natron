#!/bin/bash

# Install tiff
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libtiff.html
TIFF_VERSION=4.5.0
TIFF_TAR="tiff-${TIFF_VERSION}.tar.gz"
TIFF_SITE="http://download.osgeo.org/libtiff"
if download_step; then
    download "$TIFF_SITE" "$TIFF_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_TIFF:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/{lib,include}/*tiff* || true
    rm -f "$SDK_HOME"/lib/pkgconfig/*tiff* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libtiff-4.pc" ] || [ "$(pkg-config --modversion libtiff-4)" != "$TIFF_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$TIFF_TAR"
    pushd "tiff-${TIFF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "tiff-${TIFF_VERSION}"
    end_build
fi
