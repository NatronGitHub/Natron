#!/bin/bash

# Install libtasn1 (for gnutls)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libtasn1.html
LIBTASN1_VERSION=4.17.0
#LIBTASN1_VERSION_SHORT=${LIBTASN1_VERSION%.*}
LIBTASN1_TAR="libtasn1-${LIBTASN1_VERSION}.tar.gz"
LIBTASN1_SITE="https://ftp.gnu.org/gnu/libtasn1"
if download_step; then
    download "$LIBTASN1_SITE" "$LIBTASN1_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libtasn1.pc" ] || [ "$(pkg-config --modversion libtasn1)" != "$LIBTASN1_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBTASN1_TAR"
    pushd "libtasn1-${LIBTASN1_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --disable-doc
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libtasn1-${LIBTASN1_VERSION}"
    end_build
fi
