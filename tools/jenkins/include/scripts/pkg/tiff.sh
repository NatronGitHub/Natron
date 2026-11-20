#!/bin/bash

# Install tiff
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libtiff.html
TIFF_VERSION=4.7.2
TIFF_TAR="tiff-${TIFF_VERSION}.tar.gz"
TIFF_SITE="https://download.osgeo.org/libtiff"
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
    # --disable-sphinx: the SDK's Python has sphinx, and the doc-man and doc-html targets then
    # run in parallel on the same doctree cache, which randomly fails with
    # "_pickle.UnpicklingError: pickle data was truncated". The SDK does not need the libtiff docs.
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static --disable-sphinx
    make -j${MKJOBS}
    make install
    popd
    rm -rf "tiff-${TIFF_VERSION}"
    end_build
fi
