#!/bin/bash

# Install openssl for installer
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/openssl.html
OPENSSL_VERSION=1.1.1f
# see https://web.archive.org/web/20181008083143/http://www.linuxfromscratch.org/blfs/view/svn/postlfs/openssl10.html
#OPENSSL_VERSION=1.0.2t
OPENSSL_TAR="openssl-${OPENSSL_VERSION}.tar.gz" # always a new version around the corner
OPENSSL_SITE="https://www.openssl.org/source"
# These are the default options from Fedora https://src.fedoraproject.org/rpms/openssl:
# zlib enable-camellia enable-seed enable-rfc3779 enable-sctp enable-cms enable-md2 enable-rc5 enable-ssl3 enable-ssl3-method enable-weak-ssl-ciphers no-mdc2 no-ec2m no-sm2 no-sm4"
#
# We need enable-ssl3-method, or legacy code (including Qt4) may fail to compile. See:
# - https://code.qt.io/cgit/qt/qtbase.git/commit?id=6839aead0430a9b07b60fa3a1a7d685fe5d2d1ef
# - https://trac.macports.org/ticket/58205#comment:9
#
# Working config for the installer openssl:
OPENSSL_CONFIG_INSTALLER="enable-ssl3-method"
# Working config for the SDK openssl:
OPENSSL_CONFIG="enable-cms enable-camellia enable-idea enable-mdc2 enable-tlsext enable-rfc3779 enable-ssl3-method"
if download_step; then
    download "$OPENSSL_SITE" "$OPENSSL_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/installer/lib/pkgconfig/openssl.pc" ] || [ "$(env PKG_CONFIG_PATH="$SDK_HOME/installer/lib/pkgconfig:$SDK_HOME/installer/share/pkgconfig" pkg-config --modversion openssl)" != "$OPENSSL_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$OPENSSL_TAR"
    pushd "openssl-$OPENSSL_VERSION"
    # Use versioned symbols for OpenSSL binary compatibility.
    # Patch from http://www.linuxfromscratch.org/patches/downloads/openssl/
    if version_gt 1.1.0 "$OPENSSL_VERSION"; then
        patch -Np1 -i "$INC_PATH"/patches/openssl/openssl-1.0.2m-compat_versioned_symbols-1.patch
    fi
    # the static qt4 uses -openssl-linked to link to the static version
    ./config --prefix="$SDK_HOME/installer" no-shared ${OPENSSL_CONFIG_INSTALLER}
    perl configdata.pm --dump
    # no parallel build for openssl < 1.1.0, and it's safer to avoid parallel build
    # for a crypto library anyway:
    # https://github.com/openssl/openssl/issues/5762
    # https://github.com/openssl/openssl/issues/2331
    # https://github.com/openssl/openssl/issues/298
    make
    make install
    popd
    rm -rf "openssl-$OPENSSL_VERSION"
    end_build
fi
