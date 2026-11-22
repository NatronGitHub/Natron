#!/bin/bash

# Install openssl
# OpenSSL 1.1.1 is end-of-life: use OpenSSL 3 (as MacPorts).
OPENSSL_VERSION=3.6.5
OPENSSL_TAR="openssl-${OPENSSL_VERSION}.tar.gz"
OPENSSL_SITE="https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}"
# Fedora's options (https://src.fedoraproject.org/rpms/openssl) without the weak protocols,
# ciphers and algorithms; installed into lib (not lib64), without the tests or the docs
# (install_sw: libraries, headers, programs; install_ssldirs: the configuration directory)
OPENSSL_CONFIG="--libdir=lib no-tests no-docs enable-camellia enable-seed enable-rfc3779 enable-cms no-mdc2 no-ec2m no-sm2 no-sm4"
OPENSSL_INSTALL_TARGETS="install_sw install_ssldirs"
OPENSSL_SHA256=a2157c2830efdec3788939b00c9b0638306d3f0bbb76dc4832ee503bb397df98
if download_step; then
    download "$OPENSSL_SITE" "$OPENSSL_TAR" "$OPENSSL_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/openssl.pc" ] || [ "$(env PKG_CONFIG_PATH="$SDK_HOME/lib/pkgconfig:$SDK_HOME/share/pkgconfig" pkg-config --modversion openssl)" != "$OPENSSL_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$OPENSSL_TAR"
    pushd "openssl-$OPENSSL_VERSION"
    # Use versioned symbols for OpenSSL binary compatibility.
    # Patch from http://www.linuxfromscratch.org/patches/downloads/openssl/
    if version_gt 1.1.0 "$OPENSSL_VERSION"; then
        patch -Np1 -i "$INC_PATH"/patches/openssl/openssl-1.0.2m-compat_versioned_symbols-1.patch
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./config --prefix="$SDK_HOME" shared ${OPENSSL_CONFIG}
    perl configdata.pm --dump
    # no parallel build for openssl < 1.1.0, and it's safer to avoid parallel build
    # for a crypto library anyway:
    # https://github.com/openssl/openssl/issues/5762
    # https://github.com/openssl/openssl/issues/2331
    # https://github.com/openssl/openssl/issues/298
    make
    make ${OPENSSL_INSTALL_TARGETS}
    popd
    rm -rf "openssl-$OPENSSL_VERSION"
    end_build
fi
