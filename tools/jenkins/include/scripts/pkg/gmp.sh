#!/bin/bash

# Install gmp (used by ruby)
# version is set in gcc.sh
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/gmp.html
GMP_VERSION=6.2.0 # 6.2.0 fails when buiding using GCC 4.4.7 (CentOS6): requires -std=gnu99 but adding it to CFLAGS during configure doesn't help
if version_gt 5.1.0 "${GCC_VERSION}" && [ "${DTS:-0}" -le 3 ] && [ "${CENTOS:-6}" -le 8 ]; then
    GMP_VERSION=6.1.2 # 6.2.0 requires c99
fi
GMP_TAR="gmp-${GMP_VERSION}.tar.bz2"
GMP_SITE="https://gmplib.org/download/gmp"
if download_step; then
    download "$GMP_SITE" "$GMP_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/include/gmp.h" ]; }; }; then
    REBUILD_RUBY=1
    start_build
    untar "$SRC_PATH/$GMP_TAR"
    pushd "gmp-${GMP_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --enable-cxx --build="${ARCH}-unknown-linux-gnu"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gmp-${GMP_VERSION}"
    end_build
fi
