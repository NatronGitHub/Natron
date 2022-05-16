#!/bin/bash

# Install pcre (required by glib)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/pcre2.html
PCRE2_VERSION=10.40
PCRE2_TAR="pcre2-${PCRE2_VERSION}.tar.bz2"
PCRE2_SITE="https://github.com/PhilipHazel/pcre2/releases/download/pcre2-${PCRE2_VERSION}"
if download_step; then
    download "$PCRE2_SITE" "$PCRE2_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libpcre.pc" ] || [ "$(pkg-config --modversion libpcre)" != "$PCRE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$PCRE2_TAR"
    pushd "pcre2-${PCRE2_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --enable-unicode --enable-jit --enable-pcre2-16 --enable-pcre2-32 --enable-pcre2grep-libz --enable-pcre2grep-libbz2 --enable-pcre2test-libreadline
    make -j${MKJOBS}
    make install
    popd
    rm -rf "pcre2-${PCRE2_VERSION}"
    end_build
fi
