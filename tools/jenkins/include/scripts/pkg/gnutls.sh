#!/bin/bash

# Install gnutls (for ffmpeg)
# see http://www.linuxfromscratch.org/blfs/view/cvs/postlfs/gnutls.html
GNUTLS_VERSION=3.6.8
#GNUTLS_VERSION=3.6.11
# 3.6.9 and later fail on CentOS6 (probably needs an updated binutils/gas) with:
#  CCAS     elf/sha512-ssse3-x86_64.lo
#elf/sha1-ssse3-x86_64.s: Assembler messages:
#elf/sha1-ssse3-x86_64.s:3825: Error: no such instruction: `vinserti128 $1,(%r13),%ymm0,%ymm0'
#...

GNUTLS_VERSION_MICRO= #.1
GNUTLS_VERSION_SHORT=${GNUTLS_VERSION%.*}
GNUTLS_TAR="gnutls-${GNUTLS_VERSION}${GNUTLS_VERSION_MICRO}.tar.xz"
GNUTLS_SITE="ftp://ftp.gnupg.org/gcrypt/gnutls/v${GNUTLS_VERSION_SHORT}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/gnutls.pc" ] || [ "$(pkg-config --modversion gnutls)" != "$GNUTLS_VERSION" ]; }; }; then
    start_build
    download "$GNUTLS_SITE" "$GNUTLS_TAR"
    untar "$SRC_PATH/$GNUTLS_TAR"
    pushd "gnutls-${GNUTLS_VERSION}${GNUTLS_VERSION_MICRO}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --with-default-trust-store-pkcs11="pkcs11:"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gnutls-${GNUTLS_VERSION}${GNUTLS_VERSION_MICRO}"
    end_build
fi
