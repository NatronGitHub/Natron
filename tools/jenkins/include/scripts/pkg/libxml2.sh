#!/bin/bash

# Install libxml2
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libxml2.html
LIBXML2_VERSION=2.9.9
LIBXML2_TAR="libxml2-${LIBXML2_VERSION}.tar.gz"
LIBXML2_SITE="ftp://xmlsoft.org/libxml2"
LIBXML2_ICU=1 # set to 1 if libxml2 should be compiled with ICU support. This implies things for Qt4 and QtWebkit.
if build_step && { force_build || { [ "${REBUILD_XML:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/include/libxml2 "$SDK_HOME"/lib/libxml* "$SDK_HOME"/lib/pkgconfig/libxml* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libxml-2.0.pc" ] || [ "$(pkg-config --modversion libxml-2.0)" != "$LIBXML2_VERSION" ]; }; }; then
    REBUILD_XSLT=1
    start_build
    download "$LIBXML2_SITE" "$LIBXML2_TAR"
    untar "$SRC_PATH/$LIBXML2_TAR"
    pushd "libxml2-${LIBXML2_VERSION}"
    if [ "$LIBXML2_ICU" -eq 1 ]; then
        icu_flag=--with-icu
    else
        icu_flag=--without-icu
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --with-history $icu_flag --without-python --with-lzma --with-threads
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libxml2-${LIBXML2_VERSION}"
    end_build
fi
