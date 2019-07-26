#!/bin/bash

# install strings 
BINUTILS_VERSION=2.32
BINUTILS_TAR="binutils-${BINUTILS_VERSION}.tar.xz"
BINUTILS_SITE="https://ftp.gnu.org/gnu/binutils"
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/strings" ]; }; }; then
    start_build
    download "$BINUTILS_SITE" "$BINUTILS_TAR"
    untar "$SRC_PATH/$BINUTILS_TAR"
    pushd "binutils-${BINUTILS_VERSION}"
    ./configure --enable-static --disable-shared --disable-nls --disable-werror
    make -j${MKJOBS}
    strip -s binutils/strings
    cp binutils/strings $SDK_HOME/bin/
    popd
    rm -rf "binutils-${BINUTILS_VERSION}"
    end_build
fi
