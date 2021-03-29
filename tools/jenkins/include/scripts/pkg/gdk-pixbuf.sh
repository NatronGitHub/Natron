#!/bin/bash

# Install gdk-pixbuf
# see http://www.linuxfromscratch.org/blfs/view/svn/x/gdk-pixbuf.html
#GDKPIXBUF_VERSION=2.36.12 # last version to compile without meson
GDKPIXBUF_VERSION=2.42.2 # requires meson
GDKPIXBUF_VERSION_SHORT=${GDKPIXBUF_VERSION%.*}
GDKPIXBUF_TAR="gdk-pixbuf-${GDKPIXBUF_VERSION}.tar.xz"
GDKPIXBUF_SITE="http://ftp.gnome.org/pub/gnome/sources/gdk-pixbuf/${GDKPIXBUF_VERSION_SHORT}"
if download_step; then
    download "$GDKPIXBUF_SITE" "$GDKPIXBUF_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/gdk-pixbuf-2.0.pc" ] || [ "$(pkg-config --modversion gdk-pixbuf-2.0)" != "$GDKPIXBUF_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$GDKPIXBUF_TAR"
    pushd "gdk-pixbuf-${GDKPIXBUF_VERSION}"
    if version_gt "$GDKPIXBUF_VERSION" 2.36.12; then
        mkdir build
        pushd build

        env CFLAGS="$BF" CXXFLAGS="$BF" meson --prefix="$SDK_HOME" --libdir="lib" \
          -Dtiff=false -Dman=false -Ddocs=false -Dgtk_doc=false \
          ..
        env CFLAGS="$BF" CXXFLAGS="$BF" ninja
        env CFLAGS="$BF" CXXFLAGS="$BF" ninja install
        popd
    else
        env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --without-libtiff
        make -j${MKJOBS}
        make install
    fi
    popd
    rm -rf "gdk-pixbuf-${GDKPIXBUF_VERSION}"
    end_build
fi
