#!/bin/bash

# Install openssl for installer
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/openssl.html
#OPENSSL_VERSION=1.1.1b
# see https://web.archive.org/web/20181008083143/http://www.linuxfromscratch.org/blfs/view/svn/postlfs/openssl10.html
OPENSSL_VERSION=1.0.2r
OPENSSL_TAR="openssl-${OPENSSL_VERSION}.tar.gz" # always a new version around the corner
OPENSSL_SITE="https://www.openssl.org/source"
if build_step && { force_build || { [ ! -s "$SDK_HOME/installer/lib/pkgconfig/openssl.pc" ] || [ "$(env PKG_CONFIG_PATH="$SDK_HOME/installer/lib/pkgconfig:$SDK_HOME/installer/share/pkgconfig" pkg-config --modversion openssl)" != "$OPENSSL_VERSION" ]; }; }; then
    start_build
    download "$OPENSSL_SITE" "$OPENSSL_TAR"
    untar "$SRC_PATH/$OPENSSL_TAR"
    pushd "openssl-$OPENSSL_VERSION"
    # Use versioned symbols for OpenSSL binary compatibility.
    if version_gt 1.1.0 "$OPENSSL_VERSION"; then
	patch -Np1 -i "$INC_PATH"/patches/openssl/openssl-1.0.2m-compat_versioned_symbols-1.patch
    fi
    # the static qt4 uses -openssl-linked to link to the static version
    ./config --prefix="$SDK_HOME/installer" no-shared
    # no parallel build for openssl, see https://github.com/openssl/openssl/issues/298
    make
    make install
    popd
    rm -rf "openssl-$OPENSSL_VERSION"
    end_build
fi
