#!/bin/bash

# Install openssl
# see http://www.linuxfromscratch.org/blfs/view/svn/postlfs/openssl10.html
#OPENSSL_VERSION=1.1.1b # defined in openssl-installer.sh
OPENSSL_TAR="openssl-${OPENSSL_VERSION}.tar.gz" # always a new version around the corner
OPENSSL_SITE="https://www.openssl.org/source"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/openssl.pc" ] || [ "$(env PKG_CONFIG_PATH="$SDK_HOME/lib/pkgconfig:$SDK_HOME/share/pkgconfig" pkg-config --modversion openssl)" != "$OPENSSL_VERSION" ]; }; }; then
    start_build
    download "$OPENSSL_SITE" "$OPENSSL_TAR"
    untar "$SRC_PATH/$OPENSSL_TAR"
    pushd "openssl-$OPENSSL_VERSION"
    # Use versioned symbols for OpenSSL binary compatibility.
    patch -Np1 -i "$INC_PATH"/patches/openssl/openssl-1.0.2m-compat_versioned_symbols-1.patch
    env CFLAGS="$BF" CXXFLAGS="$BF" ./config --prefix="$SDK_HOME" shared enable-cms enable-camellia enable-idea enable-mdc2 enable-tlsext enable-rfc3779
    # no parallel build for openssl, see https://github.com/openssl/openssl/issues/298
    make
    make install
    popd
    rm -rf "openssl-$OPENSSL_VERSION"
    end_build
fi
