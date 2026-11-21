#!/bin/bash

# Install gnutls (for ffmpeg)
# see http://www.linuxfromscratch.org/blfs/view/svn/postlfs/gnutls.html
GNUTLS_VERSION=3.8.13
GNUTLS_SHA256=ffed8ec1bf09c2426d4f14aae377de4753b53e537d685e604e99a8b16ca9c97e
GNUTLS_VERSION_MICRO= #.1
if [ "${CENTOS:-0}" = 6 ] && [ -z "${DTS+x}" ]; then
    GNUTLS_VERSION=3.6.8
    GNUTLS_SHA256= # (not checked)
    GNUTLS_VERSION_MICRO=
    # 3.6.9 and later fail on CentOS6 (probably needs an updated binutils/gas) with:
    #  CCAS     elf/sha512-ssse3-x86_64.lo
    #elf/sha1-ssse3-x86_64.s: Assembler messages:
    #elf/sha1-ssse3-x86_64.s:3825: Error: no such instruction: `vinserti128 $1,(%r13),%ymm0,%ymm0'
    #...
fi
GNUTLS_VERSION_SHORT=${GNUTLS_VERSION%.*}
GNUTLS_TAR="gnutls-${GNUTLS_VERSION}${GNUTLS_VERSION_MICRO}.tar.xz"
GNUTLS_SITE="https://www.gnupg.org/ftp/gcrypt/gnutls/v${GNUTLS_VERSION_SHORT}"
if download_step; then
    download "$GNUTLS_SITE" "$GNUTLS_TAR" "$GNUTLS_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/gnutls.pc" ] || [ "$(pkg-config --modversion gnutls)" != "$GNUTLS_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$GNUTLS_TAR"
    pushd "gnutls-${GNUTLS_VERSION}${GNUTLS_VERSION_MICRO}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --disable-doc --disable-tests --with-default-trust-store-pkcs11="pkcs11:"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gnutls-${GNUTLS_VERSION}${GNUTLS_VERSION_MICRO}"
    end_build
fi
