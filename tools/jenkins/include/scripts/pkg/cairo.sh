#!/bin/bash

# Install cairo
# see http://www.linuxfromscratch.org/blfs/view/svn/x/cairo.html
CAIRO_VERSION=1.17.6
CAIRO_VERSION_SHORT=${CAIRO_VERSION%.*}
CAIRO_VERSION_PKGCONFIG=${CAIRO_VERSION}
CAIRO_TAR="cairo-${CAIRO_VERSION}.tar.xz"
CAIRO_SITE="https://download.gnome.org/sources/cairo/${CAIRO_VERSION_SHORT}"
if download_step; then
    download "$CAIRO_SITE" "$CAIRO_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_CAIRO:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/include/cairo || true
    rm -f "$SDK_HOME"/lib/pkgconfig/{cairo-*.pc,cairo.pc} || true
    rm -f "$SDK_HOME"/lib/libcairo* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/cairo.pc" ] || [ "$(pkg-config --modversion cairo)" != "$CAIRO_VERSION_PKGCONFIG" ]; }; }; then
    REBUILD_PANGO=1
    REBUILD_SVG=1
    REBUILD_BUZZ=1
    REBUILD_POPPLER=1
    start_build
    untar "$SRC_PATH/$CAIRO_TAR"
    pushd "cairo-${CAIRO_VERSION}"
    # autoreconf -fv
    # We don't want to run gtkdocize, so we run the autoreconf commands ourselves:
    aclocal --force -I m4
    libtoolize --copy --force
    autoconf --force
    automake --add-missing --copy --force-missing
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${SDK_HOME}/include/pixman-1" LDFLAGS="-L${SDK_HOME}/lib -lpixman-1" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static --disable-tee --disable-xlib-xcb --disable-gl --disable-gtk-doc --disable-dependency-tracking
    make -j${MKJOBS}
    make install
    popd
    rm -rf "cairo-${CAIRO_VERSION}"
    end_build
fi
