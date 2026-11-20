#!/bin/bash

# Install libpng
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libpng.html
LIBPNG_VERSION=1.6.59 
LIBPNG_TAR="libpng-${LIBPNG_VERSION}.tar.gz"
LIBPNG_SHA256=86a3e4b501f7f50e392c4e456ea158893e9a595b1c63eb88c7bb9f6cf8772dad
LIBPNG_SITE="https://download.sourceforge.net/libpng"
if download_step; then
    download "$LIBPNG_SITE" "$LIBPNG_TAR" "$LIBPNG_SHA256"
fi
if build_step && { force_build || { [ "${REBUILD_PNG:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/{include,lib}/*png* || true
    rm -f "$SDK_HOME"/lib/pkgconfig/*png* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libpng.pc" ] || [ "$(pkg-config --modversion libpng)" != "$LIBPNG_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBPNG_TAR"
    pushd "libpng-${LIBPNG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" LIBS=-lpthread ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libpng-${LIBPNG_VERSION}"
    end_build
fi
