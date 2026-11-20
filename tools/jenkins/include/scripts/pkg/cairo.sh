#!/bin/bash

# Install cairo
# see http://www.linuxfromscratch.org/blfs/view/svn/x/cairo.html
CAIRO_VERSION=1.18.6
CAIRO_VERSION_SHORT=${CAIRO_VERSION%.*}
CAIRO_VERSION_PKGCONFIG=${CAIRO_VERSION}
CAIRO_TAR="cairo-${CAIRO_VERSION}.tar.xz"
CAIRO_SITE="https://www.cairographics.org/releases"
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
    if [ -f configure.ac ]; then
        # autoreconf -fv
        # We don't want to run gtkdocize, so we run the autoreconf commands ourselves:
        aclocal --force -I m4
        libtoolize --copy --force
        autoconf --force
        automake --add-missing --copy --force-missing
        env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${SDK_HOME}/include/pixman-1" LDFLAGS="-L${SDK_HOME}/lib -lpixman-1" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static --disable-tee --disable-xlib-xcb --disable-gl --disable-gtk-doc --disable-dependency-tracking
        make -j${MKJOBS}
        make install
    else
        # cairo >= 1.18 only builds with meson. No X11 backends: nothing in the SDK uses them,
        # and the bundled libcairo must not need the X11 client libraries.
        env CFLAGS="$BF" CXXFLAGS="$BF" meson setup build --prefix="$SDK_HOME" --libdir=lib --buildtype=release \
            --default-library=both -Dtests=disabled -Dgtk_doc=false -Dtee=disabled \
            -Dxlib=disabled -Dxcb=disabled -Dxlib-xcb=disabled -Dquartz=disabled -Ddwrite=disabled \
            -Dpng=enabled -Dfreetype=enabled -Dfontconfig=enabled -Dglib=enabled -Dzlib=enabled \
            -Dspectre=disabled -Dsymbol-lookup=disabled -Dlzo=disabled -Dgtk2-utils=disabled
        ninja -C build -j${MKJOBS}
        ninja -C build install
    fi
    popd
    rm -rf "cairo-${CAIRO_VERSION}"
    end_build
fi
