#!/bin/bash

# Install valgrind
# see http://www.linuxfromscratch.org/blfs/view/svn/general/valgrind.html
VALGRIND_VERSION=3.20.0
VALGRIND_TAR="valgrind-${VALGRIND_VERSION}.tar.bz2"
VALGRIND_SITE="https://sourceware.org/ftp/valgrind"
if download_step; then
    download "$VALGRIND_SITE" "$VALGRIND_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/valgrind" ]; }; }; then
    start_build
    untar "$SRC_PATH/$VALGRIND_TAR"
    pushd "valgrind-${VALGRIND_VERSION}"
    sed -i '1904s/4/5/' coregrind/m_syswrap/syswrap-linux.c
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-only"${BITS}"bit
    make -j${MKJOBS}
    make install
    popd
    rm -rf "valgrind-${VALGRIND_VERSION}"
    end_build
fi
