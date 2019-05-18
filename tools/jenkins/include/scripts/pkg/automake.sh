#!/bin/bash

# Install automake
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/automake.html
AUTOMAKE_VERSION=1.16.1
AUTOMAKE_TAR="automake-${AUTOMAKE_VERSION}.tar.xz"
AUTOMAKE_SITE="https://ftp.gnu.org/gnu/automake"
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/automake" ] || [ "$("$SDK_HOME/bin/automake" --version | head -1 | awk '{print $4}')" != "$AUTOMAKE_VERSION" ]; }; }; then
    start_build
    download "$AUTOMAKE_SITE" "$AUTOMAKE_TAR"
    untar "$SRC_PATH/$AUTOMAKE_TAR"
    pushd "automake-${AUTOMAKE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "automake-${AUTOMAKE_VERSION}"
    end_build
fi
