#!/bin/bash

# strings is not needed anymore since the following commits from Natron-support:
# f090ad15ff4efa64634c4e9b14409ea38a643d45 (May 12 2017) fix Natron linux launch script
# c3f7f97350e1e0b4c543b68bd2c80c1eee803aff (Oct 24 2017) natron-linux: do not set LD_LIBRARY_PATH

# install strings 
BINUTILS_VERSION=2.32
BINUTILS_TAR="binutils-${BINUTILS_VERSION}.tar.gz"
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
