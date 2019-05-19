#!/bin/bash

# Install gdk-pixbuf
# see http://www.linuxfromscratch.org/blfs/view/svn/x/gdk-pixbuf.html
GDKPIXBUF_VERSION=2.36.11
GDKPIXBUF_VERSION_SHORT=${GDKPIXBUF_VERSION%.*}
GDKPIXBUF_TAR="gdk-pixbuf-${GDKPIXBUF_VERSION}.tar.xz"
GDKPIXBUF_SITE="http://ftp.gnome.org/pub/gnome/sources/gdk-pixbuf/${GDKPIXBUF_VERSION_SHORT}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/gdk-pixbuf-2.0.pc" ] || [ "$(pkg-config --modversion gdk-pixbuf-2.0)" != "$GDKPIXBUF_VERSION" ]; }; }; then
    start_build
    download "$GDKPIXBUF_SITE" "$GDKPIXBUF_TAR"
    untar "$SRC_PATH/$GDKPIXBUF_TAR"
    pushd "gdk-pixbuf-${GDKPIXBUF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --without-libtiff
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gdk-pixbuf-${GDKPIXBUF_VERSION}"
    end_build
fi
