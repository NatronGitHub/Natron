#!/bin/bash

# Install autoconf
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/autoconf.html
AUTOCONF_VERSION=2.71
AUTOCONF_TAR="autoconf-${AUTOCONF_VERSION}.tar.xz"
AUTOCONF_SITE="https://ftp.gnu.org/gnu/autoconf"
if download_step; then
    download "$AUTOCONF_SITE" "$AUTOCONF_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/autoconf" ]; }; }; then
    start_build
    untar "$SRC_PATH/$AUTOCONF_TAR"
    pushd "autoconf-${AUTOCONF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "autoconf-${AUTOCONF_VERSION}"
    end_build
fi
