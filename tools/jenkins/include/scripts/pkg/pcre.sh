#!/bin/bash

# Install pcre (required by glib)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/pcre.html
PCRE_VERSION=8.45
PCRE_TAR="pcre-${PCRE_VERSION}.tar.bz2"
PCRE_SITE="https://ftp.pcre.org/pub/pcre"
if download_step; then
    download "$PCRE_SITE" "$PCRE_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libpcre.pc" ] || [ "$(pkg-config --modversion libpcre)" != "$PCRE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$PCRE_TAR"
    pushd "pcre-${PCRE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --enable-utf8 --enable-unicode-properties --enable-pcre16 --enable-pcre32 --enable-pcregrep-libz --enable-pcregrep-libbz2 --enable-pcretest-libreadline
    make -j${MKJOBS}
    make install
    popd
    rm -rf "pcre-${PCRE_VERSION}"
    end_build
fi
