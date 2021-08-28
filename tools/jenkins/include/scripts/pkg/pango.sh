#!/bin/bash

# Install pango
# see http://www.linuxfromscratch.org/blfs/view/svn/x/pango.html
#PANGO_VERSION=1.42.4 # last version before meson
PANGO_VERSION=1.48.9 # requires meson
PANGO_VERSION_SHORT=${PANGO_VERSION%.*}
PANGO_TAR="pango-${PANGO_VERSION}.tar.xz"
PANGO_SITE="http://ftp.gnome.org/pub/GNOME/sources/pango/${PANGO_VERSION_SHORT}"
if download_step; then
    download "$PANGO_SITE" "$PANGO_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_PANGO:-}" = "1" ]; }; }; then
    rm -rf $SDK_HOME/include/pango* $SDK_HOME/lib/libpango* $SDK_HOME/lib/pkgconfig/pango* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/pango.pc" ] || [ "$(pkg-config --modversion pango)" != "$PANGO_VERSION" ]; }; }; then
    REBUILD_SVG=1
    start_build
    untar "$SRC_PATH/$PANGO_TAR"
    pushd "pango-${PANGO_VERSION}"
    if version_gt "$PANGO_VERSION" 1.42.4; then
        mkdir build
        pushd build

        env CFLAGS="$BF" CXXFLAGS="$BF" meson --prefix="$SDK_HOME" --libdir="lib" ..
        env CFLAGS="$BF" CXXFLAGS="$BF" ninja
        env CFLAGS="$BF" CXXFLAGS="$BF" ninja install
        popd
    else
        env FONTCONFIG_LIBS="-lfontconfig" CFLAGS="$BF -g" CXXFLAGS="$BF -g" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --with-included-modules=basic-fc
        make -j${MKJOBS}
        make install
    fi
    popd
    rm -rf "pango-${PANGO_VERSION}"
    end_build
fi
