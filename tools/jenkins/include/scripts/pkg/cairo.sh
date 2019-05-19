#!/bin/bash

# Install cairo
# see http://www.linuxfromscratch.org/blfs/view/svn/x/cairo.html
CAIRO_VERSION=1.14.12
CAIRO_TAR="cairo-${CAIRO_VERSION}.tar.xz"
CAIRO_SITE="https://www.cairographics.org/releases"
if build_step && { force_build || { [ "${REBUILD_CAIRO:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/include/cairo || true
    rm -f "$SDK_HOME"/lib/pkgconfig/{cairo-*.pc,cairo.pc} || true
    rm -f "$SDK_HOME"/lib/libcairo* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/cairo.pc" ] || [ "$(pkg-config --modversion cairo)" != "$CAIRO_VERSION" ]; }; }; then
    REBUILD_PANGO=1
    REBUILD_SVG=1
    REBUILD_BUZZ=1
    REBUILD_POPPLER=1
    start_build
    download "$CAIRO_SITE" "$CAIRO_TAR"
    untar "$SRC_PATH/$CAIRO_TAR"
    pushd "cairo-${CAIRO_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${SDK_HOME}/include/pixman-1" LDFLAGS="-L${SDK_HOME}/lib -lpixman-1" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --with-libpng=yes --enable-shared --enable-static --enable-tee
    make -j${MKJOBS}
    make install
    popd
    rm -rf "cairo-${CAIRO_VERSION}"
    end_build
fi
