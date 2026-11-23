#!/bin/bash

# Install gmp (for nettle's hogweed, used by gnutls)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/gmp.html
GMP_VERSION=6.3.0 # 6.2.0 fails when buiding using GCC 4.4.7 (CentOS6): requires -std=gnu99 but adding it to CFLAGS during configure doesn't help
GMP_SHA256=ac28211a7cfb609bae2e2c8d6058d66c8fe96434f740cf6fe2e47b000d1c20cb
if version_gt 5.1.0 "${GCC_VERSION}" && version_gt 4.8.5 "${BOOTSTRAP_GCC_VERSION:-4.4.7}"; then
    GMP_VERSION=6.1.2 # 6.2.0 requires c99
    GMP_SHA256= # (not checked)
fi
GMP_TAR="gmp-${GMP_VERSION}.tar.bz2"
GMP_SITE="https://ftp.gnu.org/gnu/gmp" # gmplib.org is often unreachable
if download_step; then
    download "$GMP_SITE" "$GMP_TAR" "$GMP_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/include/gmp.h" ]; }; }; then
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
